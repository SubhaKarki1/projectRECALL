#pragma once

#include <stddef.h>
#include <stdint.h>

int ringbuf_init(uint8_t *storage, size_t capacity);
int ringbuf_put(const uint8_t *data, size_t len);
int ringbuf_get(uint8_t *data, size_t len);
size_t ringbuf_available(void);
size_t ringbuf_free(void);
void ringbuf_reset(void);
int ringbuf_drop_oldest(size_t len);
