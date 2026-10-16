#ifndef IROBOT_INDICATORS_H
#define IROBOT_INDICATORS_H

#include <stdint.h>

enum class RobotIndicatorState : uint8_t {
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
