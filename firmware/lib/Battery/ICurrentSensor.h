#ifndef ICURRENTSENSOR_H
#define ICURRENTSENSOR_H

#include <stdint.h>

/**
 * Interface for a high-side current sensor measuring total robot current
 * draw
 */
class ICurrentSensor {
public:
  /**
   * Method to get sensed current in milliamps
   * @return current reading in mA
   */
  virtual uint16_t get_current_mA() = 0;

  virtual ~ICurrentSensor() = default;
};

#endif // ICURRENTSENSOR_H
