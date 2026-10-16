#include "RobotPrinter.h"

uint8_t RobotPrinter::compute_xor_CRC(etl::span<const uint8_t> data)
{
    uint8_t CRC = 0;

    for (auto byte : data) {
        CRC ^= byte;
    }

    return CRC;
}
