#ifndef ROBOT_H
#define ROBOT_H

#include "AutoSteering.h"
#include "Battery.h"
#include "ExternalCommandDecoder.h"
#include "IRobotIndicators.h"
#include "Motion.h"
#include "OvladackaParser.h"
#include "Regulator.h"
#include "RobotCommand.h"
#include "RobotPackets.h"
#include "Sensing.h"

/**
 * Robot's own operating mode - AUTOMATIC follows/avoids walls, MANUAL only
 * responds to direct speed/steering commands. See RobotCommand for the
 * decoded external command that toggles it.
 */
enum class RobotState { MANUAL, AUTOMATIC };

/**
 * Class representing the robot. It encapsulates sensing, motion and automatic
 * operation
 */
class Robot {
  Sensing &robot_sensing;
  Battery &robot_battery;
  Motion &robot_motion;
  AutoSteering<int> &wall_following_steering;

  ExternalCommandDecoder &external_command_decoder;

  RobotCommand robot_command;
  RobotState robot_state = RobotState::AUTOMATIC;

  IRobotIndicators &robot_indicators;

public:
  /**
   * Constructor method
   * @param[in] sensing object representing robot's sensing ability
   * @param[in] battery object responsible for battery health monitoring
   * @param[in] motion object representing robot's motion ability
   * @param[in] steering object responsible for automatic steering
   * @param[in] command_decoder object to decode external commands from user
   * @param[in] indicators object representing indicator LEDs
   */
  Robot(Sensing &sensing, Battery &battery, Motion &motion,
        AutoSteering<int> &steering, ExternalCommandDecoder &command_decoder,
        IRobotIndicators &indicators)
      : robot_sensing(sensing), robot_battery(battery), robot_motion(motion),
        wall_following_steering(steering),
        external_command_decoder(command_decoder),
        robot_indicators(indicators) {}

  /**
   * Evaluate robot status, currently just checks if battery voltage is or is
   * not under threshold. If it is robot is disabled and battery led is toggled.
   * @param[in] current_millis current system time in ms
   */
  void perform_status_check(uint32_t current_millis) {
    if (!robot_battery.is_ok(current_millis)) {
      robot_motion.disable();
      robot_sensing.disable();

      robot_indicators.indicate_battery_warning();
    }
  }

  /**
   * Perform automatic action - i.e. compute desired steering servo angle based
   * on sensor measurements
   * @param[in] current_millis current system time in ms
   */
  void perform_automatic_action(uint32_t current_millis) {
    // Always evaluate measurements and desired servo angle, even if automatic
    // operation is disabled, to be able to switch it on immediately when
    // needed.
    DistanceMeasurements distance_measurements;
    robot_sensing.get_distance_measurements(current_millis,
                                            distance_measurements);
    auto desired_servo_angle =
        wall_following_steering.get_steering_command(distance_measurements);

    if (robot_state == RobotState::AUTOMATIC) {
      robot_command.desired_servo_angle = desired_servo_angle;

      auto steering_state = wall_following_steering.get_steering_state();
      if (steering_state == SteeringState::AVOIDING) {
        robot_indicators.indicate_robot_state(
            RobotIndicatorState::MOVING_AVOIDING);

      } else {
        robot_indicators.indicate_robot_state(
            RobotIndicatorState::MOVING_FOLLOWING);
      }
    }
  }

  /**
   * Check if there is a new external command from user and if so, convert it
   * into RobotCommand instance, updating robot_state to match
   */
  void check_external_command() {
    external_command_decoder.check_external_command(robot_command);
    robot_state = robot_command.enable_automatic_operation
                      ? RobotState::AUTOMATIC
                      : RobotState::MANUAL;
  }

  void apply_motion_command() {
    robot_motion.set_speed_and_angle(robot_command.desired_speed,
                                     robot_command.desired_servo_angle);
  }
};

#endif
