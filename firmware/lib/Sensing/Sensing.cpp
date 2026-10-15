#include "Sensing.h"
#include "RobotPackets.h"
#include "RobotPrinter.h"

unsigned int Sensing::get_front_distance_cm(uint32_t currentMillis) {
  if (disabled) {
    return 0;
  }

  // Fire front sensor
  auto front_sonar_cm = sensors.front.get_distance_filtered_cm();

  SonarPacket sonar_packet(FRONT_US_ID, currentMillis, front_sonar_cm);

  sensor_printer.print(sonar_packet);

  return front_sonar_cm;
}

unsigned int
Sensing::get_side_distance_cm(uint32_t currentMillis,
                              unsigned long &right_front_distance_cm,
                              unsigned long &right_center_distance_cm) {
  if (disabled) {
    return 0;
  }

  // Fire right front sensor
  right_front_distance_cm = sensors.right_front.get_distance_raw_cm();
  auto right_sonar_cm = right_front_distance_cm;

  SonarPacket sonar_packet(RIGHT_FRONT_US_ID, currentMillis,
                           right_front_distance_cm);
  sensor_printer.print(sonar_packet);

  // Fire right center sensor
  right_center_distance_cm = sensors.right_center.get_distance_raw_cm();

  if (right_center_distance_cm < right_sonar_cm) {
    right_sonar_cm = right_center_distance_cm;
  }

  sonar_packet.id = RIGHT_CENTER_US_ID;
  sonar_packet.tick = currentMillis;
  sonar_packet.sonar_data = right_center_distance_cm;
  sensor_printer.print(sonar_packet);

  return right_sonar_cm;
}

void Sensing::get_distance_measurements(
    uint32_t current_millis, DistanceMeasurements &distance_measurements) {

  distance_measurements.front_distance_cm =
      get_front_distance_cm(current_millis);
  distance_measurements.right_distance_cm = get_side_distance_cm(
      current_millis, distance_measurements.right_front_distance_cm,
      distance_measurements.right_center_distance_cm);
}
