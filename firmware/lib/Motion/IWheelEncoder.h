#ifndef IWHEEL_ENCODER_H
#define IWHEEL_ENCODER_H

#include <stdint.h>

/**
 * Interface for the drive motor's quadrature encoder
 */
class IWheelEncoder {
public:
    /**
     * Read how far the motor turned since the previous call
     * @param[out] counts encoder counts moved, positive when driving forward
     * (the direction IMotorController::set_speed() takes as positive)
     * @return true on success, false if the encoder could not be read; counts
     * is then left unchanged
     */
    virtual bool read_counts(int32_t& counts) = 0;
    virtual ~IWheelEncoder() = default;
};

#endif // IWHEEL_ENCODER_H
