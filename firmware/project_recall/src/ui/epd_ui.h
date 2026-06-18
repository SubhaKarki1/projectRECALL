#pragma once

#include "app_state.h"

int epd_ui_init(void);
void epd_ui_apply_state(enum app_state state);
bool epd_ui_is_available(void);
