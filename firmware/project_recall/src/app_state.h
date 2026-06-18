#pragma once

enum app_state {
	APP_STATE_BOOT = 0,
	APP_STATE_ADVERTISING,
	APP_STATE_CONNECTED,
	APP_STATE_STREAMING,
	APP_STATE_ALERT,
	APP_STATE_CHARGING,
	APP_STATE_CHARGE_COMPLETE,
	APP_STATE_LOW_BATTERY,
	APP_STATE_FAULT,
};

enum app_event {
	APP_EVT_BOOT_DONE = 0,
	APP_EVT_BLE_CONNECTED,
	APP_EVT_BLE_DISCONNECTED,
	APP_EVT_STREAM_START,
	APP_EVT_STREAM_STOP,
	APP_EVT_ALERT,
	APP_EVT_CHARGING,
	APP_EVT_CHARGE_COMPLETE,
	APP_EVT_NOT_CHARGING,
	APP_EVT_LOW_BATTERY,
	APP_EVT_BATTERY_OK,
	APP_EVT_FAULT,
};

typedef void (*app_state_listener_t)(enum app_state prev, enum app_state next);

int app_state_init(app_state_listener_t listener);
void app_state_post_event(enum app_event evt);
enum app_state app_state_current(void);
bool app_state_is_streaming(void);
uint32_t app_state_buffer_loss_get(void);
void app_state_buffer_loss_inc(void);
