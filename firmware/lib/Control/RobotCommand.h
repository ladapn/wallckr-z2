#ifndef ROBOT_COMMAND_H
#define ROBOT_COMMAND_H

#include "MotionConstants.h"

/**
 * Structure to group possible robot commands, namely desired speed, desired
 * steering servo angle and flag to enable automatic operation
 */
struct RobotCommand {
  int desired_speed = 0;
  int desired_servo_angle = SERVO_CENTER;
  bool enable_automatic_operation = true;
};

#endif
