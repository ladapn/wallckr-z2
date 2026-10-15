#include "Motion.h"

bool Motion::set_speed(int speed) {
  if (disabled) {
    return false;
  }

  if (prev_speed == speed) {
    return false;
  }

  prev_speed = speed;
  return motor.set_speed(speed);
}

bool Motion::set_angle(int angle) {
  if (disabled) {
    return false;
  }

  if (old_servo != angle) {
    old_servo = angle;
    steering_servo.set_angle(angle);
  } else {
    return false;
  }

  return true;
}

bool Motion::set_speed_and_angle(int speed, int angle) {
  bool retval = set_speed(speed);
  bool retval2 = set_angle(angle);

  return retval && retval2;
}
