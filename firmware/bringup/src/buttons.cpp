// Buttons are gpio-keys, so the input subsystem debounces them and reports
// press/release events, logged here. No init is needed beyond the driver's.

#include <etl/array.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/input/input.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>

LOG_MODULE_DECLARE(bringup);

namespace {

struct Button {
    const char* name;
    uint16_t code;
    gpio_dt_spec gpio;
};

#define BUTTON(label, node)                                                \
    Button                                                                 \
    {                                                                      \
        .name = (label), .code = DT_PROP(DT_NODELABEL(node), zephyr_code), \
        .gpio = GPIO_DT_SPEC_GET(DT_NODELABEL(node), gpios)                \
    }

const etl::array buttons {
    BUTTON("Button 1", button1),
    BUTTON("Button 2", button2),
};

void on_input(input_event* event, void*)
{
    if (event->type != INPUT_EV_KEY) {
        return;
    }

    for (const auto& button : buttons) {
        if (button.code == event->code) {
            LOG_INF("%s %s", button.name, event->value ? "pressed" : "released");
        }
    }
}

int cmd_buttons(const shell* sh, size_t, char**)
{
    for (const auto& button : buttons) {
        shell_print(sh, "%s: %s", button.name, gpio_pin_get_dt(&button.gpio) ? "pressed" : "released");
    }
    return 0;
}

} // namespace

INPUT_CALLBACK_DEFINE(DEVICE_DT_GET(DT_PARENT(DT_NODELABEL(button1))), on_input, nullptr);

SHELL_CMD_REGISTER(buttons, NULL, "Show the current button states", cmd_buttons);
