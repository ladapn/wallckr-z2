#include "BLEJoystickDecoder.h"
#include "ExternalCommandDecoder.h"
#include "MotionConstants.h"
#include "OvladackaParser.h"
#include "RobotCommand.h"
#include "simulated_uart.h"

#include <zephyr/ztest.h>

namespace {

// Compare fields individually rather than raw struct bytes: RobotCommand
// has padding between its int fields and the trailing bool, and that
// padding is uninitialized - a memcmp of two separately constructed
// instances can spuriously differ even when every field matches.
void assert_commands_equal(const RobotCommand& expected, const RobotCommand& actual)
{
    zassert_equal(actual.desired_speed, expected.desired_speed);
    zassert_equal(actual.desired_servo_angle, expected.desired_servo_angle);
    zassert_equal(actual.enable_automatic_operation, expected.enable_automatic_operation);
}

} // namespace

ZTEST(command_decoder, test_simulated_uart)
{
    char in_data[] = "abcdEFGH";
    auto num_of_elements = sizeof(in_data) / sizeof(char);
    SimulatedUART sim_uart(in_data, num_of_elements);

    char out_data[num_of_elements];
    for (unsigned long i = 0; i < num_of_elements; i++) {
        out_data[i] = sim_uart.read();
    }

    zassert_mem_equal(out_data, in_data, num_of_elements);
}

ZTEST(command_decoder, test_check_external_command_speed_ovladacka)
{
    OvladackaParser in_parser;
    char in_data[] = ":a!";
    SimulatedUART sim_uart(in_data, sizeof(in_data) / sizeof(char));
    ExternalCommandDecoder ext_comm_dec(sim_uart, in_parser);

    RobotCommand rob_cmd_actual;
    RobotCommand rob_cmd_expected;

    rob_cmd_expected.desired_speed = SPEED_INCREMENT;

    ext_comm_dec.check_external_command(rob_cmd_actual);

    assert_commands_equal(rob_cmd_expected, rob_cmd_actual);
}

ZTEST(command_decoder, test_check_external_command_angle_ovladacka)
{
    OvladackaParser in_parser;
    char in_data[] = ":b!";
    SimulatedUART sim_uart(in_data, sizeof(in_data) / sizeof(char));
    ExternalCommandDecoder ext_comm_dec(sim_uart, in_parser);

    RobotCommand rob_cmd_actual;
    RobotCommand rob_cmd_expected;

    rob_cmd_expected.desired_servo_angle = SERVO_CENTER - SERVO_INCREMENT;

    ext_comm_dec.check_external_command(rob_cmd_actual);

    assert_commands_equal(rob_cmd_expected, rob_cmd_actual);
}

ZTEST(command_decoder, test_check_external_command_speed_BLE_joy)
{
    BLEJoystickDecoder in_parser;
    char in_data[] = { 'A', 'a', '\0' }; // Command to increase speed in BLE Joystick format
    SimulatedUART sim_uart(in_data, sizeof(in_data) / sizeof(char));
    ExternalCommandDecoder ext_comm_dec(sim_uart, in_parser);

    RobotCommand rob_cmd_actual;
    RobotCommand rob_cmd_expected;

    rob_cmd_expected.desired_speed = 50;

    ext_comm_dec.check_external_command(rob_cmd_actual);

    assert_commands_equal(rob_cmd_expected, rob_cmd_actual);
}

ZTEST(command_decoder, test_check_external_command_angle_BLE_joy)
{
    BLEJoystickDecoder in_parser;
    char in_data[] = { 'B', 'b', '\0' }; // Command to decrease servo angle in BLE Joystick format
    SimulatedUART sim_uart(in_data, sizeof(in_data) / sizeof(char));
    ExternalCommandDecoder ext_comm_dec(sim_uart, in_parser);

    RobotCommand rob_cmd_actual;
    RobotCommand rob_cmd_expected;

    rob_cmd_expected.desired_servo_angle = 70;

    ext_comm_dec.check_external_command(rob_cmd_actual);

    assert_commands_equal(rob_cmd_expected, rob_cmd_actual);
}

ZTEST_SUITE(command_decoder, NULL, NULL, NULL, NULL, NULL);
