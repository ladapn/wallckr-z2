#ifndef SIMULATED_UART_H
#define SIMULATED_UART_H

#include "IRobotIOStream.h"

/**
 * This is a mock uart class created for testing purposes. It contains fixed
 * sized buffer to store characters to be returned during consecutive calls to
 * read().
 */
class SimulatedUART : public IRobotIOStream {
    static const int buffer_len_max = 100;
    char buffer[buffer_len_max];
    int buffer_index = 0;
    int data_len = 0;

public:
    SimulatedUART() { }
    /**
     * Constructor method
     * @param[in] in c-style string to feed internal buffer
     * @param[in] len length of the string
     */
    SimulatedUART(char* in, int len)
    {
        write(etl::span<const uint8_t>(reinterpret_cast<const uint8_t*>(in), len));
    }

    /**
     * Return number of items still available in internal buffer
     */
    int available(void) override { return data_len - buffer_index; }

    /**
     * Read next character from internal buffer, if end of data in buffer is
     * reached, return 0
     */
    int read(void) override
    {
        if (buffer_index < data_len) {
            return buffer[buffer_index++];
        } else {
            return 0;
        }
    }

    /**
     * Dummy implementation of single-byte write, does nothing
     */
    size_t write(uint8_t) override { return 0; }

    /**
     * Write into internal buffer
     * @param data data to be written
     */
    size_t write(etl::span<const uint8_t> data) override
    {
        data_len = (buffer_len_max < data.size()) ? buffer_len_max : data.size();
        for (int i = 0; i < data_len; i++) {
            buffer[i] = data[i];
        }

        return data_len;
    }
};

#endif // SIMULATED_UART_H
