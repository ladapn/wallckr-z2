#include "ZephyrUltrasonicSensor.h"

namespace {
// NewPing's constants, so readings and timeouts match v1.
constexpr uint32_t US_ROUNDTRIP_CM = 57;
// How long the sensor may take to start its echo pulse after the trigger.
constexpr uint32_t MAX_SENSOR_DELAY_US = 5800;
constexpr uint32_t TRIGGER_PULSE_US = 10;
} // namespace

void ZephyrUltrasonicSensor::on_echo_edge(const device*, gpio_callback* callback, gpio_port_pins_t)
{
    uint32_t now = k_cycle_get_32();
    Echo* echo = CONTAINER_OF(callback, Echo, callback);

    // Edges outside a measurement, e.g. the end of an echo that already
    // timed out, are ignored.
    if (!echo->armed) {
        return;
    }
    if (gpio_pin_get_dt(&echo->pin) > 0) {
        echo->rise_cycles = now;
        echo->rise_seen = true;
        k_sem_give(&echo->started);
    } else if (echo->rise_seen) {
        echo->width_cycles = now - echo->rise_cycles;
        echo->armed = false;
        k_sem_give(&echo->ended);
    }
}

int ZephyrUltrasonicSensor::init()
{
    if (!gpio_is_ready_dt(&trigger) || !gpio_is_ready_dt(&echo.pin)) {
        return -ENODEV;
    }

    k_sem_init(&echo.started, 0, 1);
    k_sem_init(&echo.ended, 0, 1);
    if (int ret = gpio_pin_configure_dt(&trigger, GPIO_OUTPUT_INACTIVE)) {
        return ret;
    }
    if (int ret = gpio_pin_configure_dt(&echo.pin, GPIO_INPUT)) {
        return ret;
    }
    if (int ret = gpio_pin_interrupt_configure_dt(&echo.pin, GPIO_INT_EDGE_BOTH)) {
        return ret;
    }
    gpio_init_callback(&echo.callback, on_echo_edge, BIT(echo.pin.pin));
    return gpio_add_callback_dt(&echo.pin, &echo.callback);
}

uint32_t ZephyrUltrasonicSensor::ping()
{
    // NewPing's limit: the round trip for one more than the maximum distance.
    const uint32_t max_echo_us = (max_cm_distance + 1) * US_ROUNDTRIP_CM + US_ROUNDTRIP_CM / 2;

    k_sem_reset(&echo.started);
    k_sem_reset(&echo.ended);
    echo.rise_seen = false;
    echo.armed = true;

    gpio_pin_set_dt(&trigger, 1);
    k_busy_wait(TRIGGER_PULSE_US);
    gpio_pin_set_dt(&trigger, 0);

    if (k_sem_take(&echo.started, K_USEC(MAX_SENSOR_DELAY_US)) != 0
        || k_sem_take(&echo.ended, K_USEC(max_echo_us)) != 0) {
        echo.armed = false;
        return 0;
    }

    uint32_t width_us = k_cyc_to_us_floor32(echo.width_cycles);
    return width_us <= max_echo_us ? width_us : 0;
}

bool ZephyrUltrasonicSensor::trigger_measurement()
{
    // As in v1: NewPing reports no echo and out-of-range alike as 0, and both
    // read as the maximum distance.
    measurement_raw_cm = ping() / US_ROUNDTRIP_CM;
    if (measurement_raw_cm == 0) {
        measurement_raw_cm = max_cm_distance;
    }

    return true;
}

unsigned long ZephyrUltrasonicSensor::get_distance_raw_cm()
{
    trigger_measurement();

    return measurement_raw_cm;
}

unsigned long ZephyrUltrasonicSensor::get_distance_filtered_cm()
{
    trigger_measurement();

    measurement_filtered_cm = sonar_filter.next(static_cast<int>(measurement_raw_cm));

    return measurement_filtered_cm;
}
