#include "bringup.h"

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>

LOG_MODULE_DECLARE(bringup);

namespace {

// Same speed range as v1's IMotorController::set_speed().
constexpr int max_speed = 255;

const pwm_dt_spec enable = PWM_DT_SPEC_GET(DT_NODELABEL(motor));
const gpio_dt_spec phase = GPIO_DT_SPEC_GET(DT_NODELABEL(motor), ph_gpios);
const gpio_dt_spec fault = GPIO_DT_SPEC_GET(DT_NODELABEL(motor), fault_gpios);

gpio_callback fault_callback;
int current_speed;

int set_speed(int speed)
{
    if (int ret = gpio_pin_set_dt(&phase, speed >= 0); ret) {
        return ret;
    }

    uint32_t magnitude = speed < 0 ? -speed : speed;
    uint32_t pulse = static_cast<uint64_t>(enable.period) * magnitude / max_speed;
    if (int ret = pwm_set_pulse_dt(&enable, pulse); ret) {
        return ret;
    }

    current_speed = speed;
    return 0;
}

void on_fault(const device*, gpio_callback*, gpio_port_pins_t)
{
    LOG_WRN("Motor driver reports a fault (nFAULT asserted)");
}

int cmd_speed(const shell* sh, size_t, char** argv)
{
    int err = 0;
    long speed = shell_strtol(argv[1], 10, &err);
    if (err || speed < -max_speed || speed > max_speed) {
        shell_error(sh, "Speed must be in [-%d, %d]", max_speed, max_speed);
        return -EINVAL;
    }
    return set_speed(static_cast<int>(speed));
}

int cmd_stop(const shell*, size_t, char**)
{
    return set_speed(0);
}

int cmd_status(const shell* sh, size_t, char**)
{
    shell_print(sh, "speed: %d, direction: %s, fault: %s", current_speed,
        current_speed >= 0 ? "forward" : "backward",
        gpio_pin_get_dt(&fault) ? "YES" : "no");
    return 0;
}

} // namespace

int motor_init()
{
    if (!pwm_is_ready_dt(&enable) || !gpio_is_ready_dt(&phase) || !gpio_is_ready_dt(&fault)) {
        return -ENODEV;
    }

    if (int ret = gpio_pin_configure_dt(&phase, GPIO_OUTPUT_ACTIVE); ret) {
        return ret;
    }
    if (int ret = gpio_pin_configure_dt(&fault, GPIO_INPUT); ret) {
        return ret;
    }
    if (int ret = gpio_pin_interrupt_configure_dt(&fault, GPIO_INT_EDGE_TO_ACTIVE); ret) {
        return ret;
    }
    gpio_init_callback(&fault_callback, on_fault, BIT(fault.pin));
    if (int ret = gpio_add_callback_dt(&fault, &fault_callback); ret) {
        return ret;
    }

    return set_speed(0);
}

SHELL_STATIC_SUBCMD_SET_CREATE(motor_cmds,
    SHELL_CMD_ARG(speed, NULL, "Set speed: motor speed <-255..255>", cmd_speed, 2, 0),
    SHELL_CMD(stop, NULL, "Stop the motor", cmd_stop),
    SHELL_CMD(status, NULL, "Show speed and driver fault state", cmd_status),
    SHELL_SUBCMD_SET_END);
SHELL_CMD_REGISTER(motor, &motor_cmds, "Drive motor (DRV8874)", NULL);
