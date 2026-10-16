#include "ZephyrRobotIndicators.h"

#include <etl/array.h>

int ZephyrRobotIndicators::init()
{
    for (const auto* led : etl::array { &state1, &state2, &error }) {
        if (!gpio_is_ready_dt(led)) {
            return -ENODEV;
        }
        if (int ret = gpio_pin_configure_dt(led, GPIO_OUTPUT_INACTIVE)) {
            return ret;
        }
    }
    return 0;
}

int ZephyrRobotIndicators::indicate_robot_state(RobotIndicatorState state)
{
    bool following = false;
    bool avoiding = false;

    switch (state) {
    case RobotIndicatorState::MOVING_AVOIDING:
        avoiding = true;
        break;
    case RobotIndicatorState::MOVING_FOLLOWING:
        following = true;
        break;
    case RobotIndicatorState::IDLE:
    case RobotIndicatorState::ERROR:
        break;
    }

    if (int ret = gpio_pin_set_dt(&state1, following)) {
        return ret;
    }
    return gpio_pin_set_dt(&state2, avoiding);
}

int ZephyrRobotIndicators::indicate_battery_warning()
{
    error_on = !error_on;
    return gpio_pin_set_dt(&error, error_on);
}
