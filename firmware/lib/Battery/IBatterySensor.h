#ifndef IBATTERYSENSOR_H
#define IBATTERYSENSOR_H

#include <stdint.h>

/**
 * Interface for battery voltage sensor
 */
class IBatterySensor {
public:
  /**
   * Method to get battery voltage in ADC units
   * @return battery voltage in ADC units
   */
  virtual uint16_t get_battery_voltage_adc() = 0;

  /**
   * Method to get battery voltage in millivolts
   * @return battery voltage in millivolts
   */
  virtual uint16_t get_battery_voltage_mVDC() = 0;

  virtual ~IBatterySensor() = default;
};

#endif // IBATTERYSENSOR_H
