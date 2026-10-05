#include "bringup.h"

#include <zephyr/drivers/sensor.h>
#include <zephyr/shell/shell.h>

namespace {

const device* const encoder = DEVICE_DT_GET(DT_NODELABEL(encoder));

int cmd_encoder(const shell* sh, size_t, char**)
{
    if (int ret = sensor_sample_fetch(encoder)) {
        shell_error(sh, "Encoder read failed (%d)", ret);
        return ret;
    }

    sensor_value count;
    sensor_channel_get(encoder, SENSOR_CHAN_ENCODER_COUNT, &count);
    shell_print(sh, "encoder count: %d", count.val1);
    return 0;
}

} // namespace

int encoder_init()
{
    return device_is_ready(encoder) ? 0 : -ENODEV;
}

SHELL_CMD_REGISTER(encoder, NULL, "Read the raw quadrature encoder count", cmd_encoder);
