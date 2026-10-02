#include "bringup.h"

#include <etl/array.h>
#include <etl/string_view.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>

namespace {

struct Led {
    etl::string_view name;
    gpio_dt_spec gpio;
};

const etl::array leds {
    Led { .name = "state1", .gpio = GPIO_DT_SPEC_GET(DT_NODELABEL(led_state1), gpios) },
    Led { .name = "state2", .gpio = GPIO_DT_SPEC_GET(DT_NODELABEL(led_state2), gpios) },
    Led { .name = "err", .gpio = GPIO_DT_SPEC_GET(DT_NODELABEL(led_err), gpios) },
};

const Led* find_led(const etl::string_view& name)
{
    for (const auto& led : leds) {
        if (led.name == name) {
            return &led;
        }
    }
    return nullptr;
}

int cmd_led(const shell* sh, size_t, char** argv)
{
    const Led* led = find_led(argv[1]);
    if (!led) {
        shell_error(sh, "Unknown LED '%s' (state1, state2, err)", argv[1]);
        return -EINVAL;
    }

    etl::string_view state { argv[2] };
    if (state != "on" && state != "off") {
        shell_error(sh, "Expected on or off");
        return -EINVAL;
    }
    return gpio_pin_set_dt(&led->gpio, state == "on");
}

int cmd_led_test(const shell*, size_t, char**)
{
    leds_self_test();
    return 0;
}

} // namespace

int leds_init()
{
    for (const auto& led : leds) {
        if (!gpio_is_ready_dt(&led.gpio)) {
            return -ENODEV;
        }
        if (int ret = gpio_pin_configure_dt(&led.gpio, GPIO_OUTPUT_INACTIVE); ret) {
            return ret;
        }
    }
    return 0;
}

void leds_self_test()
{
    for (const auto& led : leds) {
        gpio_pin_set_dt(&led.gpio, 1);
        k_msleep(300);
        gpio_pin_set_dt(&led.gpio, 0);
    }
}

SHELL_CMD_ARG_REGISTER(led, NULL, "Switch a status LED: led <state1|state2|err> <on|off>", cmd_led, 3, 0);
SHELL_CMD_REGISTER(led_test, NULL, "Light each status LED in turn", cmd_led_test);
