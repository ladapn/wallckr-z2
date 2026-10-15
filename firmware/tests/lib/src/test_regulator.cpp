#include "Regulator.h"

#include <zephyr/ztest.h>

namespace {
const int SETPOINT_CM = 25;
const int INSENSITIVITY_CM = 2;
const int ACTION_LIMIT_UP = 45;
const int ACTION_LIMIT_BOTTOM = -30;
} // namespace

ZTEST(regulator, test_p_regulator_within_insensitivity_band_is_zero)
{
    const int GAIN = 5;
    Regulator_P<int> regulator(GAIN, SETPOINT_CM, INSENSITIVITY_CM, ACTION_LIMIT_UP, ACTION_LIMIT_BOTTOM);

    zassert_equal(regulator.action(SETPOINT_CM - 1), 0); // e = 1, within insensitivity
}

ZTEST(regulator, test_p_regulator_applies_gain)
{
    const int GAIN = 5;
    const int ERROR_CM = 5;
    Regulator_P<int> regulator(GAIN, SETPOINT_CM, INSENSITIVITY_CM, ACTION_LIMIT_UP, ACTION_LIMIT_BOTTOM);

    zassert_equal(regulator.action(SETPOINT_CM - ERROR_CM), GAIN * ERROR_CM);
}

ZTEST(regulator, test_p_regulator_clamps_to_upper_limit)
{
    const int GAIN = 10;
    Regulator_P<int> regulator(GAIN, SETPOINT_CM, INSENSITIVITY_CM, ACTION_LIMIT_UP, ACTION_LIMIT_BOTTOM);

    zassert_equal(regulator.action(0), ACTION_LIMIT_UP); // e = 25, raw out = 250
}

ZTEST(regulator, test_p_regulator_clamps_to_lower_limit)
{
    const int GAIN = 10;
    Regulator_P<int> regulator(GAIN, SETPOINT_CM, INSENSITIVITY_CM, ACTION_LIMIT_UP, ACTION_LIMIT_BOTTOM);

    zassert_equal(regulator.action(50), ACTION_LIMIT_BOTTOM); // e = -25, raw out = -250
}

ZTEST(regulator, test_p_regulator_clamps_to_custom_action_limits)
{
    const int GAIN = 10;
    // Deliberately different from ACTION_LIMIT_UP/ACTION_LIMIT_BOTTOM above, to
    // prove the limits are actually per-instance rather than hardcoded.
    const int CUSTOM_ACTION_LIMIT_UP = 20;
    const int CUSTOM_ACTION_LIMIT_BOTTOM = -15;
    Regulator_P<int> regulator(
        GAIN, SETPOINT_CM, INSENSITIVITY_CM, CUSTOM_ACTION_LIMIT_UP, CUSTOM_ACTION_LIMIT_BOTTOM);

    zassert_equal(regulator.action(0), CUSTOM_ACTION_LIMIT_UP); // e = 25, raw out = 250
    zassert_equal(regulator.action(50), CUSTOM_ACTION_LIMIT_BOTTOM); // e = -25, raw out = -250
}

ZTEST(regulator, test_pd_regulator_first_action_ignores_derivative)
{
    const int GAIN = 2;
    const int D_GAIN = 3;
    const int ERROR_CM = 5;
    Regulator_PD<int> regulator(GAIN, D_GAIN, SETPOINT_CM, INSENSITIVITY_CM, ACTION_LIMIT_UP, ACTION_LIMIT_BOTTOM);

    zassert_equal(regulator.action(SETPOINT_CM - ERROR_CM), GAIN * ERROR_CM);
}

ZTEST(regulator, test_pd_regulator_adds_derivative_term_on_subsequent_action)
{
    const int GAIN = 2;
    const int D_GAIN = 3;
    const int FIRST_ERROR_CM = 5;
    const int SECOND_ERROR_CM = 10;
    Regulator_PD<int> regulator(GAIN, D_GAIN, SETPOINT_CM, INSENSITIVITY_CM, ACTION_LIMIT_UP, ACTION_LIMIT_BOTTOM);

    regulator.action(SETPOINT_CM - FIRST_ERROR_CM); // primes derivative state (last_e = FIRST_ERROR_CM)
    zassert_equal(regulator.action(SETPOINT_CM - SECOND_ERROR_CM),
        GAIN * SECOND_ERROR_CM + (SECOND_ERROR_CM - FIRST_ERROR_CM) * D_GAIN);
}

ZTEST(regulator, test_pd_regulator_keeps_last_error_across_insensitivity_band)
{
    const int GAIN = 1;
    const int D_GAIN = 1;
    const int FIRST_ERROR_CM = 5;
    const int THIRD_ERROR_CM = 6;
    Regulator_PD<int> regulator(GAIN, D_GAIN, SETPOINT_CM, INSENSITIVITY_CM, ACTION_LIMIT_UP, ACTION_LIMIT_BOTTOM);

    regulator.action(SETPOINT_CM - FIRST_ERROR_CM); // primes derivative state (last_e = FIRST_ERROR_CM)
    regulator.action(SETPOINT_CM); // e = 0, inside insensitivity band, derivative state left untouched
    // derivative term uses the stale last_e (FIRST_ERROR_CM) rather than the
    // skipped e (0)
    zassert_equal(regulator.action(SETPOINT_CM - THIRD_ERROR_CM),
        GAIN * THIRD_ERROR_CM + (THIRD_ERROR_CM - FIRST_ERROR_CM) * D_GAIN);
}

ZTEST_SUITE(regulator, NULL, NULL, NULL, NULL, NULL);
