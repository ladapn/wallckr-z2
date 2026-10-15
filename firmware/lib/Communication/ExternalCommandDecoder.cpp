#include "ExternalCommandDecoder.h"
#include "InputStreamParser.h"
#include "MotionConstants.h"
#include "RobotCommand.h"

bool ExternalCommandDecoder::check_external_command(
    RobotCommand &robot_command) {
  auto incoming_cmd = input_decoder.decode(cmd_stream);

  if (incoming_cmd == JoystickCommand::NO_COMMAND) {
    return false;
  }

  switch (incoming_cmd) {
  case JoystickCommand::UP:
    if (robot_command.desired_speed + SPEED_INCREMENT <= MAX_SPEED) {
      robot_command.desired_speed += SPEED_INCREMENT;
    }
    break;
  case JoystickCommand::DOWN:
    if (robot_command.desired_speed - SPEED_INCREMENT >= -MAX_SPEED) {
      robot_command.desired_speed -= SPEED_INCREMENT;
    }
    break;
  case JoystickCommand::RIGHT:
    if (robot_command.desired_servo_angle - SERVO_INCREMENT >=
        SERVO_MIN_RIGHT) {
      robot_command.desired_servo_angle -= SERVO_INCREMENT;
    }
    break;
  case JoystickCommand::LEFT:
    if (robot_command.desired_servo_angle + SERVO_INCREMENT <= SERVO_MAX_LEFT) {
      robot_command.desired_servo_angle += SERVO_INCREMENT;
    }
    break;
  case JoystickCommand::UP_TRIANGLE:
    robot_command.desired_servo_angle = SERVO_CENTER;
    break;
  case JoystickCommand::RIGHT_CIRCLE:
    robot_command.enable_automatic_operation =
        !robot_command.enable_automatic_operation;
    break;
  default:
    robot_command.desired_speed = 0;
  }

  return true;
}
