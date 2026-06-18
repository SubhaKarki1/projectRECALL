#include "util/ringbuf.h"

#include <zephyr/kernel.h>

static uint8_t *mem;
static size_t cap;
static size_t w;
static size_t r;
static size_t used;
static struct k_mutex lock;

int ringbuf_init(uint8_t *storage, size_t capacity)
{
	if (!storage || !capacity) {
		return -EINVAL;
	}

	k_mutex_init(&lock);
	mem = storage;
	cap = capacity;
	w = r = used = 0;
	return 0;
}

static int push_byte(uint8_t b)
{
	if (used >= cap) {
		return -ENOSPC;
	}
	mem[w] = b;
	w = (w + 1) % cap;
	used++;
	return 0;
}

static int pop_byte(uint8_t *b)
{
	if (!used) {
		return -EAGAIN;
	}
	*b = mem[r];
	r = (r + 1) % cap;
	used--;
	return 0;
}

int ringbuf_put(const uint8_t *data, size_t len)
{
	int err = 0;

	k_mutex_lock(&lock, K_FOREVER);
	for (size_t i = 0; i < len; i++) {
		err = push_byte(data[i]);
		if (err) {
			break;
		}
	}
	k_mutex_unlock(&lock);
	return err;
}

int ringbuf_get(uint8_t *data, size_t len)
{
	int err = 0;

	k_mutex_lock(&lock, K_FOREVER);
	for (size_t i = 0; i < len; i++) {
		err = pop_byte(&data[i]);
		if (err) {
			break;
		}
	}
	k_mutex_unlock(&lock);
	return err;
}

size_t ringbuf_available(void)
{
	size_t n;

	k_mutex_lock(&lock, K_FOREVER);
	n = used;
	k_mutex_unlock(&lock);
	return n;
}

size_t ringbuf_free(void)
{
	return cap - ringbuf_available();
}

void ringbuf_reset(void)
{
	k_mutex_lock(&lock, K_FOREVER);
	w = r = used = 0;
	k_mutex_unlock(&lock);
}

int ringbuf_drop_oldest(size_t len)
{
	uint8_t junk[64];
	size_t dropped = 0;

	k_mutex_lock(&lock, K_FOREVER);
	while (dropped < len && used) {
		size_t chunk = MIN(sizeof(junk), len - dropped);
		for (size_t i = 0; i < chunk && used; i++) {
			pop_byte(&junk[i]);
			dropped++;
		}
	}
	k_mutex_unlock(&lock);
	return dropped == len ? 0 : -EAGAIN;
}
