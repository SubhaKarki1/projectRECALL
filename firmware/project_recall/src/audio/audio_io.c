#include "audio/audio_io.h"

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(audio_io, CONFIG_LOG_DEFAULT_LEVEL);

#define SAMPLE_RATE    CONFIG_RECALL_SAMPLE_RATE_HZ
#define FRAMES_PER_BLK ((SAMPLE_RATE * AUDIO_IO_BLOCK_MS) / 1000)
#define BLOCK_BYTES    (FRAMES_PER_BLK * sizeof(int32_t) * 2)

static const struct device *i2s = DEVICE_DT_GET(DT_NODELABEL(i2s0));
static const struct gpio_dt_spec amp = GPIO_DT_SPEC_GET(DT_NODELABEL(amp_en), gpios);

K_MEM_SLAB_DEFINE(rx_slab, BLOCK_BYTES, 4, 4);
K_MEM_SLAB_DEFINE(tx_slab, BLOCK_BYTES, 2, 4);

static bool capturing;
static uint32_t errors;

static struct i2s_config rx_cfg = {
	.word_size = 32,
	.channels = 2,
	.format = I2S_FMT_DATA_FORMAT_I2S,
	.options = I2S_OPT_BIT_CLK_MASTER | I2S_OPT_FRAME_CLK_MASTER,
	.frame_clk_freq = SAMPLE_RATE,
	.mem_slab = &rx_slab,
	.block_size = BLOCK_BYTES,
	.timeout = 2000,
};

static struct i2s_config tx_cfg = {
	.word_size = 32,
	.channels = 2,
	.format = I2S_FMT_DATA_FORMAT_I2S,
	.options = I2S_OPT_BIT_CLK_MASTER | I2S_OPT_FRAME_CLK_MASTER,
	.frame_clk_freq = SAMPLE_RATE,
	.mem_slab = &tx_slab,
	.block_size = BLOCK_BYTES,
	.timeout = 2000,
};

void audio_io_extract_left(const int32_t *raw, int16_t *pcm, size_t n)
{
	for (size_t i = 0; i < n; i++) {
		pcm[i] = (int16_t)(raw[i * 2] >> 16);
	}
}

void audio_io_amp_enable(bool on)
{
	if (device_is_ready(amp.port)) {
		gpio_pin_set_dt(&amp, on ? 1 : 0);
	}
}

int audio_io_init(void)
{
	if (!device_is_ready(i2s)) {
		LOG_ERR("no i2s");
		return -ENODEV;
	}

	if (gpio_is_ready_dt(&amp)) {
		gpio_pin_configure_dt(&amp, GPIO_OUTPUT_INACTIVE);
	}

	audio_io_amp_enable(false);
	return 0;
}

int audio_io_capture_start(void)
{
	int err;

	err = i2s_configure(i2s, I2S_DIR_RX, &rx_cfg);
	if (err) {
		return err;
	}

	err = i2s_trigger(i2s, I2S_DIR_RX, I2S_TRIGGER_START);
	if (err) {
		return err;
	}

	capturing = true;
	return 0;
}

int audio_io_capture_stop(void)
{
	int err = 0;

	if (!capturing) {
		return 0;
	}

	err = i2s_trigger(i2s, I2S_DIR_RX, I2S_TRIGGER_STOP);
	if (err) {
		errors++;
	}
	capturing = false;
	return err;
}

int audio_io_capture_read(int16_t *pcm, size_t samples, k_timeout_t timeout)
{
	void *block = NULL;
	size_t got = 0;
	int err;

	ARG_UNUSED(timeout);

	if (!capturing) {
		return -EAGAIN;
	}

	err = i2s_read(i2s, &block, &got);
	if (err) {
		errors++;
		i2s_trigger(i2s, I2S_DIR_RX, I2S_TRIGGER_STOP);
		i2s_trigger(i2s, I2S_DIR_RX, I2S_TRIGGER_PREPARE);
		i2s_trigger(i2s, I2S_DIR_RX, I2S_TRIGGER_START);
		return err;
	}

	audio_io_extract_left(block, pcm, MIN(samples, FRAMES_PER_BLK));
	k_mem_slab_free(&rx_slab, block);
	return MIN(samples, FRAMES_PER_BLK);
}

int audio_io_play_chime(const int16_t *pcm, size_t samples)
{
	int err;
	size_t off = 0;

	audio_io_capture_stop();

	err = i2s_configure(i2s, I2S_DIR_TX, &tx_cfg);
	if (err) {
		goto back_to_rx;
	}

	err = i2s_trigger(i2s, I2S_DIR_TX, I2S_TRIGGER_START);
	if (err) {
		goto back_to_rx;
	}

	audio_io_amp_enable(true);

	while (off < samples) {
		void *block = NULL;
		size_t chunk = MIN(FRAMES_PER_BLK, samples - off);
		size_t bytes = chunk * sizeof(int32_t) * 2;

		if (k_mem_slab_alloc(&tx_slab, &block, K_MSEC(100))) {
			break;
		}

		int32_t *out = block;
		for (size_t i = 0; i < chunk; i++) {
			int32_t s = ((int32_t)pcm[off + i]) << 16;
			out[i * 2] = s;
			out[i * 2 + 1] = 0;
		}

		err = i2s_write(i2s, block, bytes);
		if (err) {
			k_mem_slab_free(&tx_slab, block);
			break;
		}
		off += chunk;
	}

	i2s_trigger(i2s, I2S_DIR_TX, I2S_TRIGGER_DRAIN);
	audio_io_amp_enable(false);

back_to_rx:
	i2s_configure(i2s, I2S_DIR_RX, &rx_cfg);
	i2s_trigger(i2s, I2S_DIR_RX, I2S_TRIGGER_START);
	capturing = true;
	return err;
}

uint32_t audio_io_error_count_get(void)
{
	return errors;
}
