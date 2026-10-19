#ifndef ZEPHYR_NUS_STREAM_H
#define ZEPHYR_NUS_STREAM_H

#include "IRobotIOStream.h"

#include <etl/circular_buffer.h>
#include <etl/vector.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/services/nus.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>

/**
 * IRobotIOStream over the Nordic UART Service: ovladacka commands arrive on
 * the RX characteristic, telemetry packets leave as TX notifications.
 *
 * Received bytes are buffered for the control thread, which drains them with
 * available()/read(); each arrival also posts an event so the thread wakes up.
 *
 * Written bytes are collected and sent as few, MTU-sized notifications: a
 * notification goes out when the next write would not fit, and flush() sends
 * the rest. The control thread calls flush() at the end of every wake, so a
 * whole tick's telemetry usually leaves as one notification. ovladacka parses
 * a byte stream, so packet boundaries need not match notifications. Data is
 * dropped while no phone is subscribed.
 */
class ZephyrNusStream : public IRobotIOStream {
    // ovladacka commands are 3 bytes; this holds a burst of them. When full,
    // the oldest bytes are overwritten and the parser resynchronizes.
    static constexpr size_t RX_BUFFER_SIZE = 64;
    // Largest notification payload: the 247-byte ATT MTU prj.conf allows,
    // less the 3-byte notification header.
    static constexpr size_t TX_BUFFER_SIZE = 244;
    // Payload of the default 23-byte ATT MTU, until the phone negotiates more.
    static constexpr uint16_t DEFAULT_PAYLOAD = 20;

    k_event& events;
    uint32_t command_event;

    k_spinlock lock { };
    etl::circular_buffer<uint8_t, RX_BUFFER_SIZE> rx_buffer;
    atomic_t notifications_enabled = ATOMIC_INIT(0);
    bt_nus_cb nus_callbacks { };

    // Only touched by the control thread (write() and flush()).
    etl::vector<uint8_t, TX_BUFFER_SIZE> tx_buffer;
    // Notification payload the current connection allows, set from the
    // Bluetooth thread when the ATT MTU changes.
    atomic_t max_payload = ATOMIC_INIT(DEFAULT_PAYLOAD);
    bt_gatt_cb gatt_callbacks { };

    static void on_received(bt_conn* conn, const void* data, uint16_t length, void* context);
    static void on_notifications_changed(bool enabled, void* context);
    static void on_mtu_updated(bt_conn* conn, uint16_t tx, uint16_t rx);

    bool send(etl::span<const uint8_t> data);

public:
    /**
     * @param[in] events event object the control thread waits on
     * @param[in] command_event event bit posted whenever bytes arrive
     */
    ZephyrNusStream(k_event& events, uint32_t command_event)
        : events(events)
        , command_event(command_event)
    {
    }

    /**
     * Enable Bluetooth, register the NUS callbacks and start advertising as
     * CONFIG_BT_DEVICE_NAME. Advertising restarts after every disconnect.
     * @return 0 on success, a negative errno otherwise
     */
    int init();

    int available() override;

    /**
     * @return the next received byte, or -1 if there is none (as Arduino's
     * Stream::read(), which v1 used)
     */
    int read() override;

    size_t write(uint8_t c) override;

    /**
     * Queue data for sending; sends the queued data first if data would not
     * fit in the same notification.
     * @return data.size() once queued, 0 if nobody is subscribed (the data is
     * dropped)
     */
    size_t write(etl::span<const uint8_t> data) override;

    /**
     * Send the queued data as one notification. Called by the control thread
     * at the end of every wake.
     * @return true if the data was sent or nothing was queued
     */
    bool flush();

    /**
     * Fall back to the default notification payload, for a new connection.
     */
    void reset_max_payload() { atomic_set(&max_payload, DEFAULT_PAYLOAD); }
};

#endif // ZEPHYR_NUS_STREAM_H
