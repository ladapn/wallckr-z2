#ifndef SENSING_H
#define SENSING_H

#include "IDistanceSensor.h"
#include <stdint.h>

class RobotPrinter;

/**
 * Structure grouping all possible types of distance measurements, namely
 * right front distance (45 deg from front direction),
 * right center distance (90 deg from front direction),
 * right distance (minimum value of all right distances),
 * front distance
 */
struct DistanceMeasurements {
  unsigned long right_distance_cm = 0;
  unsigned long right_front_distance_cm = 0;
  unsigned long right_center_distance_cm = 0;
  unsigned long front_distance_cm = 0;
};

/**
 * Structure grouping all distance sensors used by the sensing module
 */
struct DistanceSensors {
  IDistanceSensor &front;
  IDistanceSensor &right_front;
  IDistanceSensor &right_center;
};

/**
 * Class encapsulating all distance measurement devices
 */
class Sensing {
  DistanceSensors sensors;

  RobotPrinter &sensor_printer;

  bool disabled;

public:
  /**
   * Constructor method
   * @param[in] front reference to front distance sensor
   * @param[in] right_front reference to right front sensor (45 deg from front
   * direction)
   * @param[in] right_center reference to right center sensor (90 deg from front
   * direction)
   * @param[in] printer reference to serial interface which is used to send out
   * measured data
   */
  Sensing(DistanceSensors &distance_sensors, RobotPrinter &printer)
      : sensors(distance_sensors), sensor_printer(printer), disabled(false) {};

  /**
   * Measure side distance to wall or obstacle, also obtained measurements are
   * sent out via serial interface
   * @param[in] current_millis current system tick
   * @param[out] right_front_distance_cm right front (45 deg from front
   * direction) distance in cm
   * @param[out] right_center_distance_cm right center (90 deg from front
   * direction) distance in cm
   * @return right distance in cm, right distance is the smaller one of those
   * two above
   */
  unsigned int get_side_distance_cm(uint32_t current_millis,
                                    unsigned long &right_front_distance_cm,
                                    unsigned long &right_center_distance_cm);
  /**
   * Measure front distance to wall or obstacle, also obtained measurement is
   * sent out via serial interface
   * @param[in] current_millis current system tick
   * @return front distance in cm
   */
  unsigned int get_front_distance_cm(uint32_t current_millis);
  /**
   * Get all possible distance measurements in form of DistanceMeasurements
   * structure, also obtained measurements are sent out via serial interface
   * @param[in] current_millis current system tick
   * @param[out] distance_measurements structure holding the measurements
   */
  void get_distance_measurements(uint32_t current_millis,
                                 DistanceMeasurements &distance_measurements);
  /**
   * Disable sensing module, when disable all measurement requests will be
   * ignored
   */
  void disable() { disabled = true; };

  /**
   * Enable sensing module, measurement requests will no longer be ignored
   */
  void enable() { disabled = false; };
};

#endif // SENSING_H
