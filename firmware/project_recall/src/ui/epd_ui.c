#include "ui/epd_ui.h"

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(epd_ui, CONFIG_LOG_DEFAULT_LEVEL);

#if DT_HAS_CHOSEN(zephyr_display)
static const struct device *panel = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
#define HAS_PANEL 1
#else
static const struct device *panel;
#define HAS_PANEL 0
#endif

static enum app_state last_state = APP_STATE_BOOT;
static uint8_t refresh_count;

static const char *label_for(enum app_state s)
{
	switch (s) {
	case APP_STATE_BOOT: return "Recall boot";
	case APP_STATE_ADVERTISING: return "Searching...";
	case APP_STATE_CONNECTED: return "Connected";
	case APP_STATE_STREAMING: return "Streaming";
	case APP_STATE_ALERT: return "Alert!";
	case APP_STATE_CHARGING: return "Charging";
	case APP_STATE_CHARGE_COMPLETE: return "Charged";
	case APP_STATE_LOW_BATTERY: return "Low battery";
	case APP_STATE_FAULT: return "Fault";
	default: return "";
	}
}

bool epd_ui_is_available(void)
{
#if HAS_PANEL
	return device_is_ready(panel);
#else
	return false;
#endif
}

int epd_ui_init(void)
{
#if HAS_PANEL
	if (!device_is_ready(panel)) {
		return -ENODEV;
	}
	display_blanking_off(panel);
#endif
	return 0;
}

void epd_ui_apply_state(enum app_state state)
{
#if HAS_PANEL
	struct display_buffer_descriptor desc;
	uint8_t row[25];
	const char *text;

	if (!device_is_ready(panel)) {
		return;
	}

	if (state == APP_STATE_STREAMING && last_state == APP_STATE_STREAMING) {
		return;
	}

	if (state == last_state && (refresh_count % 10) != 0) {
		return;
	}

	text = label_for(state);
	memset(row, 0, sizeof(row));
	for (size_t i = 0; text[i] && i < sizeof(row); i++) {
		row[i % sizeof(row)] |= 1 << (i % 8);
	}

	desc.buf_size = sizeof(row);
	desc.width = 200;
	desc.height = 8;
	desc.pitch = sizeof(row);
	desc.frame_incomplete = false;

	display_write(panel, 0, 0, &desc, row);
	last_state = state;
	refresh_count++;
#else
	ARG_UNUSED(state);
#endif
}
