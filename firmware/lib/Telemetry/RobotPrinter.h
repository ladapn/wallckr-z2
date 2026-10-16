#ifndef ROBOT_PRINTER_H
#define ROBOT_PRINTER_H

#include "IRobotIOStream.h"

#include <etl/array.h>
#include <etl/span.h>
#include <stddef.h>
#include <stdint.h>

class RobotPrinter {
    IRobotIOStream& robot_serial;

    static uint8_t compute_xor_CRC(etl::span<const uint8_t> data);

public:
    explicit RobotPrinter(IRobotIOStream& serial)
        : robot_serial(serial)
    {
    }

    template <typename PacketType>
    size_t print(const PacketType& packet)
    {
        static_assert(PacketType::SERIALIZED_SIZE > 1, "a packet holds at least its ID and CRC");

        etl::array<uint8_t, PacketType::SERIALIZED_SIZE> buffer { };
        packet.serialize(buffer);
        // The last byte is the CRC over all the others.
        buffer.back() = compute_xor_CRC(etl::span<const uint8_t>(buffer).first(buffer.size() - 1));

        return robot_serial.write(buffer);
    }
};

#endif
