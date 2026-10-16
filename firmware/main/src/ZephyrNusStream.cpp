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

int ZephyrNusStream::init()
{
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
        return 0;
    }
    // NULL: notify every subscribed connection; there is at most one.
    if (bt_nus_send(nullptr, data.data(), static_cast<uint16_t>(data.size())) != 0) {
        return 0;
    }
    return data.size();
}
