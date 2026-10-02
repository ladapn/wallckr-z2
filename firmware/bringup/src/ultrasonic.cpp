// HY-SRF05 ranging: a 10 us TRIG pulse starts a measurement, and the ECHO
// pulse width is the round-trip time. The pulse edges are timestamped from
// the GPIO interrupt with the cycle counter.

#include "bringup.h"

#include <etl/array.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>

namespace {

// Same conversion v1 used (NewPing's round-trip microseconds per cm).
constexpr int us_per_cm = 57;
// With nothing in range the HY-SRF05 still returns a ~30 ms echo pulse.
constexpr int out_of_range_us = 25000;
constexpr int echo_timeout_ms = 40;
// Let the previous ping's echoes die out before the next sensor fires.
constexpr int ping_gap_ms = 60;

struct Sonar {
    gpio_dt_spec trigger;
    gpio_dt_spec echo;
    gpio_callback callback;
    k_sem done;
    uint32_t rise_cycles;
    uint32_t width_cycles;
    bool rise_seen;
};

#define SONAR(node)                                                     \
    Sonar                                                               \
    {                                                                   \
        .trigger = GPIO_DT_SPEC_GET(DT_NODELABEL(node), trigger_gpios), \
        .echo = GPIO_DT_SPEC_GET(DT_NODELABEL(node), echo_gpios)        \
    }

etl::array sonars {
    SONAR(ultrasonic1),
    SONAR(ultrasonic2),
    SONAR(ultrasonic3),
    SONAR(ultrasonic4),
};

void on_echo_edge(const device*, gpio_callback* callback, gpio_port_pins_t)
{
    uint32_t now = k_cycle_get_32();
    Sonar* sonar = CONTAINER_OF(callback, Sonar, callback);

    if (gpio_pin_get_dt(&sonar->echo)) {
        sonar->rise_cycles = now;
        sonar->rise_seen = true;
    } else if (sonar->rise_seen) {
        sonar->width_cycles = now - sonar->rise_cycles;
        sonar->rise_seen = false;
        k_sem_give(&sonar->done);
    }
}

// Returns the echo pulse width in microseconds, or a negative errno.
int ping(Sonar& sonar)
{
    sonar.rise_seen = false;
    k_sem_reset(&sonar.done);

    gpio_pin_set_dt(&sonar.trigger, 1);
    k_busy_wait(10);
    gpio_pin_set_dt(&sonar.trigger, 0);

    if (k_sem_take(&sonar.done, K_MSEC(echo_timeout_ms))) {
        return -ETIMEDOUT;
    }
    return static_cast<int>(k_cyc_to_us_floor32(sonar.width_cycles));
}

void print_ping(const shell* sh, size_t index)
{
    int echo_us = ping(sonars[index]);
    if (echo_us < 0) {
        shell_print(sh, "sensor %u: no echo pulse (check wiring)", index + 1);
    } else if (echo_us >= out_of_range_us) {
        shell_print(sh, "sensor %u: out of range (%d us)", index + 1, echo_us);
    } else {
        shell_print(sh, "sensor %u: %d cm (%d us)", index + 1, echo_us / us_per_cm, echo_us);
    }
}

int cmd_sonar(const shell* sh, size_t argc, char** argv)
{
    if (argc == 1) {
        for (size_t i = 0; i < sonars.size(); i++) {
            if (i) {
                k_msleep(ping_gap_ms);
            }
            print_ping(sh, i);
        }
        return 0;
    }

    int err = 0;
    unsigned long number = shell_strtoul(argv[1], 10, &err);
    if (err || number < 1 || number > sonars.size()) {
        shell_error(sh, "Sensor must be 1..%u", sonars.size());
        return -EINVAL;
    }
    print_ping(sh, number - 1);
    return 0;
}

} // namespace

int ultrasonic_init()
{
    for (auto& sonar : sonars) {
        if (!gpio_is_ready_dt(&sonar.trigger) || !gpio_is_ready_dt(&sonar.echo)) {
            return -ENODEV;
        }

        k_sem_init(&sonar.done, 0, 1);
        if (int ret = gpio_pin_configure_dt(&sonar.trigger, GPIO_OUTPUT_INACTIVE); ret) {
            return ret;
        }
        if (int ret = gpio_pin_configure_dt(&sonar.echo, GPIO_INPUT); ret) {
            return ret;
        }
        if (int ret = gpio_pin_interrupt_configure_dt(&sonar.echo, GPIO_INT_EDGE_BOTH); ret) {
            return ret;
        }
        gpio_init_callback(&sonar.callback, on_echo_edge, BIT(sonar.echo.pin));
        if (int ret = gpio_add_callback_dt(&sonar.echo, &sonar.callback); ret) {
            return ret;
        }
    }
    return 0;
}

SHELL_CMD_ARG_REGISTER(sonar, NULL,
    "Measure distance: sonar [1..4] (all sensors if omitted)", cmd_sonar, 1, 1);
