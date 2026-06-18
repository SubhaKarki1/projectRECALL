#pragma once

#include <stddef.h>
#include <stdint.h>
#include <zephyr/kernel.h>

#define AUDIO_IO_BLOCK_MS 10

int audio_io_init(void);
int audio_io_capture_start(void);
int audio_io_capture_stop(void);
int audio_io_capture_read(int16_t *pcm, size_t samples, k_timeout_t timeout);
int audio_io_play_chime(const int16_t *pcm, size_t samples);
void audio_io_amp_enable(bool enable);
uint32_t audio_io_error_count_get(void);
void audio_io_extract_left(const int32_t *i2s_block, int16_t *pcm, size_t frames);
