#include "power/charger.h"

#include <zephyr/drivers/gpio.h>

#define STAT_PIN 26

static const struct device *gpio0;

int charger_init(void)
{
	gpio0 = DEVICE_DT_GET(DT_NODELABEL(gpio0));
	if (!device_is_ready(gpio0)) {
		return -ENODEV;
	}
	return gpio_pin_configure(gpio0, STAT_PIN, GPIO_INPUT);
}

enum charger_state charger_read_state(void)
{
	int pulled_high;
	int pulled_low;

	if (!gpio0) {
		return CHARGER_NOT_CHARGING;
	}

	gpio_pin_configure(gpio0, STAT_PIN, GPIO_INPUT | GPIO_PULL_UP);
	pulled_high = gpio_pin_get(gpio0, STAT_PIN);

	gpio_pin_configure(gpio0, STAT_PIN, GPIO_INPUT | GPIO_PULL_DOWN);
	pulled_low = gpio_pin_get(gpio0, STAT_PIN);

	if (pulled_high == 1 && pulled_low == 0) {
		return CHARGER_NOT_CHARGING;
	}
	if (pulled_high == 0 && pulled_low == 0) {
		return CHARGER_CHARGING;
	}
	return CHARGER_COMPLETE;
}
