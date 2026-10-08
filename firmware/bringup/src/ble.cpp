// BLE link test, based on Zephyr's peripheral_nus sample: advertise the
// Nordic UART Service, send a numbered heartbeat every second once a phone
// subscribes, and log whatever it writes. Disconnects are logged with
// the HCI reason (e.g. supervision timeout), so link drops while the motor
// runs show up on the console as well as in the phone's heartbeat stream.

#include "bringup.h"

#include <etl/array.h>
#include <etl/string.h>
#include <etl/to_string.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/services/nus.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>

LOG_MODULE_DECLARE(bringup);

namespace {

constexpr int heartbeat_period_ms = 1000;

const etl::array<bt_data, 2> advertising_data {
    bt_data BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    bt_data BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

const etl::array<bt_data, 1> scan_response_data {
    bt_data BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_NUS_SRV_VAL),
};

struct LinkState {
    bt_conn* connection;
    int64_t connected_at_ms;
    uint32_t connections;
    uint32_t disconnects;
    uint8_t last_disconnect_reason;
    bool notifications_enabled;
    uint32_t heartbeats_sent;
    uint32_t heartbeat_errors;
    uint32_t bytes_received;
};

LinkState link;

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

void send_heartbeat(k_work* work)
{
    if (link.notifications_enabled) {
        etl::string<20> message { "tick " };
        etl::to_string(link.heartbeats_sent + 1, message, true);
        message += '\n';
        // nullptr: notify every subscriber, so no connection pointer is shared
        // with the Bluetooth thread that handles disconnects.
        if (bt_nus_send(nullptr, message.data(), message.size())) {
            link.heartbeat_errors++;
        } else {
            link.heartbeats_sent++;
        }
    }
    k_work_schedule(k_work_delayable_from_work(work), K_MSEC(heartbeat_period_ms));
}

K_WORK_DELAYABLE_DEFINE(heartbeat_work, send_heartbeat);

void on_connected(bt_conn* connection, uint8_t error)
{
    if (error) {
        LOG_WRN("BLE: connection failed: %s (0x%02x)", bt_hci_err_to_str(error), error);
        return;
    }
    link.connection = bt_conn_ref(connection);
    link.connected_at_ms = k_uptime_get();
    link.connections++;
    LOG_INF("BLE: connected");
}

void on_disconnected(bt_conn*, uint8_t reason)
{
    link.disconnects++;
    link.last_disconnect_reason = reason;
    link.notifications_enabled = false;
    if (link.connection) {
        bt_conn_unref(link.connection);
        link.connection = nullptr;
    }
    LOG_WRN("BLE: disconnected after %lld s: %s (0x%02x)", (k_uptime_get() - link.connected_at_ms) / 1000,
        bt_hci_err_to_str(reason), reason);
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

void on_notifications_changed(bool enabled, void*)
{
    link.notifications_enabled = enabled;
    LOG_INF("BLE: heartbeat %s", enabled ? "started" : "stopped");
}

void on_received(bt_conn*, const void* data, uint16_t length, void*)
{
    link.bytes_received += length;
    LOG_INF("BLE: received \"%.*s\"", length, static_cast<const char*>(data));
}

bt_nus_cb nus_callbacks = {
    .notif_enabled = on_notifications_changed,
    .received = on_received,
};

int cmd_status(const shell* sh, size_t, char**)
{
    if (link.connection) {
        bt_conn_info info;
        bt_conn_get_info(link.connection, &info);
        shell_print(sh, "connected for %lld s, interval %u.%02u ms, latency %u, supervision timeout %u ms",
            (k_uptime_get() - link.connected_at_ms) / 1000, info.le.interval_us / 1000,
            (info.le.interval_us % 1000) / 10, info.le.latency, info.le.timeout * 10U);
    } else {
        shell_print(sh, "not connected (advertising as \"%s\")", CONFIG_BT_DEVICE_NAME);
    }
    shell_print(sh, "connections: %u, disconnects: %u, last reason: %s", link.connections, link.disconnects,
        link.disconnects ? bt_hci_err_to_str(link.last_disconnect_reason) : "-");
    shell_print(sh, "heartbeat: %s, sent %u, send errors %u; bytes received %u",
        link.notifications_enabled ? "on" : "off", link.heartbeats_sent, link.heartbeat_errors, link.bytes_received);
    return 0;
}

} // namespace

int ble_init()
{
    if (int ret = bt_nus_cb_register(&nus_callbacks, nullptr)) {
        return ret;
    }
    if (int ret = bt_enable(nullptr)) {
        return ret;
    }
    k_work_submit(&advertising_work);
    k_work_schedule(&heartbeat_work, K_MSEC(heartbeat_period_ms));
    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(ble_cmds,
    SHELL_CMD(status, NULL, "Show link state, connection parameters and drop statistics", cmd_status),
    SHELL_SUBCMD_SET_END);
SHELL_CMD_REGISTER(ble, &ble_cmds, "BLE link test (Nordic UART Service)", NULL);
