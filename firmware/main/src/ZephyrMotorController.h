#ifndef ZEPHYR_MOTOR_CONTROLLER_H
#define ZEPHYR_MOTOR_CONTROLLER_H

#include "IMotorController.h"

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>

/**
 * IMotorController for a DRV8874 in PH/EN mode: a PWM on EN sets the speed,
 * PH the direction (active = forward).
 */
class ZephyrMotorController : public IMotorController {
    pwm_dt_spec enable;
    gpio_dt_spec phase;

public:
    ZephyrMotorController(const pwm_dt_spec& enable, const gpio_dt_spec& phase)
        : enable(enable)
        , phase(phase)
    {
    }

    /**
     * Configure PH and stop the motor. Call before handing the controller to
     * Motion, whose constructor stops the motor too.
     * @return 0 on success, a negative errno otherwise
     */
    int init();

    bool set_speed(int speed) override;
    void stop() override;
};

#endif // ZEPHYR_MOTOR_CONTROLLER_H
