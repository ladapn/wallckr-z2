#ifndef IROBOT_INDICATORS_H
#define IROBOT_INDICATORS_H

enum class RobotIndicatorState {
  IDLE = 0,
  MOVING_AVOIDING,
  MOVING_FOLLOWING,
  ERROR
};

class IRobotIndicators {
public:
  virtual ~IRobotIndicators() = default;
  virtual int indicate_robot_state(RobotIndicatorState state) = 0;
  virtual int indicate_battery_warning() = 0;
};

#endif // IROBOT_INDICATORS_H
