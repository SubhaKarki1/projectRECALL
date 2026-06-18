#pragma once

#include <stddef.h>
#include <stdint.h>

#define ADPCM_BLOCK_HEADER_SIZE 4

struct adpcm_state {
	int16_t predictor;
	int8_t step_index;
};

int codec_adpcm_init(void);
size_t codec_adpcm_encode_block(const int16_t *pcm, size_t samples,
				uint8_t *out, size_t out_max,
				struct adpcm_state *state);
void codec_adpcm_reset_state(struct adpcm_state *state);
