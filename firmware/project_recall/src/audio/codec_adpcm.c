#include "audio/codec_adpcm.h"

static const int16_t steps[89] = {
	7, 8, 9, 10, 12, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
	50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143, 157, 173, 190, 209,
	230, 253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
	876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499,
	2750, 3024, 3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845,
	8630, 9493, 10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350,
	22385, 24623, 27086, 29794, 32767
};

static const int8_t index_lut[16] = {
	-1, -1, -1, -1, 2, 4, 6, 8,
	-1, -1, -1, -1, 2, 4, 6, 8
};

int codec_adpcm_init(void)
{
	return 0;
}

void codec_adpcm_reset_state(struct adpcm_state *st)
{
	st->predictor = 0;
	st->step_index = 0;
}

static int8_t clamp_idx(int32_t i)
{
	if (i < 0) {
		return 0;
	}
	if (i > 88) {
		return 88;
	}
	return i;
}

static uint8_t encode_nibble(struct adpcm_state *st, int16_t sample)
{
	int32_t step = steps[st->step_index];
	int32_t diff = sample - st->predictor;
	uint8_t nib;
	int32_t delta;

	if (diff < 0) {
		nib = 8;
		diff = -diff;
	} else {
		nib = 0;
	}

	delta = step >> 3;
	if (diff >= step) {
		nib |= 4;
		diff -= step;
		delta += step;
	}
	step >>= 1;
	if (diff >= step) {
		nib |= 2;
		diff -= step;
		delta += step;
	}
	step >>= 1;
	if (diff >= step) {
		nib |= 1;
		delta += step;
	}

	if (nib & 8) {
		st->predictor -= delta;
	} else {
		st->predictor += delta;
	}

	if (st->predictor > INT16_MAX) {
		st->predictor = INT16_MAX;
	} else if (st->predictor < INT16_MIN) {
		st->predictor = INT16_MIN;
	}

	st->step_index = clamp_idx(st->step_index + index_lut[nib]);
	return nib;
}

size_t codec_adpcm_encode_block(const int16_t *pcm, size_t samples,
				uint8_t *out, size_t out_max,
				struct adpcm_state *st)
{
	size_t pos = 0;

	if (out_max < ADPCM_BLOCK_HEADER_SIZE + (samples + 1) / 2) {
		return 0;
	}

	out[pos++] = st->predictor & 0xff;
	out[pos++] = (st->predictor >> 8) & 0xff;
	out[pos++] = st->step_index;
	out[pos++] = 0;

	for (size_t i = 0; i < samples; i += 2) {
		uint8_t lo = encode_nibble(st, pcm[i]);
		uint8_t hi = 0;

		if (i + 1 < samples) {
			hi = encode_nibble(st, pcm[i + 1]);
		}
		out[pos++] = (hi << 4) | (lo & 0x0f);
	}

	return pos;
}
