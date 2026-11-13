#include "ClosedLoopMotorController.h"

namespace {

int round_to_int(float value)
{
    return static_cast<int>(value >= 0.0f ? value + 0.5f : value - 0.5f);
}

} // namespace

ClosedLoopMotorController::ClosedLoopMotorController(IMotorController& open_loop_motor,
    IWheelEncoder& wheel_encoder, float kp, float ki, int period_ms, float max_counts_per_s)
    : motor(open_loop_motor)
    , encoder(wheel_encoder)
    , controller(kp, ki, static_cast<float>(period_ms) / 1000.0f, static_cast<float>(-MAX_SPEED),
          static_cast<float>(MAX_SPEED))
    , counts_per_s_per_count(1000.0f / static_cast<float>(period_ms))
    , max_counts_per_s(max_counts_per_s)
{
}

bool ClosedLoopMotorController::set_speed(int speed)
{
    if (speed < -MAX_SPEED || speed > MAX_SPEED) {
        return false;
    }

    target_speed = speed;
    if (speed == 0) {
        stop_motor();
    }
    return true;
}

void ClosedLoopMotorController::stop()
{
    target_speed = 0;
    stop_motor();
}

void ClosedLoopMotorController::stop_motor()
{
    motor.stop();
    controller.reset();
}

void ClosedLoopMotorController::update()
{
    // Read on every period, also while stopped, so the next reading covers
    // exactly one period.
    int32_t counts = 0;
    if (!encoder.read_counts(counts)) {
        stop_motor();
        return;
    }

    if (target_speed == 0) {
        return;
    }

    float target_counts_per_s = static_cast<float>(target_speed) * max_counts_per_s / MAX_SPEED;
    float measured_counts_per_s = static_cast<float>(counts) * counts_per_s_per_count;
    motor.set_speed(round_to_int(controller.update(target_counts_per_s, measured_counts_per_s)));
}
