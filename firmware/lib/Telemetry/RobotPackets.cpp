#include "RobotPackets.h"

namespace {
/**
 * Appends little-endian fields to a packet buffer
 */
class PacketWriter {
    etl::span<uint8_t> buffer;
    size_t offset = 0;

public:
    explicit PacketWriter(etl::span<uint8_t> buffer)
        : buffer(buffer)
    {
    }

    void append_uint8(uint8_t value) { buffer[offset++] = value; }

    void append_uint16_le(uint16_t value)
    {
        append_uint8(static_cast<uint8_t>(value));
        append_uint8(static_cast<uint8_t>(value >> 8));
    }

    void append_uint32_le(uint32_t value)
    {
        append_uint16_le(static_cast<uint16_t>(value));
        append_uint16_le(static_cast<uint16_t>(value >> 16));
    }
};
} // namespace

void SonarPacket::serialize(etl::span<uint8_t, SERIALIZED_SIZE> buffer) const
{
    PacketWriter writer(buffer);
    writer.append_uint8(id);
    writer.append_uint32_le(tick);
    writer.append_uint32_le(sonar_data);
    writer.append_uint8(0); // CRC is reserved for RobotPrinter - it will be overwritten there
}

void StatusPacket::serialize(etl::span<uint8_t, SERIALIZED_SIZE> buffer) const
{
    PacketWriter writer(buffer);
    writer.append_uint8(id);
    writer.append_uint32_le(tick);
    writer.append_uint32_le(version_ID);
    writer.append_uint16_le(battery_voltage_mvdc);
    writer.append_uint16_le(total_current_mA);
    writer.append_uint16_le(motor_current_mA);
    writer.append_uint8(0); // CRC is reserved for RobotPrinter - it will be overwritten there
}

void MotionCommandPacket::serialize(etl::span<uint8_t, SERIALIZED_SIZE> buffer) const
{
    PacketWriter writer(buffer);
    writer.append_uint8(id);
    writer.append_uint32_le(tick);
    writer.append_uint16_le(servo_cmd);
    // cast int16 to uint16 for little-endian bytes
    writer.append_uint16_le(static_cast<uint16_t>(motor_cmd));
    writer.append_uint8(0); // CRC is reserved for RobotPrinter - it will be overwritten there
}
