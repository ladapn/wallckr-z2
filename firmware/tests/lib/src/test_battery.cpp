#include "Battery.h"
#include "IRobotIOStream.h"
#include "RobotPackets.h"
#include "RobotPrinter.h"

#include <zephyr/ztest.h>

namespace {
// Stub battery sensor with a settable voltage reading, so tests can
// simulate the pack draining/recovering without real hardware.
class StubBatterySensor : public IBatterySensor {
public:
    uint16_t voltage_mVDC = 0;
    uint16_t get_battery_voltage_adc() override { return 0; }
    uint16_t get_battery_voltage_mVDC() override { return voltage_mVDC; }
};

// Stub current sensor with a settable mA reading.
class StubCurrentSensor : public ICurrentSensor {
public:
    uint16_t current_mA = 0;
    uint16_t get_current_mA() override { return current_mA; }
};

// Minimal IRobotIOStream that just captures what was written, so tests
// can inspect the telemetry Battery::is_ok sends out.
class CapturingIOStream : public IRobotIOStream {
public:
    uint8_t buffer[32] = { 0 };
    size_t bytes_written = 0;

    int available() override { return 0; }
    int read() override { return -1; }
    size_t write(uint8_t c) override
    {
        if (bytes_written < sizeof(buffer)) {
            buffer[bytes_written] = c;
        }
        bytes_written++;
        return 1;
    }
    size_t write(uint8_t* data, size_t size) override
    {
        for (size_t i = 0; i < size; i++) {
            write(data[i]);
        }
        return size;
    }
};

const uint16_t CUTOFF_mVDC = 450;
const uint16_t HYSTERESIS_mVDC = 50;
} // namespace

ZTEST(battery, test_battery_ok_above_cutoff)
{
    StubBatterySensor sensor;
    sensor.voltage_mVDC = CUTOFF_mVDC + 1;
    StubCurrentSensor current_sensor;
    CapturingIOStream io_stream;
    RobotPrinter printer(io_stream);
    Battery battery(sensor, current_sensor, printer, CUTOFF_mVDC, HYSTERESIS_mVDC);

    zassert_true(battery.is_ok(0));
}

ZTEST(battery, test_battery_dead_at_or_below_cutoff)
{
    StubBatterySensor sensor;
    sensor.voltage_mVDC = CUTOFF_mVDC; // exactly at cutoff, not below
    StubCurrentSensor current_sensor;
    CapturingIOStream io_stream;
    RobotPrinter printer(io_stream);
    Battery battery(sensor, current_sensor, printer, CUTOFF_mVDC, HYSTERESIS_mVDC);

    zassert_false(battery.is_ok(0));
}

ZTEST(battery, test_battery_stays_dead_until_hysteresis_cleared)
{
    StubBatterySensor sensor;
    StubCurrentSensor current_sensor;
    CapturingIOStream io_stream;
    RobotPrinter printer(io_stream);
    Battery battery(sensor, current_sensor, printer, CUTOFF_mVDC, HYSTERESIS_mVDC);

    sensor.voltage_mVDC = CUTOFF_mVDC - 1;
    battery.is_ok(0); // drains below cutoff, enters dead state

    // Recovered above cutoff, but sitting exactly at (not past) the
    // hysteresis band - must not be considered ok yet.
    sensor.voltage_mVDC = CUTOFF_mVDC + HYSTERESIS_mVDC;
    zassert_false(battery.is_ok(1000));
}

ZTEST(battery, test_battery_recovers_once_hysteresis_cleared)
{
    StubBatterySensor sensor;
    StubCurrentSensor current_sensor;
    CapturingIOStream io_stream;
    RobotPrinter printer(io_stream);
    Battery battery(sensor, current_sensor, printer, CUTOFF_mVDC, HYSTERESIS_mVDC);

    sensor.voltage_mVDC = CUTOFF_mVDC - 1;
    battery.is_ok(0); // drains below cutoff, enters dead state

    sensor.voltage_mVDC = CUTOFF_mVDC + HYSTERESIS_mVDC + 1;
    zassert_true(battery.is_ok(1000));
}

ZTEST(battery, test_battery_sends_status_packet_with_measured_voltage)
{
    StubBatterySensor sensor;
    sensor.voltage_mVDC = 1234;
    StubCurrentSensor current_sensor;
    CapturingIOStream io_stream;
    RobotPrinter printer(io_stream);
    Battery battery(sensor, current_sensor, printer, CUTOFF_mVDC, HYSTERESIS_mVDC);

    battery.is_ok(2000);

    zassert_equal(io_stream.bytes_written, StatusPacket::SERIALIZED_SIZE);

    // battery_voltage_mvdc is a little-endian uint16_t right after id (1
    // byte), tick (4 bytes) and version_ID (4 bytes) - see RobotPackets.h
    uint16_t reported_voltage_mVDC
        = static_cast<uint16_t>(io_stream.buffer[9]) | (static_cast<uint16_t>(io_stream.buffer[10]) << 8);
    zassert_equal(reported_voltage_mVDC, 1234);
}

ZTEST(battery, test_battery_sends_status_packet_with_measured_current)
{
    StubBatterySensor sensor;
    StubCurrentSensor current_sensor;
    current_sensor.current_mA = 512;
    CapturingIOStream io_stream;
    RobotPrinter printer(io_stream);
    Battery battery(sensor, current_sensor, printer, CUTOFF_mVDC, HYSTERESIS_mVDC);

    battery.is_ok(2000);

    // total_current_mA and motor_current_mA are little-endian uint16_t
    // fields right after battery_voltage_mvdc - see RobotPackets.h
    uint16_t reported_total_current_mA
        = static_cast<uint16_t>(io_stream.buffer[11]) | (static_cast<uint16_t>(io_stream.buffer[12]) << 8);
    uint16_t reported_motor_current_mA
        = static_cast<uint16_t>(io_stream.buffer[13]) | (static_cast<uint16_t>(io_stream.buffer[14]) << 8);

    // total current comes from the sensor...
    zassert_equal(reported_total_current_mA, 512);
    // ...but there's no motor-specific sensor, so it must be flagged as not
    // measured rather than sent as a misleading 0.
    zassert_equal(reported_motor_current_mA, 0xFFFF);
}

ZTEST_SUITE(battery, NULL, NULL, NULL, NULL, NULL);
