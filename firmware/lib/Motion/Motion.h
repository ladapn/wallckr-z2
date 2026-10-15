#ifndef MOTION_H
#define MOTION_H

#include "IMotorController.h"
#include "ISteeringServo.h"

/**
 * Class responsible for robot movement, namely for control of motor and
 * steering servo
 */
class Motion {
  int prev_speed = 0;
  int old_servo = 0;
  IMotorController &motor;
  ISteeringServo &steering_servo;
  bool disabled = false;

public:
  /**
   * Constructor method, configures pins attached to servo and motor controls,
   * stops motor as a safety measure
   */
  Motion(IMotorController &motor_controller, ISteeringServo &steering_servo)
      : motor(motor_controller), steering_servo(steering_servo) {
    stop();
  }

  /**
   * Set motor speed to desired value. If the desired speed is the same as in
   * previous call of this method, or if Motion module is disabled, nothing
   * happens
   * @param[in] speed desired speed, positive value means forward motion,
   * negative value backward motion, in both cases maximal value is 255
   * @return true if speed command is different to previous one and Motion is
   * not disabled, false otherwise
   */
  bool set_speed(int speed);
  /**
   * Set steering servo to desired value. If the desired value is the same as in
   * previous call of this method, or if Motion module is disabled, nothing
   * happens
   * @param[in] angle desired angle in degrees from 0 to 180
   * @return true if servo command is different to previous one and Motion is
   * not disabled, false otherwise
   */
  bool set_angle(int angle);
  /**
   * Stops motor
   */
  void stop() { motor.stop(); };
  /**
   * Set motor speed and steering servo angle to desired values. This is
   * equivalent to calling set_speed and SetAngle in sequence.
   * @param[in] speed desired speed, positive value means forward motion,
   * negative value backward motion, in both cases maximal value is 255
   * @param[in] angle desired angle in degrees from 0 to 180
   * @return logic and of return values of internally called set_speed and
   * SetAngle
   */
  bool set_speed_and_angle(int speed, int angle);
  /**
   * Disabled Motion module. Robot motor is stopped and any command to motor and
   * servo gets ignored.
   */
  void disable() {
    stop();
    disabled = true;
  }
  /**
   * Enables Motion module. Commands to motor and servo will be accepted.
   */
  void enable() { disabled = false; }
};

#endif
