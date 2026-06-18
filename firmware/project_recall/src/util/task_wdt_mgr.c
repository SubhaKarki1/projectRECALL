#include "util/task_wdt_mgr.h"

#include <zephyr/task_wdt/task_wdt.h>

#define MAX_CH 4

static int ch_ids[MAX_CH];
static int n_ch;

int task_wdt_mgr_init(void)
{
	return task_wdt_init(NULL);
}

int task_wdt_mgr_add(const char *name, k_timeout_t timeout)
{
	int id;

	if (n_ch >= MAX_CH) {
		return -ENOMEM;
	}

	id = task_wdt_add(timeout.ticks, NULL, (void *)name);
	if (id < 0) {
		return id;
	}

	ch_ids[n_ch++] = id;
	return id;
}

void task_wdt_mgr_feed(int channel_id)
{
	task_wdt_feed(channel_id);
}
