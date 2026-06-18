#include "ble/ble_conn.h"

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ble_conn, CONFIG_LOG_DEFAULT_LEVEL);

static struct bt_conn *conn;
static bool linked;
static ble_conn_event_cb_t on_link_change;

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME,
		sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

static const struct bt_data sd[] = {
	BT_DATA_BYTES(BT_DATA_UUID128_ALL,
		      0x9e, 0xca, 0xdc, 0x24, 0x0e, 0xe5, 0xa9, 0xe0,
		      0x93, 0xf3, 0xa3, 0xb5, 0x01, 0x00, 0x40, 0x6e),
};

static void tune_link(struct bt_conn *c)
{
	const struct bt_le_conn_param ci = {
		.interval_min = 0x0018,
		.interval_max = 0x0028,
		.latency = 0,
		.timeout = 400,
	};

	bt_conn_le_param_update(c, &ci);

	struct bt_conn_le_phy_param phy = {
		.options = BT_CONN_LE_OPT_NONE,
		.pref_tx_phy = BT_GAP_LE_PHY_2M,
		.pref_rx_phy = BT_GAP_LE_PHY_2M,
	};
	bt_conn_le_phy_update(c, &phy);

	struct bt_conn_le_data_len_param dle = {
		.tx_max_len = BT_GAP_DATA_LEN_MAX,
		.tx_max_time = BT_GAP_DATA_TIME_MAX,
	};
	bt_conn_le_data_len_update(c, &dle);
	bt_gatt_exchange_mtu(c);
}

static void connected_cb(struct bt_conn *c, uint8_t err)
{
	if (err) {
		LOG_ERR("connect err %u", err);
		return;
	}

	conn = bt_conn_ref(c);
	linked = true;
	if (on_link_change) {
		on_link_change(true);
	}
	tune_link(c);
}

static void disconnected_cb(struct bt_conn *c, uint8_t reason)
{
	LOG_INF("disconnect %u", reason);
	if (conn) {
		bt_conn_unref(conn);
		conn = NULL;
	}
	linked = false;
	if (on_link_change) {
		on_link_change(false);
	}
}

void ble_conn_set_event_callback(ble_conn_event_cb_t cb)
{
	on_link_change = cb;
}

BT_CONN_CB_DEFINE(conn_cbs) = {
	.connected = connected_cb,
	.disconnected = disconnected_cb,
};

int ble_conn_init(void)
{
	int err = bt_enable(NULL);

	if (err) {
		LOG_ERR("bt_enable %d", err);
	}
	return err;
}

void ble_conn_start_advertising(void)
{
	int err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));

	if (err) {
		LOG_ERR("adv %d", err);
	}
}

void ble_conn_stop_advertising(void)
{
	bt_le_adv_stop();
}

bool ble_conn_is_connected(void)
{
	return linked;
}
