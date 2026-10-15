#ifndef ZEPHYR_STEERING_SERVO_H
#define ZEPHYR_STEERING_SERVO_H

#include "ISteeringServo.h"

#include <zephyr/drivers/pwm.h>

/**
 * ISteeringServo for an RC servo on a PWM channel, mapping 0-180 degrees
 * linearly onto [min_pulse_ns, max_pulse_ns] like v1's Arduino Servo library.
 */
class ZephyrSteeringServo : public ISteeringServo {
    pwm_dt_spec servo;
    uint32_t min_pulse_ns;
    uint32_t max_pulse_ns;

public:
    ZephyrSteeringServo(const pwm_dt_spec& servo, uint32_t min_pulse_ns, uint32_t max_pulse_ns)
        : servo(servo)
        , min_pulse_ns(min_pulse_ns)
        , max_pulse_ns(max_pulse_ns)
    {
    }

    /**
     * Start sending pulses at 1500 us, the position the Arduino Servo library
     * holds from attach() until the first write().
     * @return 0 on success, a negative errno otherwise
     */
    int init();

    /**
     * @param[in] angle desired angle in degrees; values outside [0, 180] are
     * clamped, as Arduino's Servo::write() does
     */
    bool set_angle(int angle) override;
};

#endif // ZEPHYR_STEERING_SERVO_H
