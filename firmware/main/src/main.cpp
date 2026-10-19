#include "AutoSteering.h"
#include "Battery.h"
#include "ExpFilter.h"
#include "ExternalCommandDecoder.h"
#include "Motion.h"
#include "OvladackaParser.h"
#include "Regulator.h"
#include "Robot.h"
#include "RobotPrinter.h"
#include "Sensing.h"
#include "ZephyrAnalogSensors.h"
#include "ZephyrMotorController.h"
#include "ZephyrNusStream.h"
#include "ZephyrRobotIndicators.h"
#include "ZephyrSteeringServo.h"
#include "ZephyrUltrasonicSensor.h"

#include <app_version.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(wallckr, LOG_LEVEL_INF);

namespace {

// Events the control thread waits on.
constexpr uint32_t TICK_EVENT = BIT(0);
constexpr uint32_t COMMAND_EVENT = BIT(1);

// Automatic action every tick, status check every 10th tick (v1's 100 ms
// and 1000 ms).
constexpr k_timeout_t TICK_PERIOD = K_MSEC(100);
constexpr uint32_t STATUS_CHECK_TICKS = 10;

// v1's NewPing maximum; a timeout reads as this distance.
constexpr unsigned int MAX_DISTANCE_CM = 200;

// Battery thresholds in mV at the ADC pin, behind the 1/5 divider.
constexpr int BATTERY_CELL_COUNT = 8;
constexpr int BATTERY_VOLTAGE_DIVIDER_FACTOR = 5;
constexpr uint16_t BATTERY_CELL_CUTOFF_mVDC = 900;
constexpr uint16_t BATTERY_PACK_CUTOFF_mVDC
    = BATTERY_CELL_CUTOFF_mVDC * BATTERY_CELL_COUNT / BATTERY_VOLTAGE_DIVIDER_FACTOR;
// v1's 50 mV behind its 1/16 divider: 800 mV at the pack.
constexpr uint16_t BATTERY_HYSTERESIS_mVDC = 160;

// Wall-following steering, as in v1.
constexpr int FILTER_N = 4;
constexpr int K_GAIN = 5;
constexpr int RIGHT_DISTANCE_SETPOINT_CM = 25;
constexpr int INSENSITIV_CM = 2;
constexpr int REGULATOR_ACTION_LIMIT_UP = 45;
constexpr int REGULATOR_ACTION_LIMIT_BOTTOM = -30;

#define MOTOR_NODE DT_NODELABEL(motor)
#define SERVO_NODE DT_NODELABEL(steering_servo)
#define CURRENT_NODE DT_NODELABEL(supply_current)

k_event control_events;
k_timer tick_timer;

void on_tick(k_timer*)
{
    k_event_post(&control_events, TICK_EVENT);
}

bool init_ok(int ret, const char* what)
{
    if (ret) {
        LOG_ERR("%s init failed: %d", what, ret);
    }
    return ret == 0;
}

} // namespace

// The main thread is the control thread: it owns the robot, and nothing else
// calls into it. The objects live on its stack, as in v1's loop(); main()
// never returns once they exist.
int main()
{
    LOG_INF("wallckr-z2 %s", APP_VERSION_EXTENDED_STRING);

    k_event_init(&control_events);
    k_timer_init(&tick_timer, on_tick, nullptr);

    // Front = J4 (Sensor 3), right front = J3 (Sensor 2), right = J2
    // (Sensor 1, v1's right center).
    ZephyrUltrasonicSensor sonar_front(GPIO_DT_SPEC_GET(DT_NODELABEL(ultrasonic3), trigger_gpios),
        GPIO_DT_SPEC_GET(DT_NODELABEL(ultrasonic3), echo_gpios), MAX_DISTANCE_CM);
    ZephyrUltrasonicSensor sonar_right_front(GPIO_DT_SPEC_GET(DT_NODELABEL(ultrasonic2), trigger_gpios),
        GPIO_DT_SPEC_GET(DT_NODELABEL(ultrasonic2), echo_gpios), MAX_DISTANCE_CM);
    ZephyrUltrasonicSensor sonar_right_center(GPIO_DT_SPEC_GET(DT_NODELABEL(ultrasonic1), trigger_gpios),
        GPIO_DT_SPEC_GET(DT_NODELABEL(ultrasonic1), echo_gpios), MAX_DISTANCE_CM);
    ZephyrBatterySensor battery_sensor(ADC_DT_SPEC_GET(DT_NODELABEL(battery_voltage)));
    ZephyrCurrentSensor current_sensor(ADC_DT_SPEC_GET(CURRENT_NODE),
        DT_PROP(CURRENT_NODE, sense_resistor_milli_ohms), DT_PROP(CURRENT_NODE, sense_gain_mult),
        DT_PROP(CURRENT_NODE, sense_gain_div));
    ZephyrMotorController motor_controller(PWM_DT_SPEC_GET(MOTOR_NODE), GPIO_DT_SPEC_GET(MOTOR_NODE, ph_gpios));
    ZephyrSteeringServo steering_servo(
        PWM_DT_SPEC_GET(SERVO_NODE), DT_PROP(SERVO_NODE, min_pulse), DT_PROP(SERVO_NODE, max_pulse));
    ZephyrRobotIndicators robot_indicators(GPIO_DT_SPEC_GET(DT_NODELABEL(led_state1), gpios),
        GPIO_DT_SPEC_GET(DT_NODELABEL(led_state2), gpios), GPIO_DT_SPEC_GET(DT_NODELABEL(led_err), gpios));
    ZephyrNusStream nus_stream(control_events, COMMAND_EVENT);

    // The motor first, so it is stopped as early as possible.
    if (!init_ok(motor_controller.init(), "motor") || !init_ok(steering_servo.init(), "servo")
        || !init_ok(sonar_front.init(), "front sensor") || !init_ok(sonar_right_front.init(), "right front sensor")
        || !init_ok(sonar_right_center.init(), "right sensor") || !init_ok(battery_sensor.init(), "battery sensor")
        || !init_ok(current_sensor.init(), "current sensor") || !init_ok(robot_indicators.init(), "LEDs")
        || !init_ok(nus_stream.init(), "Bluetooth")) {
        // Don't run the robot, but keep the objects alive: callbacks already
        // registered (echo interrupts, Bluetooth) still point at them.
        k_sleep(K_FOREVER);
    }

    RobotPrinter ble_out(nus_stream);
    DistanceSensors distance_sensors
        = { .front = sonar_front, .right_front = sonar_right_front, .right_center = sonar_right_center };
    Sensing robot_sensing(distance_sensors, ble_out);
    Battery robot_battery(
        battery_sensor, current_sensor, ble_out, BATTERY_PACK_CUTOFF_mVDC, BATTERY_HYSTERESIS_mVDC);
    Motion robot_motion(motor_controller, steering_servo);
    OvladackaParser ovladacka_parser;
    ExternalCommandDecoder external_command_decoder(nus_stream, ovladacka_parser);
    ExpFilter<int, FILTER_N> servo_cmd_filter;
    Regulator_P<int> side_distance_regulator(K_GAIN, RIGHT_DISTANCE_SETPOINT_CM, INSENSITIV_CM,
        REGULATOR_ACTION_LIMIT_UP, REGULATOR_ACTION_LIMIT_BOTTOM);
    AutoSteering<int> wall_following_steering(RIGHT_DISTANCE_SETPOINT_CM, side_distance_regulator, servo_cmd_filter);
    Robot robot(robot_sensing, robot_battery, robot_motion, wall_following_steering, external_command_decoder,
        robot_indicators);

    // First tick right away, so the first status check runs at boot as in v1.
    k_timer_start(&tick_timer, K_NO_WAIT, TICK_PERIOD);

    for (uint32_t tick = 0;;) {
        // Clears the events it returns atomically, so none posted meanwhile
        // is lost.
        uint32_t events = k_event_wait_safe(&control_events, TICK_EVENT | COMMAND_EVENT, false, K_FOREVER);

        // The parser takes at most 10 bytes per call; drain them all now
        // rather than leaving the rest for the next tick.
        while (nus_stream.available() > 0) {
            robot.check_external_command();
        }

        if (events & TICK_EVENT) {
            uint32_t now_ms = k_uptime_get_32();
            if (tick % STATUS_CHECK_TICKS == 0) {
                robot.perform_status_check(now_ms);
            }
            robot.perform_automatic_action(now_ms);
            tick++;
        }

        robot.apply_motion_command();
        nus_stream.flush();
    }
}
