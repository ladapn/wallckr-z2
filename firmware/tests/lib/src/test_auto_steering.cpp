#include "AutoSteering.h"

#include <zephyr/ztest.h>

namespace {
// Pass-through regulator/filter stand-ins so tests can exercise
// AutoSteering's FOLLOWING/AVOIDING state machine in isolation from
// the regulator and filter math, which are covered by their own tests.
class IdentityRegulator : public Regulator<int> {
protected:
    int regulator_equation(int e) override { return e; }

public:
    // Action limits mirror Regulator's old hardcoded defaults - irrelevant to
    // what this test double is standing in for here.
    IdentityRegulator()
        : Regulator<int>(1, 0, 0, 45, -30)
    {
    }
};

class IdentityFilter : public IFilter<int> {
public:
    int next(int input) override { return input; }
};

// Mirrors AutoSteering's internal thresholds (see AutoSteering.h) so test
// measurements can be expressed relative to them instead of magic numbers.
const int SIDE_DISTANCE_SETPOINT_CM = 25;
const int TURNING_RADIUS_CM = 15;
const int AVOIDING_DISTANCE_RIGHT_FRONT_THR_CM = 15;
const int AVOIDING_DISTANCE_HYSTERESIS_CM = 20;
const int AVOIDING_DISTANCE_THR_CM = SIDE_DISTANCE_SETPOINT_CM + TURNING_RADIUS_CM;

const int FAR_DISTANCE_CM = 100; // clearly outside any threshold used below

void assert_state(AutoSteering<int>& auto_steering, SteeringState expected)
{
    zassert_equal(static_cast<int>(auto_steering.get_steering_state()), static_cast<int>(expected));
}
} // namespace

ZTEST(auto_steering, test_starts_in_following_state)
{
    IdentityRegulator regulator;
    IdentityFilter filter;
    AutoSteering<int> auto_steering(SIDE_DISTANCE_SETPOINT_CM, regulator, filter);

    assert_state(auto_steering, SteeringState::FOLLOWING);
}

ZTEST(auto_steering, test_following_enters_avoiding_on_close_front_obstacle)
{
    IdentityRegulator regulator;
    IdentityFilter filter;
    AutoSteering<int> auto_steering(SIDE_DISTANCE_SETPOINT_CM, regulator, filter);

    DistanceMeasurements measurements;
    measurements.front_distance_cm = AVOIDING_DISTANCE_THR_CM - 1;
    measurements.right_front_distance_cm = FAR_DISTANCE_CM;
    measurements.right_distance_cm = SIDE_DISTANCE_SETPOINT_CM;

    auto_steering.get_steering_command(measurements);

    assert_state(auto_steering, SteeringState::AVOIDING);
}

ZTEST(auto_steering, test_following_enters_avoiding_on_close_right_front_obstacle)
{
    IdentityRegulator regulator;
    IdentityFilter filter;
    AutoSteering<int> auto_steering(SIDE_DISTANCE_SETPOINT_CM, regulator, filter);

    DistanceMeasurements measurements;
    measurements.front_distance_cm = FAR_DISTANCE_CM;
    measurements.right_front_distance_cm = AVOIDING_DISTANCE_RIGHT_FRONT_THR_CM - 1;
    measurements.right_distance_cm = SIDE_DISTANCE_SETPOINT_CM;

    auto_steering.get_steering_command(measurements);

    assert_state(auto_steering, SteeringState::AVOIDING);
}

ZTEST(auto_steering, test_avoiding_commands_max_left_steering)
{
    IdentityRegulator regulator;
    IdentityFilter filter;
    AutoSteering<int> auto_steering(SIDE_DISTANCE_SETPOINT_CM, regulator, filter);

    DistanceMeasurements measurements;
    measurements.front_distance_cm = AVOIDING_DISTANCE_THR_CM - 1;
    measurements.right_front_distance_cm = FAR_DISTANCE_CM;
    measurements.right_distance_cm = SIDE_DISTANCE_SETPOINT_CM;

    auto_steering.get_steering_command(measurements); // FOLLOWING -> AVOIDING
    auto servo = auto_steering.get_steering_command(measurements); // stays AVOIDING

    zassert_equal(servo, SERVO_MAX_LEFT);
}

ZTEST(auto_steering, test_avoiding_returns_to_following_once_clear)
{
    IdentityRegulator regulator;
    IdentityFilter filter;
    AutoSteering<int> auto_steering(SIDE_DISTANCE_SETPOINT_CM, regulator, filter);

    DistanceMeasurements close_measurements;
    close_measurements.front_distance_cm = AVOIDING_DISTANCE_THR_CM - 1;
    close_measurements.right_front_distance_cm = FAR_DISTANCE_CM;
    close_measurements.right_distance_cm = SIDE_DISTANCE_SETPOINT_CM;
    auto_steering.get_steering_command(close_measurements); // enter AVOIDING

    // Recovery needs front > AVOIDING_DISTANCE_THR_CM + hysteresis and
    // right_front > AVOIDING_DISTANCE_THR_CM - right front threshold
    DistanceMeasurements clear_measurements;
    clear_measurements.front_distance_cm = AVOIDING_DISTANCE_THR_CM + AVOIDING_DISTANCE_HYSTERESIS_CM + 1;
    clear_measurements.right_front_distance_cm = AVOIDING_DISTANCE_THR_CM - AVOIDING_DISTANCE_RIGHT_FRONT_THR_CM + 1;
    clear_measurements.right_distance_cm = SIDE_DISTANCE_SETPOINT_CM;

    auto_steering.get_steering_command(clear_measurements);

    assert_state(auto_steering, SteeringState::FOLLOWING);
}

ZTEST(auto_steering, test_avoiding_stays_avoiding_until_both_thresholds_clear)
{
    IdentityRegulator regulator;
    IdentityFilter filter;
    AutoSteering<int> auto_steering(SIDE_DISTANCE_SETPOINT_CM, regulator, filter);

    DistanceMeasurements close_measurements;
    close_measurements.front_distance_cm = AVOIDING_DISTANCE_THR_CM - 1;
    close_measurements.right_front_distance_cm = FAR_DISTANCE_CM;
    close_measurements.right_distance_cm = SIDE_DISTANCE_SETPOINT_CM;
    auto_steering.get_steering_command(close_measurements); // enter AVOIDING

    // Front is clear, but right front sits exactly at its recovery threshold
    // rather than past it, so recovery must not trigger yet (condition is '>')
    DistanceMeasurements partially_clear_measurements;
    partially_clear_measurements.front_distance_cm = AVOIDING_DISTANCE_THR_CM + AVOIDING_DISTANCE_HYSTERESIS_CM + 1;
    partially_clear_measurements.right_front_distance_cm
        = AVOIDING_DISTANCE_THR_CM - AVOIDING_DISTANCE_RIGHT_FRONT_THR_CM;
    partially_clear_measurements.right_distance_cm = SIDE_DISTANCE_SETPOINT_CM;

    auto_steering.get_steering_command(partially_clear_measurements);

    assert_state(auto_steering, SteeringState::AVOIDING);
}

ZTEST_SUITE(auto_steering, NULL, NULL, NULL, NULL, NULL);
