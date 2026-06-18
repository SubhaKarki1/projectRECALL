#pragma once

#include <zephyr/kernel.h>

int task_wdt_mgr_init(void);
int task_wdt_mgr_add(const char *name, k_timeout_t timeout);
void task_wdt_mgr_feed(int channel_id);
