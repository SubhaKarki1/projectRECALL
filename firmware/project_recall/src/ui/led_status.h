#pragma once

#include "app_state.h"

int led_status_init(void);
void led_status_apply_state(enum app_state state);
void led_status_cycle_demo(void);
