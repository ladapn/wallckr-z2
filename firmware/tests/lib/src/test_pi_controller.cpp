#include "PiController.h"

#include <zephyr/ztest.h>

namespace {
const float PERIOD_S = 0.1f;
const float OUTPUT_MIN = -255.0f;
const float OUTPUT_MAX = 255.0f;
const float TOLERANCE = 1e-4f;
} // namespace

// zassert_within() evaluates its first argument twice, so each update() result
// is stored before it is checked.

ZTEST(pi_controller, test_zero_error_gives_zero_output)
{
    float out = 0.0f;
    PiController controller(2.0f, 5.0f, PERIOD_S, OUTPUT_MIN, OUTPUT_MAX);

    out = controller.update(100.0f, 100.0f);

    zassert_within(out, 0.0f, TOLERANCE);
}

ZTEST(pi_controller, test_proportional_term)
{
    float out = 0.0f;
    const float KP = 0.5f;
    PiController controller(KP, 0.0f, PERIOD_S, OUTPUT_MIN, OUTPUT_MAX);

    out = controller.update(100.0f, 60.0f);

    zassert_within(out, KP * 40.0f, TOLERANCE);
    out = controller.update(100.0f, 140.0f);
    zassert_within(out, KP * -40.0f, TOLERANCE);
}

ZTEST(pi_controller, test_integral_term_accumulates_per_period)
{
    float out = 0.0f;
    const float KI = 2.0f;
    const float ERROR = 10.0f;
    PiController controller(0.0f, KI, PERIOD_S, OUTPUT_MIN, OUTPUT_MAX);

    out = controller.update(ERROR, 0.0f);

    zassert_within(out, KI * ERROR * PERIOD_S, TOLERANCE);
    out = controller.update(ERROR, 0.0f);
    zassert_within(out, 2 * KI * ERROR * PERIOD_S, TOLERANCE);
    out = controller.update(ERROR, 0.0f);
    zassert_within(out, 3 * KI * ERROR * PERIOD_S, TOLERANCE);
}

ZTEST(pi_controller, test_proportional_and_integral_terms_add)
{
    float out = 0.0f;
    const float KP = 0.5f;
    const float KI = 2.0f;
    const float ERROR = 10.0f;
    PiController controller(KP, KI, PERIOD_S, OUTPUT_MIN, OUTPUT_MAX);

    controller.update(ERROR, 0.0f);
    out = controller.update(ERROR, 0.0f);
    zassert_within(out, KP * ERROR + 2 * KI * ERROR * PERIOD_S, TOLERANCE);
}

ZTEST(pi_controller, test_integral_holds_output_once_error_is_gone)
{
    float out = 0.0f;
    // The point of the integral term: it keeps the output that removed the
    // error, which a P controller alone can't.
    const float KI = 2.0f;
    const float ERROR = 10.0f;
    PiController controller(1.0f, KI, PERIOD_S, OUTPUT_MIN, OUTPUT_MAX);

    controller.update(ERROR, 0.0f);
    controller.update(ERROR, 0.0f);
    out = controller.update(ERROR, ERROR);
    zassert_within(out, 2 * KI * ERROR * PERIOD_S, TOLERANCE);
}

ZTEST(pi_controller, test_output_is_clamped)
{
    float out = 0.0f;
    PiController controller(10.0f, 0.0f, PERIOD_S, OUTPUT_MIN, OUTPUT_MAX);

    out = controller.update(1000.0f, 0.0f);

    zassert_within(out, OUTPUT_MAX, TOLERANCE);
    out = controller.update(-1000.0f, 0.0f);
    zassert_within(out, OUTPUT_MIN, TOLERANCE);
}

ZTEST(pi_controller, test_no_windup_while_saturated_high)
{
    float out = 0.0f;
    // Without anti-windup, 50 periods at +1000 would build an integral of
    // 5000 and keep the output clamped long after the error reverses.
    PiController controller(0.0f, 1.0f, 1.0f, OUTPUT_MIN, OUTPUT_MAX);

    for (int i = 0; i < 50; i++) {
        out = controller.update(1000.0f, 0.0f);
        zassert_within(out, OUTPUT_MAX, TOLERANCE);
    }

    out = controller.update(0.0f, 10.0f);
    zassert_true(out < OUTPUT_MAX, "output must leave the limit at once");
}

ZTEST(pi_controller, test_no_windup_while_saturated_low)
{
    float out = 0.0f;
    PiController controller(0.0f, 1.0f, 1.0f, OUTPUT_MIN, OUTPUT_MAX);

    for (int i = 0; i < 50; i++) {
        out = controller.update(-1000.0f, 0.0f);
        zassert_within(out, OUTPUT_MIN, TOLERANCE);
    }

    out = controller.update(10.0f, 0.0f);
    zassert_true(out > OUTPUT_MIN, "output must leave the limit at once");
}

ZTEST(pi_controller, test_reset_clears_integral)
{
    float out = 0.0f;
    const float KI = 2.0f;
    PiController controller(0.0f, KI, PERIOD_S, OUTPUT_MIN, OUTPUT_MAX);

    controller.update(10.0f, 0.0f);
    controller.reset();
    out = controller.update(0.0f, 0.0f);
    zassert_within(out, 0.0f, TOLERANCE);
}

ZTEST_SUITE(pi_controller, NULL, NULL, NULL, NULL, NULL);
