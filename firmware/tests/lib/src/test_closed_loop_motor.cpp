#include "ClosedLoopMotorController.h"

#include <zephyr/ztest.h>

namespace {

// Records the duty it was given, as the open-loop motor controller.
class FakeMotor : public IMotorController {
public:
    int speed = 0;
    int set_speed_calls = 0;
    int stop_calls = 0;

    bool set_speed(int new_speed) override
    {
        speed = new_speed;
        set_speed_calls++;
        return true;
    }
    void stop() override
    {
        speed = 0;
        stop_calls++;
    }
};

// Reports the same count every period, or a read failure.
class FakeEncoder : public IWheelEncoder {
public:
    int32_t counts_per_read = 0;
    bool fails = false;

    bool read_counts(int32_t& counts) override
    {
        if (fails) {
            return false;
        }
        counts = counts_per_read;
        return true;
    }
};

const int PERIOD_MS = 100;
const float MAX_COUNTS_PER_S = 2000.0f;
const float KP = 0.1f;

} // namespace

ZTEST(closed_loop_motor, test_rejects_speed_out_of_range)
{
    FakeMotor motor;
    FakeEncoder encoder;
    ClosedLoopMotorController controller(motor, encoder, KP, 0.0f, PERIOD_MS, MAX_COUNTS_PER_S);

    zassert_false(controller.set_speed(256));
    zassert_false(controller.set_speed(-256));
    zassert_true(controller.set_speed(255));
    zassert_true(controller.set_speed(-255));
}

ZTEST(closed_loop_motor, test_duty_changes_only_on_update)
{
    FakeMotor motor;
    FakeEncoder encoder;
    ClosedLoopMotorController controller(motor, encoder, KP, 0.0f, PERIOD_MS, MAX_COUNTS_PER_S);

    controller.set_speed(255);
    zassert_equal(motor.set_speed_calls, 0);

    controller.update();
    zassert_equal(motor.set_speed_calls, 1);
}

ZTEST(closed_loop_motor, test_proportional_duty_from_speed_error)
{
    FakeMotor motor;
    FakeEncoder encoder;
    ClosedLoopMotorController controller(motor, encoder, KP, 0.0f, PERIOD_MS, MAX_COUNTS_PER_S);

    // Target 2000 counts/s; 100 counts per 100 ms is 1000 counts/s.
    controller.set_speed(255);
    encoder.counts_per_read = 100;
    controller.update();

    zassert_equal(motor.speed, 100); // 0.1 * (2000 - 1000)
}

ZTEST(closed_loop_motor, test_speed_is_a_fraction_of_max)
{
    FakeMotor motor;
    FakeEncoder encoder;
    ClosedLoopMotorController controller(motor, encoder, KP, 0.0f, PERIOD_MS, MAX_COUNTS_PER_S);

    controller.set_speed(51); // 51 / 255 of 2000 = 400 counts/s
    controller.update();

    zassert_equal(motor.speed, 40);
}

ZTEST(closed_loop_motor, test_backward)
{
    FakeMotor motor;
    FakeEncoder encoder;
    ClosedLoopMotorController controller(motor, encoder, KP, 0.0f, PERIOD_MS, MAX_COUNTS_PER_S);

    controller.set_speed(-255);
    encoder.counts_per_read = -100;
    controller.update();

    zassert_equal(motor.speed, -100);
}

ZTEST(closed_loop_motor, test_integral_raises_duty_while_too_slow)
{
    FakeMotor motor;
    FakeEncoder encoder;
    // 0.5 * 1000 counts/s * 0.1 s = 50 duty per period
    ClosedLoopMotorController controller(motor, encoder, 0.0f, 0.5f, PERIOD_MS, MAX_COUNTS_PER_S);

    controller.set_speed(255);
    encoder.counts_per_read = 100;
    controller.update();
    zassert_equal(motor.speed, 50);
    controller.update();
    zassert_equal(motor.speed, 100);
}

ZTEST(closed_loop_motor, test_duty_is_clamped)
{
    FakeMotor motor;
    FakeEncoder encoder;
    ClosedLoopMotorController controller(motor, encoder, 1.0f, 0.0f, PERIOD_MS, MAX_COUNTS_PER_S);

    controller.set_speed(255);
    controller.update();
    zassert_equal(motor.speed, 255);

    controller.set_speed(-255);
    controller.update();
    zassert_equal(motor.speed, -255);
}

ZTEST(closed_loop_motor, test_zero_speed_stops_at_once_and_stays_stopped)
{
    FakeMotor motor;
    FakeEncoder encoder;
    ClosedLoopMotorController controller(motor, encoder, KP, 0.0f, PERIOD_MS, MAX_COUNTS_PER_S);

    controller.set_speed(255);
    controller.update();
    controller.set_speed(0);
    zassert_equal(motor.stop_calls, 1);
    zassert_equal(motor.speed, 0);

    int calls = motor.set_speed_calls;
    encoder.counts_per_read = 50; // still coasting
    controller.update();
    zassert_equal(motor.set_speed_calls, calls, "the loop must not drive towards zero");
    zassert_equal(motor.speed, 0);
}

ZTEST(closed_loop_motor, test_stop_resets_target_and_integral)
{
    FakeMotor motor;
    FakeEncoder encoder;
    ClosedLoopMotorController controller(motor, encoder, 0.0f, 0.5f, PERIOD_MS, MAX_COUNTS_PER_S);

    controller.set_speed(255);
    controller.update();
    controller.update(); // integral at 100
    controller.stop();
    zassert_equal(motor.speed, 0);

    controller.update();
    zassert_equal(motor.speed, 0, "stop() clears the target");

    controller.set_speed(255);
    controller.update();
    zassert_equal(motor.speed, 100, "integral restarts from zero"); // 0.5 * 2000 * 0.1
}

ZTEST(closed_loop_motor, test_encoder_failure_stops_motor_and_resets_integral)
{
    FakeMotor motor;
    FakeEncoder encoder;
    ClosedLoopMotorController controller(motor, encoder, 0.0f, 0.5f, PERIOD_MS, MAX_COUNTS_PER_S);

    controller.set_speed(255);
    encoder.counts_per_read = 100;
    controller.update();
    controller.update(); // duty 100

    encoder.fails = true;
    controller.update();
    zassert_equal(motor.stop_calls, 1);
    zassert_equal(motor.speed, 0);

    // The target survives; the loop restarts from zero integral.
    encoder.fails = false;
    controller.update();
    zassert_equal(motor.speed, 50);
}

ZTEST_SUITE(closed_loop_motor, NULL, NULL, NULL, NULL, NULL);
