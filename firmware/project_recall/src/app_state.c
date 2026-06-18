#include "app_state.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(app_state, CONFIG_LOG_DEFAULT_LEVEL);

static enum app_state current_state = APP_STATE_BOOT;
static app_state_listener_t state_listener;
static uint32_t buffer_loss_count;
static struct k_work state_work;
static enum app_event pending_event;
static K_MUTEX_DEFINE(state_lock);

static enum app_state map_event(enum app_state cur, enum app_event evt)
{
	switch (evt) {
	case APP_EVT_BOOT_DONE:
		return APP_STATE_ADVERTISING;
	case APP_EVT_BLE_CONNECTED:
		return APP_STATE_CONNECTED;
	case APP_EVT_BLE_DISCONNECTED:
		return APP_STATE_ADVERTISING;
	case APP_EVT_STREAM_START:
		return APP_STATE_STREAMING;
	case APP_EVT_STREAM_STOP:
		return APP_STATE_CONNECTED;
	case APP_EVT_ALERT:
		return APP_STATE_ALERT;
	case APP_EVT_CHARGING:
		return APP_STATE_CHARGING;
	case APP_EVT_CHARGE_COMPLETE:
		return APP_STATE_CHARGE_COMPLETE;
	case APP_EVT_NOT_CHARGING:
		if (cur == APP_STATE_CHARGING || cur == APP_STATE_CHARGE_COMPLETE) {
			return APP_STATE_ADVERTISING;
		}
		return cur;
	case APP_EVT_LOW_BATTERY:
		return APP_STATE_LOW_BATTERY;
	case APP_EVT_BATTERY_OK:
		if (cur == APP_STATE_LOW_BATTERY) {
			return APP_STATE_ADVERTISING;
		}
		return cur;
	case APP_EVT_FAULT:
		return APP_STATE_FAULT;
	default:
		return cur;
	}
}

static void state_work_handler(struct k_work *work)
{
	enum app_state prev;
	enum app_event evt;

	ARG_UNUSED(work);

	k_mutex_lock(&state_lock, K_FOREVER);
	evt = pending_event;
	prev = current_state;
	current_state = map_event(current_state, evt);
	k_mutex_unlock(&state_lock);

	if (prev != current_state && state_listener) {
		LOG_INF("%d -> %d", prev, current_state);
		state_listener(prev, current_state);
	}
}

int app_state_init(app_state_listener_t listener)
{
	state_listener = listener;
	k_work_init(&state_work, state_work_handler);
	return 0;
}

void app_state_post_event(enum app_event evt)
{
	k_mutex_lock(&state_lock, K_FOREVER);
	pending_event = evt;
	k_mutex_unlock(&state_lock);
	k_work_submit(&state_work);
}

enum app_state app_state_current(void)
{
	enum app_state s;

	k_mutex_lock(&state_lock, K_FOREVER);
	s = current_state;
	k_mutex_unlock(&state_lock);
	return s;
}

bool app_state_is_streaming(void)
{
	return app_state_current() == APP_STATE_STREAMING;
}

uint32_t app_state_buffer_loss_get(void)
{
	return buffer_loss_count;
}

void app_state_buffer_loss_inc(void)
{
	buffer_loss_count++;
}
