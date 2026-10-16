#ifndef ZEPHYR_ROBOT_INDICATORS_H
#define ZEPHYR_ROBOT_INDICATORS_H

#include "IRobotIndicators.h"

#include <zephyr/drivers/gpio.h>

/**
 * IRobotIndicators on the Button/LED shield, with v1's LED meanings:
 * STATE1 (v1's LED1) = wall following, STATE2 (LED2) = avoiding, ERR (LED5)
 * toggles on every battery warning. Polarity comes from the devicetree.
 */
class ZephyrRobotIndicators : public IRobotIndicators {
    gpio_dt_spec state1;
    gpio_dt_spec state2;
    gpio_dt_spec error;
    bool error_on = false;

public:
    ZephyrRobotIndicators(const gpio_dt_spec& state1, const gpio_dt_spec& state2, const gpio_dt_spec& error)
        : state1(state1)
        , state2(state2)
        , error(error)
    {
    }

    /**
     * Configure the LEDs, all off.
     * @return 0 on success, a negative errno otherwise
     */
    int init();

    int indicate_robot_state(RobotIndicatorState state) override;

    /**
     * Toggle the ERR LED; called on every status check while the battery is
     * dead, so it blinks at half the status check rate, as in v1.
     */
    int indicate_battery_warning() override;
};

#endif // ZEPHYR_ROBOT_INDICATORS_H
