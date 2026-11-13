#include "bringup.h"

#include <etl/array.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>

LOG_MODULE_DECLARE(bringup);

namespace {

// Same speed range as v1's IMotorController::set_speed().
constexpr int max_speed = 255;

const pwm_dt_spec enable = PWM_DT_SPEC_GET(DT_NODELABEL(motor));
const gpio_dt_spec phase = GPIO_DT_SPEC_GET(DT_NODELABEL(motor), ph_gpios);
const gpio_dt_spec fault = GPIO_DT_SPEC_GET(DT_NODELABEL(motor), fault_gpios);

const device* const encoder = DEVICE_DT_GET(DT_NODELABEL(encoder));

// The STM32 qdec driver counts from 0 up to the largest multiple of
// counts-per-revolution that fits TIM4's 16 bits, then wraps.
constexpr int32_t counts_per_revolution = DT_PROP(DT_NODELABEL(encoder), st_counts_per_revolution);
constexpr int32_t encoder_modulus = UINT16_MAX - UINT16_MAX % counts_per_revolution;

// Step-response sampling: one encoder sample per period, up to max_step_ms.
constexpr int step_period_ms = 10;
constexpr int max_step_ms = 5000;
etl::array<int32_t, max_step_ms / step_period_ms + 1> step_counts;

gpio_callback fault_callback;
int current_speed;

int set_speed(int speed)
{
    if (int ret = gpio_pin_set_dt(&phase, speed >= 0)) {
        return ret;
    }

    uint32_t magnitude = speed < 0 ? -speed : speed;
    uint32_t pulse = static_cast<uint64_t>(enable.period) * magnitude / max_speed;
    if (int ret = pwm_set_pulse_dt(&enable, pulse)) {
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

int read_encoder(int32_t& count)
{
    if (int ret = sensor_sample_fetch(encoder)) {
        return ret;
    }
    sensor_value value;
    if (int ret = sensor_channel_get(encoder, SENSOR_CHAN_ENCODER_COUNT, &value)) {
        return ret;
    }
    count = value.val1;
    return 0;
}

// Change between two raw counter values, taking the shorter way round the
// wrap.
int32_t encoder_delta(int32_t from, int32_t to)
{
    int32_t delta = to - from;
    if (delta > encoder_modulus / 2) {
        delta -= encoder_modulus;
    } else if (delta < -encoder_modulus / 2) {
        delta += encoder_modulus;
    }
    return delta;
}

// Applies a duty step and samples the encoder every step_period_ms, then
// stops the motor and prints the samples. Printing waits until the end so the
// UART does not disturb the sampling.
int cmd_step(const shell* sh, size_t, char** argv)
{
    int err = 0;
    long speed = shell_strtol(argv[1], 10, &err);
    long duration_ms = err ? 0 : shell_strtol(argv[2], 10, &err);
    if (err || speed < -max_speed || speed > max_speed || duration_ms < step_period_ms
        || duration_ms > max_step_ms) {
        shell_error(sh, "Usage: motor step <-%d..%d> <%d..%d ms>", max_speed, max_speed, step_period_ms,
            max_step_ms);
        return -EINVAL;
    }

    if (!device_is_ready(encoder)) {
        shell_error(sh, "Encoder not ready");
        return -ENODEV;
    }

    int32_t raw = 0;
    if (int ret = read_encoder(raw)) {
        shell_error(sh, "Encoder read failed (%d)", ret);
        return ret;
    }

    step_counts[0] = 0;
    size_t samples = 1;
    int32_t position = 0;
    bool fault_seen = false;

    int64_t start_ms = k_uptime_get();
    if (int ret = set_speed(static_cast<int>(speed))) {
        set_speed(0);
        return ret;
    }
    for (long t = step_period_ms; t <= duration_ms; t += step_period_ms) {
        k_sleep(K_TIMEOUT_ABS_MS(start_ms + t));
        int32_t now = 0;
        if (int ret = read_encoder(now)) {
            set_speed(0);
            shell_error(sh, "Encoder read failed (%d)", ret);
            return ret;
        }
        position += encoder_delta(raw, now);
        raw = now;
        step_counts[samples++] = position;
        fault_seen = fault_seen || gpio_pin_get_dt(&fault) == 1;
    }
    set_speed(0);

    shell_print(sh, "t_ms,count,counts_per_s");
    for (size_t i = 0; i < samples; i++) {
        int32_t per_s = i == 0 ? 0 : (step_counts[i] - step_counts[i - 1]) * (1000 / step_period_ms);
        shell_print(sh, "%d,%d,%d", static_cast<int>(i) * step_period_ms, static_cast<int>(step_counts[i]),
            static_cast<int>(per_s));
    }

    // Steady speed over the last quarter of the step, and the time to reach
    // 63 % of it (the time constant, for a first-order response).
    size_t last = samples - 1;
    size_t quarter = last / 4 > 0 ? last / 4 : 1;
    int32_t steady = (step_counts[last] - step_counts[last - quarter]) * (1000 / step_period_ms)
        / static_cast<int32_t>(quarter);
    long tau_ms = -1;
    for (size_t i = 1; steady != 0 && i <= last; i++) {
        int32_t per_s = (step_counts[i] - step_counts[i - 1]) * (1000 / step_period_ms);
        if ((steady >= 0 && per_s * 1000 >= steady * 632) || (steady < 0 && per_s * 1000 <= steady * 632)) {
            tau_ms = static_cast<long>(i) * step_period_ms;
            break;
        }
    }
    shell_print(sh, "steady: %d counts/s (%d rpm at the gearbox output), 63 %% at: %ld ms (-1: none)",
        static_cast<int>(steady), static_cast<int>(steady * 60 / counts_per_revolution), tau_ms);
    if (speed != 0 && steady != 0 && (speed > 0) != (steady > 0)) {
        shell_warn(sh, "The encoder counts against the motor direction");
    }
    if (fault_seen) {
        shell_warn(sh, "nFAULT was asserted during the step");
    }
    return 0;
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

    if (int ret = gpio_pin_configure_dt(&phase, GPIO_OUTPUT_ACTIVE)) {
        return ret;
    }
    if (int ret = gpio_pin_configure_dt(&fault, GPIO_INPUT)) {
        return ret;
    }
    if (int ret = gpio_pin_interrupt_configure_dt(&fault, GPIO_INT_EDGE_TO_ACTIVE)) {
        return ret;
    }
    gpio_init_callback(&fault_callback, on_fault, BIT(fault.pin));
    if (int ret = gpio_add_callback_dt(&fault, &fault_callback)) {
        return ret;
    }

    return set_speed(0);
}

SHELL_STATIC_SUBCMD_SET_CREATE(motor_cmds,
    SHELL_CMD_ARG(speed, NULL, "Set speed: motor speed <-255..255>", cmd_speed, 2, 0),
    SHELL_CMD_ARG(step, NULL,
        "Step response: motor step <-255..255> <10..5000 ms>; prints encoder samples every 10 ms",
        cmd_step, 3, 0),
    SHELL_CMD(stop, NULL, "Stop the motor", cmd_stop),
    SHELL_CMD(status, NULL, "Show speed and driver fault state", cmd_status),
    SHELL_SUBCMD_SET_END);
SHELL_CMD_REGISTER(motor, &motor_cmds, "Drive motor (DRV8874)", NULL);
