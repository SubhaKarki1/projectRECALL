#pragma once

#include <stdbool.h>
#include <stdint.h>

int pm_init(void);
void pm_heartbeat_begin(void);
void pm_heartbeat_end(void);
void pm_idle_until_next_heartbeat(uint32_t interval_sec);
bool pm_allow_ui_refresh(void);
