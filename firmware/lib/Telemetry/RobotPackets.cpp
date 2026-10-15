#include "RobotPackets.h"

namespace {
void append_uint8(uint8_t *buffer, size_t &offset, uint8_t value) {
  buffer[offset++] = value;
}

void append_uint16_le(uint8_t *buffer, size_t &offset, uint16_t value) {
  buffer[offset++] = static_cast<uint8_t>(value);
  buffer[offset++] = static_cast<uint8_t>(value >> 8);
}

void append_uint32_le(uint8_t *buffer, size_t &offset, uint32_t value) {
  buffer[offset++] = static_cast<uint8_t>(value);
  buffer[offset++] = static_cast<uint8_t>(value >> 8);
  buffer[offset++] = static_cast<uint8_t>(value >> 16);
  buffer[offset++] = static_cast<uint8_t>(value >> 24);
}
} // namespace

bool SonarPacket::serialize(uint8_t *buffer, size_t buffer_size) const {
  if (buffer == nullptr || buffer_size < SERIALIZED_SIZE) {
    return false;
  }

  size_t offset = 0;
  append_uint8(buffer, offset, id);
  append_uint32_le(buffer, offset, tick);
  append_uint32_le(buffer, offset, sonar_data);
  append_uint8(
      buffer, offset,
      0); // CRC is reserved for RobotPrinter - it will be overwritten there

  return true;
}

bool StatusPacket::serialize(uint8_t *buffer, size_t buffer_size) const {
  if (buffer == nullptr || buffer_size < SERIALIZED_SIZE) {
    return false;
  }

  size_t offset = 0;
  append_uint8(buffer, offset, id);
  append_uint32_le(buffer, offset, tick);
  append_uint32_le(buffer, offset, version_ID);
  append_uint16_le(buffer, offset, battery_voltage_mvdc);
  append_uint16_le(buffer, offset, total_current_mA);
  append_uint16_le(buffer, offset, motor_current_mA);
  append_uint8(
      buffer, offset,
      0); // CRC is reserved for RobotPrinter - it will be overwritten there

  return true;
}

bool MotionCommandPacket::serialize(uint8_t *buffer, size_t buffer_size) const {
  if (buffer == nullptr || buffer_size < SERIALIZED_SIZE) {
    return false;
  }

  size_t offset = 0;
  append_uint8(buffer, offset, id);
  append_uint32_le(buffer, offset, tick);
  append_uint16_le(buffer, offset, servo_cmd);
  append_uint16_le(
      buffer, offset,
      static_cast<uint16_t>(
          motor_cmd)); // cast int16 to uint16 for little-endian bytes
  append_uint8(
      buffer, offset,
      0); // CRC is reserved for RobotPrinter - it will be overwritten there

  return true;
}
