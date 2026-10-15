#include "ZephyrMotorController.h"

#include "MotionConstants.h"

int ZephyrMotorController::init()
{
    if (!pwm_is_ready_dt(&enable) || !gpio_is_ready_dt(&phase)) {
        return -ENODEV;
    }
    if (int ret = gpio_pin_configure_dt(&phase, GPIO_OUTPUT_INACTIVE)) {
        return ret;
    }
    stop();
    return 0;
}

bool ZephyrMotorController::set_speed(int speed)
{
    if (speed > MAX_SPEED || speed < -MAX_SPEED) {
        return false;
    }

    if (gpio_pin_set_dt(&phase, speed >= 0)) {
        return false;
    }

    // Same duty as v1's analogWrite(speed): |speed| / MAX_SPEED of the period.
    auto magnitude = static_cast<uint32_t>(speed < 0 ? -speed : speed);
    auto pulse = static_cast<uint32_t>(static_cast<uint64_t>(enable.period) * magnitude / MAX_SPEED);
    return pwm_set_pulse_dt(&enable, pulse) == 0;
}

void ZephyrMotorController::stop()
{
    // As in v1, stopping only drops the duty to 0 and leaves PH as it is.
    pwm_set_pulse_dt(&enable, 0);
}
