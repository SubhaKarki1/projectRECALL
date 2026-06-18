#include "power/battery.h"

#include <zephyr/bluetooth/services/bas.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(battery, CONFIG_LOG_DEFAULT_LEVEL);

#if DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels)
#define ADC_NODE ADC_DT_SPEC_GET(DT_PATH(zephyr_user))
#else
#define ADC_NODE {0}
#endif

static const struct adc_dt_spec adc = ADC_NODE;

static const struct {
	int32_t mv;
	uint8_t pct;
} curve[] = {
	{ 4200, 100 }, { 4100, 90 }, { 4000, 80 }, { 3900, 70 },
	{ 3800, 60 }, { 3700, 50 }, { 3600, 40 }, { 3500, 30 },
	{ 3400, 20 }, { 3300, 10 }, { 3000, 0 },
};

static uint8_t cached_soc = 100;

int battery_init(void)
{
#if DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels)
	if (!adc_is_ready_dt(&adc)) {
		return -ENODEV;
	}
	return adc_channel_setup_dt(&adc);
#else
	return -ENODEV;
#endif
}

static void sort5(int16_t *s)
{
	for (int i = 0; i < 4; i++) {
		for (int j = i + 1; j < 5; j++) {
			if (s[i] > s[j]) {
				int16_t t = s[i];
				s[i] = s[j];
				s[j] = t;
			}
		}
	}
}

int battery_read_mv(int32_t *mv)
{
#if DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels)
	int16_t buf[5];
	int32_t pin_mv;
	struct adc_sequence seq = {
		.buffer = buf,
		.buffer_size = sizeof(buf),
		.calibrate = true,
	};

	adc_sequence_init_dt(&adc, &seq);
	if (adc_read_dt(&adc, &seq)) {
		return -EIO;
	}

	sort5(buf);
	pin_mv = buf[2];
	adc_raw_to_millivolts_dt(&adc, &pin_mv);
	*mv = pin_mv * CONFIG_RECALL_VBAT_DIVIDER_NUM / CONFIG_RECALL_VBAT_DIVIDER_DEN;
	return 0;
#else
	*mv = 3700;
	return -ENODEV;
#endif
}

static uint8_t interpolate_soc(int32_t mv)
{
	for (size_t i = 0; i < ARRAY_SIZE(curve) - 1; i++) {
		if (mv >= curve[i + 1].mv) {
			int32_t top = curve[i].mv;
			int32_t bot = curve[i + 1].mv;
			int32_t span = top - bot;

			if (span <= 0) {
				return curve[i].pct;
			}
			return curve[i + 1].pct +
			       (mv - bot) * (curve[i].pct - curve[i + 1].pct) / span;
		}
	}
	return 0;
}

int battery_read_soc(uint8_t *soc)
{
	int32_t mv;

	if (battery_read_mv(&mv)) {
		*soc = cached_soc;
		return -EIO;
	}

	cached_soc = interpolate_soc(mv);
	*soc = cached_soc;
	return 0;
}

void battery_update_bas(void)
{
	uint8_t pct;

	if (!battery_read_soc(&pct)) {
		bt_bas_set_battery_level(pct);
	}
}
