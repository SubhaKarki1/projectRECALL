#pragma once

#include <stdint.h>

int battery_init(void);
int battery_read_mv(int32_t *mv);
int battery_read_soc(uint8_t *soc);
void battery_update_bas(void);
