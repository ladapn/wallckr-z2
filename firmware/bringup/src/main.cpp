#include "bringup.h"

#include <etl/array.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(bringup, LOG_LEVEL_INF);

namespace {

struct Module {
    const char* name;
    int (*init)();
};

constexpr etl::array modules {
    Module { .name = "leds", .init = leds_init },
    Module { .name = "motor", .init = motor_init },
    Module { .name = "servo", .init = servo_init },
    Module { .name = "ultrasonic", .init = ultrasonic_init },
    Module { .name = "analog", .init = analog_init },
    Module { .name = "encoder", .init = encoder_init },
};

} // namespace

int main()
{
    LOG_INF("wallckr-z2 bring-up firmware");

    int failures = 0;
    for (const auto& module : modules) {
        if (int ret = module.init()) {
            LOG_ERR("%s: init failed (%d)", module.name, ret);
            failures++;
        }
    }

    leds_self_test();

    if (failures) {
        LOG_ERR("%d module(s) failed to initialise", failures);
    }
    LOG_INF("Ready, type 'help' for the bring-up commands");
    return 0;
}
