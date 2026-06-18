#pragma once

#include <stddef.h>
#include <stdint.h>

typedef void (*recall_control_handler_t)(uint8_t cmd, const uint8_t *data, size_t len);

#define RECALL_STATUS_SIZE 16

int ble_audio_svc_init(recall_control_handler_t handler);
int recall_audio_notify_block(const uint8_t *data, size_t len);
void recall_status_update(void);
void recall_status_set_field(size_t offset, uint8_t value);
