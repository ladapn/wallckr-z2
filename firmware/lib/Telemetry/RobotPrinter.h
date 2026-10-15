#ifndef ROBOT_PRINTER_H
#define ROBOT_PRINTER_H

#include <stddef.h>
#include <stdint.h>

class IRobotIOStream;

class RobotPrinter {
  IRobotIOStream &robot_serial;

  uint8_t compute_xor_CRC(const uint8_t *buffer, size_t size);
  size_t write_serialized(uint8_t *buffer, size_t size);

public:
  explicit RobotPrinter(IRobotIOStream &serial) : robot_serial(serial) {}

  template <typename PacketType> size_t print(const PacketType &packet) {
    uint8_t buffer[PacketType::SERIALIZED_SIZE];
    if (!packet.serialize(buffer, PacketType::SERIALIZED_SIZE)) {
      return 0;
    }

    return write_serialized(buffer, PacketType::SERIALIZED_SIZE);
  }
};

#endif
