#ifndef MOTOR_CONTROLLER_H
#define MOTOR_CONTROLLER_H

/**
 * Interface for motor controller
 */
class IMotorController {
public:
    /**
     * Set the speed of the motor
     * @param[in] speed desired speed of the motor, in the range [-255, 255]
     * @return true if the speed was set successfully, false otherwise
     */
    virtual bool set_speed(int speed) = 0;
    /**
     * Stop the motor
     */
    virtual void stop() = 0;
    virtual ~IMotorController() = default;
};

#endif // MOTOR_CONTROLLER_H
