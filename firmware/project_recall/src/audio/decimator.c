#include "audio/decimator.h"

#include <stdint.h>

static const int16_t taps[] = { 8, 24, 40, 24, 8 };
#define NTAPS ARRAY_SIZE(taps)

static int32_t history[NTAPS];
static size_t head;

int decimator_init(void)
{
	decimator_reset();
	return 0;
}

void decimator_reset(void)
{
	for (size_t i = 0; i < NTAPS; i++) {
		history[i] = 0;
	}
	head = 0;
}

size_t decimator_process(const int16_t *in, size_t in_count, int16_t *out, size_t out_max)
{
	size_t n_out = 0;
	size_t skip = 0;

	for (size_t i = 0; i < in_count; i++) {
		history[head] = in[i];
		head = (head + 1) % NTAPS;

		if (skip++ & 1) {
			continue;
		}
		if (n_out >= out_max) {
			break;
		}

		int32_t sum = 0;
		size_t idx = head;

		for (size_t t = 0; t < NTAPS; t++) {
			idx = idx ? idx - 1 : NTAPS - 1;
			sum += history[idx] * taps[t];
		}

		sum >>= 8;
		if (sum > INT16_MAX) {
			sum = INT16_MAX;
		} else if (sum < INT16_MIN) {
			sum = INT16_MIN;
		}
		out[n_out++] = sum;
	}

	return n_out;
}
