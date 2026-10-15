#ifndef IDISTANCESENSOR_H
#define IDISTANCESENSOR_H

/**
 * Interface for sensors measuring distance
 */
class IDistanceSensor {
public:
  virtual ~IDistanceSensor() = default;

  /**
   * Command sensor to measure distance
   * @return true if measurement was successful, false otherwise
   */
  virtual bool trigger_measurement() = 0;
  /**
   * Command sensor to measure distance and return unfiltered measurement
   * @return unfiltered distance measurement in cm
   */
  virtual unsigned long get_distance_raw_cm() = 0;
  /**
   * Command sensor to measure distance and return filtered measurement
   * @return filtered distance measurement in cm
   */
  virtual unsigned long get_distance_filtered_cm() = 0;
};

#endif // IDISTANCESENSOR_H
