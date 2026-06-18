#include "power/pm.h"

#include <zephyr/kernel.h>

static bool streaming_burst;
static uint32_t beat_secs = CONFIG_RECALL_HEARTBEAT_INTERVAL_SEC;

int pm_init(void)
{
	return 0;
}

void pm_heartbeat_begin(void)
{
	streaming_burst = true;
}

void pm_heartbeat_end(void)
{
	streaming_burst = false;
}

void pm_idle_until_next_heartbeat(uint32_t secs)
{
	if (secs) {
		beat_secs = secs;
	}
	k_sleep(K_SECONDS(beat_secs));
}

bool pm_allow_ui_refresh(void)
{
	return !streaming_burst;
}
