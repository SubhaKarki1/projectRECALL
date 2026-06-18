#pragma once

#include <stdbool.h>

int ble_conn_init(void);
void ble_conn_start_advertising(void);
void ble_conn_stop_advertising(void);
bool ble_conn_is_connected(void);

typedef void (*ble_conn_event_cb_t)(bool connected);
void ble_conn_set_event_callback(ble_conn_event_cb_t cb);
