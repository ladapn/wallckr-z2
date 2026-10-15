#include "ExpFilter.h"

#include <zephyr/ztest.h>

namespace {

// Overloads select the right comparison for each filter value type.
void assert_filter_output(int expected, int actual)
{
    zassert_equal(actual, expected);
}

void assert_filter_output(float expected, float actual)
{
    // Same tolerance as Unity's TEST_ASSERT_EQUAL_FLOAT, which v1's test used:
    // a relative delta of expected * 0.00001.
    float delta = expected * 0.00001f;
    zassert_within(actual, expected, delta < 0 ? -delta : delta);
}

template <int N, typename T>
void test_step(const T input[], const T expected[], unsigned len)
{
    ExpFilter<T, N> filter;
    for (unsigned i = 0; i < len; i++) {
        auto out = filter.next(input[i]);

        assert_filter_output(expected[i], out);
    }
}

} // namespace

ZTEST(filter, test_step_N_4_int)
{
    constexpr int FILTER_N = 4;
    const int input[] = { 0, 0, 0, 0, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 };
    const int expected[] = { 0, 0, 0, 0, 25, 43, 57, 67, 75, 81, 85, 88, 91, 93, 94, 95, 96, 97, 97 };

    test_step<FILTER_N>(input, expected, ARRAY_SIZE(input));
}

ZTEST(filter, test_step_N_4_float)
{
    constexpr int FILTER_N = 4;
    const float input[] = { 0, 0, 0, 0, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 };
    const float expected[] = { 0, 0, 0, 0, 25, 43.75, 57.8125, 68.359375, 76.26953125, 82.2021484375,
        86.651611328125, 89.9887084960938, 92.4915313720703, 94.3686485290527, 95.7764863967896, 96.8323647975922,
        97.6242735981941, 98.2182051986456, 98.6636538989842 };

    test_step<FILTER_N>(input, expected, ARRAY_SIZE(input));
}

ZTEST(filter, test_step_N_10_int)
{
    constexpr int FILTER_N = 10;
    const int input[] = { 0, 0, 0, 0, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 };
    const int expected[] = { 0, 0, 0, 0, 10, 19, 27, 34, 40, 46, 51, 55, 59, 63, 66, 69, 72, 74, 76 };

    test_step<FILTER_N>(input, expected, ARRAY_SIZE(input));
}

ZTEST(filter, test_step_N_10_float)
{
    constexpr int FILTER_N = 10;
    const float input[] = { 0, 0, 0, 0, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 };
    const float expected[] = { 0, 0, 0, 0, 10, 19, 27.1, 34.39, 40.951, 46.8559, 52.17031, 56.953279, 61.2579511,
        65.13215599, 68.618940391, 71.7570463519, 74.58134171671, 77.123207545039, 79.4108867905351 };

    test_step<FILTER_N>(input, expected, ARRAY_SIZE(input));
}

ZTEST_SUITE(filter, NULL, NULL, NULL, NULL, NULL);
