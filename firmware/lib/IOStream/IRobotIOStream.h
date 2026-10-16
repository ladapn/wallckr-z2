#ifndef IROBOTIOSTREAM_H
#define IROBOTIOSTREAM_H

#include <etl/span.h>
#include <stddef.h>
#include <stdint.h>

class IRobotIOStream {

public:
    virtual ~IRobotIOStream() = default;

    virtual int available() = 0;
    virtual int read() = 0;
    virtual size_t write(uint8_t c) = 0;
    virtual size_t write(etl::span<const uint8_t> data) = 0;
};

#endif
