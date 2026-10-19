#include "ZephyrNusStream.h"

#include <etl/array.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(wallckr);

namespace {

const etl::array<bt_data, 2> advertising_data {
    bt_data BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    bt_data BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

const etl::array<bt_data, 1> scan_response_data {
    bt_data BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_NUS_SRV_VAL),
};

void start_advertising(k_work*)
{
    if (int ret = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, advertising_data.data(), advertising_data.size(),
            scan_response_data.data(), scan_response_data.size())) {
        LOG_ERR("BLE: advertising failed to start (%d)", ret);
        return;
    }
    LOG_INF("BLE: advertising as \"%s\"", CONFIG_BT_DEVICE_NAME);
}

K_WORK_DEFINE(advertising_work, start_advertising);

// The stream that the GATT MTU callback, which has no context pointer,
// reports to. There is one NUS stream.
ZephyrNusStream* mtu_listener = nullptr;

void on_connected(bt_conn*, uint8_t error)
{
    if (error) {
        LOG_WRN("BLE: connection failed (0x%02x)", error);
        return;
    }
    LOG_INF("BLE: connected");
}

void on_disconnected(bt_conn*, uint8_t reason)
{
    LOG_INF("BLE: disconnected (0x%02x)", reason);
    // The next connection starts at the default ATT MTU again.
    if (mtu_listener) {
        mtu_listener->reset_max_payload();
    }
}

// Advertising stops on connection; restart it once the connection object is
// free again, as the Zephyr samples do.
void on_recycled()
{
    k_work_submit(&advertising_work);
}

BT_CONN_CB_DEFINE(connection_callbacks) = {
    .connected = on_connected,
    .disconnected = on_disconnected,
    .recycled = on_recycled,
};

} // namespace

void ZephyrNusStream::on_received(bt_conn*, const void* data, uint16_t length, void* context)
{
    auto* stream = static_cast<ZephyrNusStream*>(context);
    etl::span<const uint8_t> bytes(static_cast<const uint8_t*>(data), length);

    k_spinlock_key_t key = k_spin_lock(&stream->lock);
    for (auto byte : bytes) {
        stream->rx_buffer.push(byte);
    }
    k_spin_unlock(&stream->lock, key);

    k_event_post(&stream->events, stream->command_event);
}

void ZephyrNusStream::on_notifications_changed(bool enabled, void* context)
{
    auto* stream = static_cast<ZephyrNusStream*>(context);
    atomic_set(&stream->notifications_enabled, enabled ? 1 : 0);
    LOG_INF("BLE: telemetry %s", enabled ? "on" : "off");
}

void ZephyrNusStream::on_mtu_updated(bt_conn*, uint16_t tx, uint16_t)
{
    // A notification carries ATT MTU - 3 bytes; never more than tx_buffer.
    constexpr uint16_t NOTIFICATION_HEADER = 3;
    uint16_t payload = tx > NOTIFICATION_HEADER ? tx - NOTIFICATION_HEADER : DEFAULT_PAYLOAD;
    if (payload > TX_BUFFER_SIZE) {
        payload = TX_BUFFER_SIZE;
    }
    if (mtu_listener) {
        atomic_set(&mtu_listener->max_payload, payload);
    }
    LOG_INF("BLE: notification payload %u bytes", payload);
}

int ZephyrNusStream::init()
{
    mtu_listener = this;
    gatt_callbacks.att_mtu_updated = on_mtu_updated;
    bt_gatt_cb_register(&gatt_callbacks);

    nus_callbacks.notif_enabled = on_notifications_changed;
    nus_callbacks.received = on_received;
    if (int ret = bt_nus_cb_register(&nus_callbacks, this)) {
        return ret;
    }
    if (int ret = bt_enable(nullptr)) {
        return ret;
    }
    k_work_submit(&advertising_work);
    return 0;
}

int ZephyrNusStream::available()
{
    k_spinlock_key_t key = k_spin_lock(&lock);
    auto count = static_cast<int>(rx_buffer.size());
    k_spin_unlock(&lock, key);
    return count;
}

int ZephyrNusStream::read()
{
    int byte = -1;

    k_spinlock_key_t key = k_spin_lock(&lock);
    if (!rx_buffer.empty()) {
        byte = rx_buffer.front();
        rx_buffer.pop();
    }
    k_spin_unlock(&lock, key);

    return byte;
}

size_t ZephyrNusStream::write(uint8_t c)
{
    return write(etl::span<const uint8_t>(&c, 1));
}

size_t ZephyrNusStream::write(etl::span<const uint8_t> data)
{
    if (data.empty() || !atomic_get(&notifications_enabled)) {
        tx_buffer.clear();
        return 0;
    }

    auto limit = static_cast<size_t>(atomic_get(&max_payload));
    if (tx_buffer.size() + data.size() > limit) {
        flush();
    }
    if (data.size() > limit) {
        // Too long for any notification on this connection; send it alone
        // and let the stack report the error. Telemetry packets (10 and 16
        // bytes) always fit the default 20-byte payload.
        return send(data) ? data.size() : 0;
    }

    tx_buffer.insert(tx_buffer.end(), data.begin(), data.end());
    return data.size();
}

bool ZephyrNusStream::flush()
{
    if (tx_buffer.empty()) {
        return true;
    }
    bool sent = atomic_get(&notifications_enabled) && send(tx_buffer);
    tx_buffer.clear();
    return sent;
}

bool ZephyrNusStream::send(etl::span<const uint8_t> data)
{
    // NULL: notify every subscribed connection; there is at most one.
    return bt_nus_send(nullptr, data.data(), static_cast<uint16_t>(data.size())) == 0;
}
