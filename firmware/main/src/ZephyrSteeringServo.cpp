#include "ZephyrSteeringServo.h"

namespace {
constexpr int MAX_ANGLE = 180;
constexpr uint32_t INITIAL_PULSE_NS = PWM_USEC(1500);
} // namespace

int ZephyrSteeringServo::init()
{
    if (!pwm_is_ready_dt(&servo)) {
        return -ENODEV;
    }
    return pwm_set_pulse_dt(&servo, INITIAL_PULSE_NS);
}

bool ZephyrSteeringServo::set_angle(int angle)
{
    if (angle < 0) {
        angle = 0;
    } else if (angle > MAX_ANGLE) {
        angle = MAX_ANGLE;
    }

    uint32_t pulse_ns = min_pulse_ns + (max_pulse_ns - min_pulse_ns) * angle / MAX_ANGLE;
    return pwm_set_pulse_dt(&servo, pulse_ns) == 0;
}
