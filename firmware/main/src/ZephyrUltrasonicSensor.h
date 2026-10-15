#ifndef ZEPHYR_ULTRASONIC_SENSOR_H
#define ZEPHYR_ULTRASONIC_SENSOR_H

#include "ExpFilter.h"
#include "IDistanceSensor.h"

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

/**
 * IDistanceSensor for an HY-SRF05 (HC-SR04 timing), measured the way v1's
 * NewPing did: echo edges are timestamped from the GPIO interrupt, the wait
 * gives up at the maximum distance, and no echo or an out-of-range one reads
 * as that maximum.
 */
class ZephyrUltrasonicSensor : public IDistanceSensor {
    // State shared with the echo interrupt. A separate standard-layout struct,
    // so the callback can find it with CONTAINER_OF.
    struct Echo {
        gpio_callback callback;
        gpio_dt_spec pin;
        k_sem started;
        k_sem ended;
        uint32_t rise_cycles;
        uint32_t width_cycles;
        bool armed;
        bool rise_seen;
    };

    gpio_dt_spec trigger;
    unsigned int max_cm_distance;
    Echo echo;

    unsigned long measurement_raw_cm = 0;
    unsigned long measurement_filtered_cm = 0;
    static constexpr int FILTER_N = 4;
    ExpFilter<int, FILTER_N> sonar_filter;

    static void on_echo_edge(const device* port, gpio_callback* callback, gpio_port_pins_t pins);

    // Echo pulse width in microseconds, or 0 for no echo or one longer than
    // max_cm_distance (NewPing's NO_ECHO).
    uint32_t ping();

public:
    ZephyrUltrasonicSensor(const gpio_dt_spec& trigger, const gpio_dt_spec& echo_pin, unsigned int max_cm_distance)
        : trigger(trigger)
        , max_cm_distance(max_cm_distance)
        , echo { .pin = echo_pin }
    {
    }

    /**
     * Configure TRIG and the ECHO interrupt.
     * @return 0 on success, a negative errno otherwise
     */
    int init();

    /**
     * Ping once; blocks for up to ~17 ms (NewPing's 5.8 ms wait for the echo
     * to start, then ~11.5 ms for 200 cm).
     * @return always true, as in v1
     */
    bool trigger_measurement() override;
    unsigned long get_distance_raw_cm() override;
    unsigned long get_distance_filtered_cm() override;
};

#endif // ZEPHYR_ULTRASONIC_SENSOR_H
