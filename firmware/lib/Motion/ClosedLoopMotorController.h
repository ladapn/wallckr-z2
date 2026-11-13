#ifndef CLOSED_LOOP_MOTOR_CONTROLLER_H
#define CLOSED_LOOP_MOTOR_CONTROLLER_H

#include "IMotorController.h"
#include "IWheelEncoder.h"
#include "PiController.h"

#include <stdint.h>

/**
 * IMotorController that regulates the motor speed with the wheel encoder.
 *
 * It wraps an open-loop IMotorController, whose speed is a PWM duty. The speed
 * given to set_speed() becomes a target, in the same [-255, 255] range but now
 * meaning a fraction of max_counts_per_s; update(), called once per period,
 * measures the speed and sets the duty from a PI controller.
 *
 * A target of 0, stop() and a failed encoder read all set the duty to 0 at
 * once and reset the controller, so the motor never runs on stale data.
 */
class ClosedLoopMotorController : public IMotorController {
    static constexpr int MAX_SPEED = 255;

    IMotorController& motor;
    IWheelEncoder& encoder;
    PiController controller;
    float counts_per_s_per_count;
    float max_counts_per_s;
    int target_speed = 0;

    void stop_motor();

public:
    /**
     * Constructor method
     * @param[in] open_loop_motor motor controller taking a PWM duty
     * @param[in] wheel_encoder encoder on the same motor
     * @param[in] kp proportional gain, duty per count/s of speed error
     * @param[in] ki integral gain, duty per count/s of speed error and second
     * @param[in] period_ms time between calls of update(), in ms
     * @param[in] max_counts_per_s speed that set_speed(255) asks for, in
     * encoder counts per second
     */
    ClosedLoopMotorController(IMotorController& open_loop_motor, IWheelEncoder& wheel_encoder, float kp, float ki,
        int period_ms, float max_counts_per_s);

    /**
     * Set the target speed; the duty follows on the next update(), except that
     * 0 stops the motor at once
     * @param[in] speed desired speed in the range [-255, 255], as a fraction of
     * max_counts_per_s
     * @return true if the speed is in range, false otherwise
     */
    bool set_speed(int speed) override;

    /**
     * Stop the motor at once and reset the controller
     */
    void stop() override;

    /**
     * Measure the speed and set the duty; call once per period
     */
    void update();
};

#endif // CLOSED_LOOP_MOTOR_CONTROLLER_H
