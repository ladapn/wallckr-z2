#include "RobotPrinter.h"
#include "IRobotIOStream.h"

uint8_t RobotPrinter::compute_xor_CRC(const uint8_t* buffer, size_t size)
{
    uint8_t CRC = 0;

    if (size) {
        CRC = buffer[0];

        for (unsigned int i = 1; i < (size - 1); i++) {
            CRC ^= buffer[i];
        }
    }

    return CRC;
}

size_t RobotPrinter::write_serialized(uint8_t* buffer, size_t size)
{
    // Every packet has at least its CRC byte; print() never passes 0.
    if (size == 0) {
        return 0;
    }
    buffer[size - 1] = compute_xor_CRC(buffer, size);
    return robot_serial.write(buffer, size);
}
