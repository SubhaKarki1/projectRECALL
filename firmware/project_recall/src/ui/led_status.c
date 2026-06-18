#include "ui/led_status.h"

#include <zephyr/device.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(led_status, CONFIG_LOG_DEFAULT_LEVEL);

#if DT_HAS_ALIAS(led_strip)
static const struct device *strip = DEVICE_DT_GET(DT_ALIAS(led_strip));
#else
static const struct device *strip;
#endif

static struct led_rgb pixel;

static int write_rgb(uint8_t r, uint8_t g, uint8_t b)
{
#if DT_HAS_ALIAS(led_strip)
	if (!device_is_ready(strip)) {
		return -ENODEV;
	}
	pixel.r = r;
	pixel.g = g;
	pixel.b = b;
	return led_strip_update_rgb(strip, &pixel, 1);
#else
	ARG_UNUSED(r);
	ARG_UNUSED(g);
	ARG_UNUSED(b);
	return -ENODEV;
#endif
}

int led_status_init(void)
{
#if DT_HAS_ALIAS(led_strip)
	if (!device_is_ready(strip)) {
		return -ENODEV;
	}
#endif
	return write_rgb(0, 0, 0);
}

void led_status_cycle_demo(void)
{
	static uint8_t step;

	switch (step++ % 3) {
	case 0:
		write_rgb(32, 0, 0);
		break;
	case 1:
		write_rgb(0, 32, 0);
		break;
	default:
		write_rgb(0, 0, 32);
		break;
	}
}

void led_status_apply_state(enum app_state state)
{
	switch (state) {
	case APP_STATE_BOOT:
		write_rgb(16, 16, 16);
		break;
	case APP_STATE_ADVERTISING:
		write_rgb(0, 0, 32);
		break;
	case APP_STATE_CONNECTED:
		write_rgb(0, 16, 32);
		break;
	case APP_STATE_STREAMING:
		write_rgb(0, 32, 32);
		break;
	case APP_STATE_ALERT:
		write_rgb(32, 16, 0);
		break;
	case APP_STATE_CHARGING:
		write_rgb(32, 0, 0);
		break;
	case APP_STATE_CHARGE_COMPLETE:
		write_rgb(0, 32, 0);
		break;
	case APP_STATE_LOW_BATTERY:
		write_rgb(32, 8, 0);
		break;
	case APP_STATE_FAULT:
		write_rgb(32, 0, 0);
		break;
	default:
		write_rgb(0, 0, 0);
		break;
	}
}
