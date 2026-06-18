#pragma once

#include <stddef.h>
#include <stdint.h>

int decimator_init(void);
void decimator_reset(void);
size_t decimator_process(const int16_t *in, size_t in_count, int16_t *out, size_t out_max);
