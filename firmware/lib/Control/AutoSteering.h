#ifndef AUTOSTEERING_H
#define AUTOSTEERING_H

#include "IFilter.h"
#include "MotionConstants.h"
#include "Regulator.h"
#include "Sensing.h"

enum class SteeringState { AVOIDING = 0, FOLLOWING };

template <typename T> class AutoSteering {
  const int TURNING_RADIUS_CM = 15;
  unsigned int AVOIDING_DISTANCE_THR_CM;
  const unsigned int AVOIDING_DISTANCE_RIGHT_FRONT_THR_CM = 15;
  const int AVOIDING_DISTANCE_HYSTERESIS_CM = 20;

  SteeringState automatic_state = SteeringState::FOLLOWING;
  Regulator<T> &side_distance_regulator;
  IFilter<T> &servo_cmd_filter;

public:
  AutoSteering(int side_distance_setpoint_cm, Regulator<T> &side_regulator,
               IFilter<T> &servo_flt)
      : side_distance_regulator(side_regulator), servo_cmd_filter(servo_flt) {
    // Distance to front obstacle that triggers turning, to end up with desired
    // side distance after the turn, this has to consist of the distance
    // setpoint and turning radius
    AVOIDING_DISTANCE_THR_CM = side_distance_setpoint_cm + TURNING_RADIUS_CM;
  };

  int get_steering_command(const DistanceMeasurements &distance_measurements) {
    auto desired_servo = servo_cmd_filter.next(side_distance_regulator.action(
                             distance_measurements.right_distance_cm)) +
                         SERVO_CENTER;

    switch (automatic_state) {
    case SteeringState::FOLLOWING:

      if (distance_measurements.front_distance_cm < AVOIDING_DISTANCE_THR_CM ||
          distance_measurements.right_front_distance_cm <
              AVOIDING_DISTANCE_RIGHT_FRONT_THR_CM) {
        automatic_state = SteeringState::AVOIDING;
      }
      break;
    case SteeringState::AVOIDING:
      desired_servo = SERVO_MAX_LEFT;

      if (distance_measurements.front_distance_cm >
              (AVOIDING_DISTANCE_THR_CM + AVOIDING_DISTANCE_HYSTERESIS_CM) &&
          distance_measurements.right_front_distance_cm >
              (AVOIDING_DISTANCE_THR_CM -
               AVOIDING_DISTANCE_RIGHT_FRONT_THR_CM)) {
        automatic_state = SteeringState::FOLLOWING;
      }

      break;
    }

    return desired_servo;
  }

  SteeringState get_steering_state() { return automatic_state; }
};

#endif
