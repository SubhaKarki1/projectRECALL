#include "ble/ble_audio_svc.h"

#include "app_state.h"

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ble_audio_svc, CONFIG_LOG_DEFAULT_LEVEL);

#define BT_UUID_RECALL_SVC_VAL \
	BT_UUID_128_ENCODE(0x6e400001, 0xb5a3, 0xf393, 0xe0a9, 0xe50e24dcca9e)
#define BT_UUID_RECALL_AUDIO_VAL \
	BT_UUID_128_ENCODE(0x6e400002, 0xb5a3, 0xf393, 0xe0a9, 0xe50e24dcca9e)
#define BT_UUID_RECALL_CTRL_VAL \
	BT_UUID_128_ENCODE(0x6e400003, 0xb5a3, 0xf393, 0xe0a9, 0xe50e24dcca9e)
#define BT_UUID_RECALL_STATUS_VAL \
	BT_UUID_128_ENCODE(0x6e400004, 0xb5a3, 0xf393, 0xe0a9, 0xe50e24dcca9e)

static struct bt_uuid_128 svc_uuid = BT_UUID_INIT_128(BT_UUID_RECALL_SVC_VAL);
static struct bt_uuid_128 audio_uuid = BT_UUID_INIT_128(BT_UUID_RECALL_AUDIO_VAL);
static struct bt_uuid_128 ctrl_uuid = BT_UUID_INIT_128(BT_UUID_RECALL_CTRL_VAL);
static struct bt_uuid_128 status_uuid = BT_UUID_INIT_128(BT_UUID_RECALL_STATUS_VAL);

static recall_control_handler_t on_ctrl;
static bool audio_notify_on;
static bool status_notify_on;
static uint8_t status[RECALL_STATUS_SIZE];

static void fill_status_defaults(void)
{
	status[0] = APP_STATE_ADVERTISING;
	status[1] = 0;
	status[2] = 0;
	status[3] = 1;
	status[4] = CONFIG_RECALL_OUTPUT_RATE_HZ >> 8;
	status[5] = CONFIG_RECALL_OUTPUT_RATE_HZ & 0xff;
	status[6] = CONFIG_RECALL_FW_VERSION_MAJOR;
	status[7] = CONFIG_RECALL_FW_VERSION_MINOR;
	status[8] = CONFIG_RECALL_FW_VERSION_PATCH;
}

static ssize_t ctrl_write(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			  const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	const uint8_t *b = buf;

	ARG_UNUSED(conn);
	ARG_UNUSED(attr);
	ARG_UNUSED(offset);
	ARG_UNUSED(flags);

	if (len < 1 || !on_ctrl) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	on_ctrl(b[0], &b[1], len - 1);
	return len;
}

static void audio_ccc(const struct bt_gatt_attr *attr, uint16_t value)
{
	ARG_UNUSED(attr);
	audio_notify_on = (value == BT_GATT_CCC_NOTIFY);
}

static void status_ccc(const struct bt_gatt_attr *attr, uint16_t value)
{
	ARG_UNUSED(attr);
	status_notify_on = (value == BT_GATT_CCC_NOTIFY);
}

BT_GATT_SERVICE_DEFINE(recall_svc,
	BT_GATT_PRIMARY_SERVICE(&svc_uuid),
	BT_GATT_CHARACTERISTIC(&audio_uuid.uuid,
			       BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_NONE, NULL, NULL, NULL),
	BT_GATT_CCC(audio_ccc, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
	BT_GATT_CHARACTERISTIC(&ctrl_uuid.uuid,
			       BT_GATT_CHRC_WRITE | BT_GATT_CHRC_WRITE_WITHOUT_RESP,
			       BT_GATT_PERM_WRITE, NULL, ctrl_write, NULL),
	BT_GATT_CHARACTERISTIC(&status_uuid.uuid,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ, bt_gatt_attr_read, NULL, status),
	BT_GATT_CCC(status_ccc, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);

int ble_audio_svc_init(recall_control_handler_t handler)
{
	on_ctrl = handler;
	fill_status_defaults();
	return 0;
}

int recall_audio_notify_block(const uint8_t *data, size_t len)
{
	struct bt_gatt_notify_params p;

	if (!audio_notify_on) {
		return -ENOTCONN;
	}

	p.attr = &recall_svc.attrs[1];
	p.data = data;
	p.len = len;
	p.func = NULL;

	return bt_gatt_notify_cb(NULL, &p);
}

void recall_status_set_field(size_t offset, uint8_t value)
{
	if (offset < RECALL_STATUS_SIZE) {
		status[offset] = value;
	}
}

void recall_status_update(void)
{
	uint32_t loss = app_state_buffer_loss_get();

	status[0] = app_state_current();
	status[1] = loss & 0xff;
	status[2] = (loss >> 8) & 0xff;

	if (status_notify_on) {
		bt_gatt_notify(NULL, &recall_svc.attrs[5], status, RECALL_STATUS_SIZE);
	}
}
