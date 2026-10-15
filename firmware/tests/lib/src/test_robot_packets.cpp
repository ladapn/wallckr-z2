#include "RobotPackets.h"
#include "RobotPrinter.h"
#include "simulated_uart.h"

#include <zephyr/ztest.h>

ZTEST(robot_packets, test_status_packet)
{
    const int CURRENT_TICK = 1000;
    const int BATTERY_VOLTAGE = 100;
    const int TOTAL_CURRENT = 0;
    const int MOTOR_CURRENT = 0;
    StatusPacket status_packet(CURRENT_TICK, BATTERY_VOLTAGE, TOTAL_CURRENT, MOTOR_CURRENT);
    status_packet.version_ID = 0xAB;
    // Little endian on the wire, i.e. LSB goes first
    uint8_t expected[] = {
        0x50, // id
        0xE8, 0x03, 0x00, 0x00, // tick
        0xAB, 0x00, 0x00, 0x00, // version ID
        0x64, 0x00, // battery voltage
        0x00, 0x00, // total current
        0x00, 0x00, // motor current
        0x74 // CRC
    };

    SimulatedUART sim_uart;
    RobotPrinter printer(sim_uart);

    printer.print(status_packet);

    uint8_t actual[sizeof(expected)];
    for (unsigned i = 0; i < sizeof(expected); i++) {
        actual[i] = sim_uart.read();
    }

    zassert_mem_equal(actual, expected, sizeof(expected));
}

ZTEST(robot_packets, test_sonar_packet)
{
    const int PACKET_ID = 101;
    const unsigned long CURRENT_TICK = 1000;
    const unsigned long MEASUREMENT = 0xdeadbeef;

    SonarPacket sonar_packet(PACKET_ID, CURRENT_TICK, MEASUREMENT);
    // Little endian on the wire, i.e. LSB goes first
    uint8_t expected[] = {
        0x65, // id
        0xE8, 0x03, 0x00, 0x00, // tick
        0xEF, 0xBE, 0xAD, 0xDE, // measurement
        0xAC // CRC
    };

    SimulatedUART sim_uart;
    RobotPrinter printer(sim_uart);

    printer.print(sonar_packet);

    uint8_t actual[sizeof(expected)];
    for (unsigned i = 0; i < sizeof(expected); i++) {
        actual[i] = sim_uart.read();
    }

    zassert_mem_equal(actual, expected, sizeof(expected));
}

ZTEST_SUITE(robot_packets, NULL, NULL, NULL, NULL, NULL);
