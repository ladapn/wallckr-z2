#ifndef ZEPHYR_NUS_STREAM_H
#define ZEPHYR_NUS_STREAM_H

#include "IRobotIOStream.h"

#include <etl/circular_buffer.h>
#include <zephyr/bluetooth/services/nus.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>

/**
 * IRobotIOStream over the Nordic UART Service: ovladacka commands arrive on
 * the RX characteristic, telemetry packets leave as TX notifications.
 *
 * Received bytes are buffered for the control thread, which drains them with
 * available()/read(); each arrival also posts an event so the thread wakes up.
 * Writes are dropped while no phone is subscribed to notifications.
 */
class ZephyrNusStream : public IRobotIOStream {
    // ovladacka commands are 3 bytes; this holds a burst of them. When full,
    // the oldest bytes are overwritten and the parser resynchronizes.
    static constexpr size_t RX_BUFFER_SIZE = 64;

    k_event& events;
    uint32_t command_event;

    k_spinlock lock { };
    etl::circular_buffer<uint8_t, RX_BUFFER_SIZE> rx_buffer;
    atomic_t notifications_enabled = ATOMIC_INIT(0);
    bt_nus_cb nus_callbacks { };

    static void on_received(bt_conn* conn, const void* data, uint16_t length, void* context);
    static void on_notifications_changed(bool enabled, void* context);

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
     * Send data as one notification.
     * @return data.size() on success, 0 if nobody is subscribed or the send
     * failed (the data is dropped)
     */
    size_t write(etl::span<const uint8_t> data) override;
};

#endif // ZEPHYR_NUS_STREAM_H
