#ifndef ISTEERING_SERVO_H
#define ISTEERING_SERVO_H

/**
 * Interface for steering servo
 */
class ISteeringServo {
public: /**
         * Set the angle of the steering servo
         * @param[in] angle desired angle of the steering servo, in the range
         * [0, 180]
         * @return true if the angle was set successfully, false otherwise
         */
  virtual bool set_angle(int angle) = 0;
  virtual ~ISteeringServo() = default;
};

#endif // ISTEERING_SERVO_H
