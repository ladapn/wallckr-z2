#include "bringup.h"

#include <zephyr/drivers/pwm.h>
#include <zephyr/shell/shell.h>

namespace {

#define SERVO_NODE DT_NODELABEL(steering_servo)

const pwm_dt_spec servo = PWM_DT_SPEC_GET(SERVO_NODE);
constexpr long min_pulse_us = DT_PROP(SERVO_NODE, min_pulse) / 1000;
constexpr long max_pulse_us = DT_PROP(SERVO_NODE, max_pulse) / 1000;

// Same angle range and meaning as v1's ISteeringServo::set_angle().
constexpr long max_angle = 180;

int cmd_angle(const shell* sh, size_t, char** argv)
{
    int err = 0;
    long angle = shell_strtol(argv[1], 10, &err);
    if (err || angle < 0 || angle > max_angle) {
        shell_error(sh, "Angle must be in [0, %ld]", max_angle);
        return -EINVAL;
    }

    long pulse_us = min_pulse_us + (max_pulse_us - min_pulse_us) * angle / max_angle;
    shell_print(sh, "angle %ld -> pulse %ld us", angle, pulse_us);
    return pwm_set_pulse_dt(&servo, PWM_USEC(pulse_us));
}

int cmd_pulse(const shell* sh, size_t, char** argv)
{
    int err = 0;
    long pulse_us = shell_strtol(argv[1], 10, &err);
    if (err || pulse_us < min_pulse_us || pulse_us > max_pulse_us) {
        shell_error(sh, "Pulse must be in [%ld, %ld] us", min_pulse_us, max_pulse_us);
        return -EINVAL;
    }
    return pwm_set_pulse_dt(&servo, PWM_USEC(pulse_us));
}

int cmd_off(const shell*, size_t, char**)
{
    return pwm_set_pulse_dt(&servo, 0);
}

} // namespace

int servo_init()
{
    // The servo stays unpowered (no pulses) until a command moves it.
    return pwm_is_ready_dt(&servo) ? 0 : -ENODEV;
}

SHELL_STATIC_SUBCMD_SET_CREATE(servo_cmds,
    SHELL_CMD_ARG(angle, NULL, "Move to an angle: servo angle <0..180>", cmd_angle, 2, 0),
    SHELL_CMD_ARG(pulse, NULL, "Set the raw pulse width: servo pulse <us>", cmd_pulse, 2, 0),
    SHELL_CMD(off, NULL, "Stop sending pulses (servo goes limp)", cmd_off),
    SHELL_SUBCMD_SET_END);
SHELL_CMD_REGISTER(servo, &servo_cmds, "Steering servo (HS-422)", NULL);
