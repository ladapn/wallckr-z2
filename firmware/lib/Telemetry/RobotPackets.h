#ifndef ROBOT_PACKETS_H
#define ROBOT_PACKETS_H

#include <stdint.h>
#include <stdlib.h>

/**
 * enum defining all possible IDs of ultrasound measurement packets
 */
enum us_packet_ids {
  FRONT_US_ID = 100,
  RIGHT_FRONT_US_ID = 101,
  RIGHT_CENTER_US_ID = 102,
  RIGHT_BACK_US_ID = 103
};
/**
 * enum defining all possible IDs of utility packets
 */
enum util_packet_ids { STATUS_ID = 80, MOTION_CMD_ID = 90 };

/**
 * Structure representing output packet to hold measurements from ultrasound
 * sensors
 */
struct SonarPacket {

  static constexpr size_t SERIALIZED_SIZE = 10;
  uint8_t id;
  uint32_t tick;
  uint32_t sonar_data;
  uint8_t crc = 0;
  /**
   * Constructor method
   * @param[in] packet_id ID of ultrasound packet, it differs based on sensor
   * location - see \ref us_packet_ids
   * @param[in] tick_ms current system time in ms
   * @param[in] measurement ultrasound measurement
   */
  SonarPacket(uint8_t packet_id, uint32_t tick_ms, uint32_t measurement)
      : id(packet_id), tick(tick_ms), sonar_data(measurement) {}

  /**
   * Serialize packet fields in the wire format. CRC field is reserved for
   * RobotPrinter.
   * @param[out] buffer chunk of data to hold serialized packet
   * @param[in] buffer_size size of buffer in bytes, it should be at least
   * SERIALIZED_SIZE
   * @return true if serialization was successful, false otherwise
   */
  bool serialize(uint8_t *buffer, size_t buffer_size) const;
};

/**
 * Structure representing status packet - packet holding information about
 * robot's status
 */
struct StatusPacket {
  static constexpr size_t SERIALIZED_SIZE = 16;
  uint8_t id = STATUS_ID;
  uint32_t tick;
  uint32_t version_ID = GIT_REV; // long -> 4 bytes, instead of 8 chars
  uint16_t battery_voltage_mvdc;
  uint16_t total_current_mA;
  uint16_t motor_current_mA;
  uint8_t crc = 0;

  /**
   * Constructor method
   * @param[in] tick_ms current system time in ms
   * @param[in] battery_voltage current battery voltage in mVDC units
   * @param[in] total_current total current drawn from battery, in mA
   * @param[in] motor_current current drawn by the motor, in mA (an
   * implausibly high value signals "not measured" - see Battery::is_ok)
   */
  StatusPacket(uint32_t tick_ms, uint16_t battery_voltage,
               uint16_t total_current, uint16_t motor_current)
      : tick(tick_ms), battery_voltage_mvdc(battery_voltage),
        total_current_mA(total_current), motor_current_mA(motor_current) {}

  /**
   * Serialize packet fields in the wire format. CRC field is reserved for
   * RobotPrinter.
   * @param[out] buffer chunk of data to hold serialized packet
   * @param[in] buffer_size size of buffer in bytes, it should be at least
   * SERIALIZED_SIZE
   * @return true if serialization was successful, false otherwise
   */
  bool serialize(uint8_t *buffer, size_t buffer_size) const;
};

/**
 * Structure representing motion command packet -> packet sent back when a
 * motion command is received from user
 */
struct MotionCommandPacket {
  static constexpr size_t SERIALIZED_SIZE = 10;
  uint8_t id = MOTION_CMD_ID;
  uint32_t tick;
  uint16_t servo_cmd;
  int16_t motor_cmd;
  uint8_t crc = 0;
  /**
   * Constructor method
   * @param[in] tick_ms current system time in ms
   * @param[in] servo_command desired servo angle in deg
   * @param[in] motor_command desired motor speed on scale from -255 to 255,
   * where 255 is full speed and negative numbers mean going backward
   */
  MotionCommandPacket(uint32_t tick_ms, uint16_t servo_command,
                      int16_t motor_command)
      : tick(tick_ms), servo_cmd(servo_command), motor_cmd(motor_command) {}

  /**
   * Serialize packet fields in the wire format. CRC field is reserved for
   * RobotPrinter.
   * @param[out] buffer chunk of data to hold serialized packet
   * @param[in] buffer_size size of buffer in bytes, it should be at least
   * SERIALIZED_SIZE
   * @return true if serialization was successful, false otherwise
   */
  bool serialize(uint8_t *buffer, size_t buffer_size) const;
};

#endif
