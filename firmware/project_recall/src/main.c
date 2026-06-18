#include "app_state.h"
#include "audio/audio_io.h"
#include "audio/chime_data.h"
#include "audio/codec_adpcm.h"
#include "audio/decimator.h"
#include "ble/ble_audio_svc.h"
#include "ble/ble_conn.h"
#include "power/battery.h"
#include "power/charger.h"
#include "power/pm.h"
#include "ui/epd_ui.h"
#include "ui/led_status.h"
#include "util/ringbuf.h"
#include "util/task_wdt_mgr.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>

#if defined(CONFIG_MCUBOOT)
#include <zephyr/dfu/mcuboot.h>
#endif

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

#define PCM_PER_BLOCK CONFIG_RECALL_PCM_BLOCK_SAMPLES
#define ADPCM_PKT     (ADPCM_BLOCK_HEADER_SIZE + (PCM_PER_BLOCK + 1) / 2 + 2)

static K_THREAD_STACK_DEFINE(cap_stack, 4096);
static K_THREAD_STACK_DEFINE(tx_stack, 4096);
static struct k_thread cap_tid;
static struct k_thread tx_tid;

static uint8_t comp_buf[CONFIG_RECALL_COMPRESSED_RING_SIZE];
static uint8_t notify_pkt[244];
static struct adpcm_state adpcm;

static int wdt_main, wdt_cap, wdt_tx;
static bool live;
static uint16_t seq;
static uint32_t beat_secs = CONFIG_RECALL_HEARTBEAT_INTERVAL_SEC;

static void refresh_ui(enum app_state s)
{
	if (pm_allow_ui_refresh()) {
		led_status_apply_state(s);
		epd_ui_apply_state(s);
	}
}

static void on_state(enum app_state prev, enum app_state next)
{
	ARG_UNUSED(prev);
	refresh_ui(next);
	recall_status_update();
}

static void on_link(bool up)
{
	if (up) {
		app_state_post_event(APP_EVT_BLE_CONNECTED);
	} else {
		live = false;
		ringbuf_reset();
		app_state_post_event(APP_EVT_BLE_DISCONNECTED);
		ble_conn_start_advertising();
	}
}

static void on_ctrl(uint8_t cmd, const uint8_t *data, size_t len)
{
	switch (cmd) {
	case 0x01:
		live = true;
		pm_heartbeat_begin();
		app_state_post_event(APP_EVT_STREAM_START);
		audio_io_capture_start();
		break;
	case 0x02:
		live = false;
		pm_heartbeat_end();
		app_state_post_event(APP_EVT_STREAM_STOP);
		break;
	case 0x03:
		app_state_post_event(APP_EVT_ALERT);
		audio_io_play_chime(chime_pcm, chime_pcm_samples);
		break;
	case 0x04:
		recall_status_update();
		break;
	case 0x05:
		if (len >= 1) {
			beat_secs = data[0];
		}
		break;
	}
}

static int enqueue_block(const uint8_t *block, size_t len)
{
	if (ringbuf_free() < len) {
		ringbuf_drop_oldest(len);
		app_state_buffer_loss_inc();
	}
	return ringbuf_put(block, len);
}

static void capture_fn(void *a, void *b, void *c)
{
	int16_t raw[320];
	int16_t half[160];
	int16_t block[PCM_PER_BLOCK];
	size_t fill = 0;

	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);

	codec_adpcm_reset_state(&adpcm);
	decimator_reset();

	for (;;) {
		task_wdt_mgr_feed(wdt_cap);

		if (!live) {
			k_sleep(K_MSEC(50));
			continue;
		}

		int n = audio_io_capture_read(raw, ARRAY_SIZE(raw), K_MSEC(200));
		if (n <= 0) {
			continue;
		}

		size_t got = decimator_process(raw, n, half, ARRAY_SIZE(half));
		for (size_t i = 0; i < got; i++) {
			block[fill++] = half[i];
			if (fill < PCM_PER_BLOCK) {
				continue;
			}

			uint8_t pkt[ADPCM_PKT];
			size_t enc = codec_adpcm_encode_block(block, PCM_PER_BLOCK, pkt, sizeof(pkt), &adpcm);

			if (enc) {
				enqueue_block(pkt, enc);
			}
			fill = 0;
		}
	}
}

static void ble_tx_fn(void *a, void *b, void *c)
{
	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);

	for (;;) {
		task_wdt_mgr_feed(wdt_tx);

		if (!live || !ble_conn_is_connected()) {
			k_sleep(K_MSEC(20));
			continue;
		}

		if (ringbuf_available() < ADPCM_PKT) {
			k_sleep(K_MSEC(5));
			continue;
		}

		notify_pkt[0] = seq;
		notify_pkt[1] = seq >> 8;
		ringbuf_get(&notify_pkt[2], ADPCM_PKT - 2);

		int err = recall_audio_notify_block(notify_pkt, ADPCM_PKT);
		if (err == -ENOMEM) {
			k_sleep(K_MSEC(10));
		} else if (!err) {
			seq++;
		}
	}
}
static void tick_fn(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(tick_work, tick_fn);

static void tick_fn(struct k_work *work)
{
	int32_t mv;
	uint8_t soc;

	ARG_UNUSED(work);
	task_wdt_mgr_feed(wdt_main);

	switch (charger_read_state()) {
	case CHARGER_CHARGING:
		app_state_post_event(APP_EVT_CHARGING);
		break;
	case CHARGER_COMPLETE:
		app_state_post_event(APP_EVT_CHARGE_COMPLETE);
		break;
	default:
		app_state_post_event(APP_EVT_NOT_CHARGING);
		break;
	}

	if (!battery_read_mv(&mv)) {
		battery_read_soc(&soc);
		battery_update_bas();
		if (mv < CONFIG_RECALL_CUTOFF_BATTERY_MV) {
			live = false;
			app_state_post_event(APP_EVT_LOW_BATTERY);
		} else if (mv < CONFIG_RECALL_LOW_BATTERY_MV) {
			app_state_post_event(APP_EVT_LOW_BATTERY);
		} else {
			app_state_post_event(APP_EVT_BATTERY_OK);
		}
	}

	if (!live && !ble_conn_is_connected()) {
		led_status_cycle_demo();
	}

	k_work_schedule(&tick_work, K_SECONDS(2));
}

static void confirm_image(void)
{
#if defined(CONFIG_MCUBOOT)
	if (!boot_is_img_confirmed()) {
		boot_write_img_confirmed();
	}
#endif
}

int main(void)
{
	LOG_INF("recall %d.%d.%d",
		CONFIG_RECALL_FW_VERSION_MAJOR,
		CONFIG_RECALL_FW_VERSION_MINOR,
		CONFIG_RECALL_FW_VERSION_PATCH);

	confirm_image();
	ringbuf_init(comp_buf, sizeof(comp_buf));
	task_wdt_mgr_init();
	wdt_main = task_wdt_mgr_add("main", K_SECONDS(30));
	wdt_cap = task_wdt_mgr_add("capture", K_SECONDS(5));
	wdt_tx = task_wdt_mgr_add("ble_tx", K_SECONDS(5));

	app_state_init(on_state);
	led_status_init();
	epd_ui_init();
	audio_io_init();
	decimator_init();
	codec_adpcm_init();
	battery_init();
	charger_init();
	pm_init();

	settings_load();
	ble_conn_init();
	ble_conn_set_event_callback(on_link);
	ble_audio_svc_init(on_ctrl);

	ble_conn_start_advertising();
	app_state_post_event(APP_EVT_BOOT_DONE);
	refresh_ui(APP_STATE_BOOT);

	k_work_schedule(&tick_work, K_SECONDS(1));

	k_thread_create(&cap_tid, cap_stack, K_THREAD_STACK_SIZEOF(cap_stack),
			capture_fn, NULL, NULL, NULL, 5, 0, K_NO_WAIT);
	k_thread_name_set(&cap_tid, "capture");

	k_thread_create(&tx_tid, tx_stack, K_THREAD_STACK_SIZEOF(tx_stack),
			ble_tx_fn, NULL, NULL, NULL, 6, 0, K_NO_WAIT);
	k_thread_name_set(&tx_tid, "ble_tx");

	for (;;) {
		task_wdt_mgr_feed(wdt_main);
		if (!live && !ble_conn_is_connected()) {
			pm_idle_until_next_heartbeat(beat_secs);
		} else {
			k_sleep(K_SECONDS(1));
		}
	}
}
