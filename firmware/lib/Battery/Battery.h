#ifndef BATTERY_H
#define BATTERY_H

#include "IBatterySensor.h"
#include "ICurrentSensor.h"
#include <stdint.h>

class RobotPrinter;

/**
 * Class encapsulating battery health monitoring. Decides if the battery pack
 * voltage is within an acceptable range, with hysteresis around the cutoff
 * threshold to avoid flickering when the voltage sits close to it. Cutoff and
 * hysteresis are constructor parameters so different battery packs (cell
 * count, chemistry) can be supported without changing this class.
 */
class Battery {
  IBatterySensor &battery_sensor;
  ICurrentSensor &current_sensor;
  RobotPrinter &battery_printer;

  uint16_t cutoff_mVDC;
  uint16_t hysteresis_mVDC;

  bool battery_dead = false;

public:
  /**
   * Constructor method
   * @param[in] sensor reference to battery voltage sensor
   * @param[in] current_sensor reference to the high-side current sensor
   * @param[in] printer reference to serial interface which is used to send
   * out measured data
   * @param[in] cutoff_mVDC pack voltage threshold, in the same units returned
   * by IBatterySensor::get_battery_voltage_mVDC, below which the battery is
   * considered dead
   * @param[in] hysteresis_mVDC voltage above cutoff_mVDC the pack has to
   * recover to before being considered ok again
   */
  Battery(IBatterySensor &sensor, ICurrentSensor &current_sensor,
          RobotPrinter &printer, uint16_t cutoff_mVDC, uint16_t hysteresis_mVDC)
      : battery_sensor(sensor), current_sensor(current_sensor),
        battery_printer(printer), cutoff_mVDC(cutoff_mVDC),
        hysteresis_mVDC(hysteresis_mVDC) {}

  /**
   * Check if battery voltage is above threshold (cutoff_mVDC), also obtained
   * measurement is sent out via serial interface
   * @param[in] current_millis current system tick
   * @return true if battery voltage is above threshold, false otherwise
   */
  bool is_ok(uint32_t current_millis);
};

#endif // BATTERY_H
