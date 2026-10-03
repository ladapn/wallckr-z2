#include "bringup.h"

#include <etl/array.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/shell/shell.h>

namespace {

#define BATTERY_NODE DT_NODELABEL(battery_voltage)
#define CURRENT_NODE DT_NODELABEL(supply_current)

const adc_dt_spec battery = ADC_DT_SPEC_GET(BATTERY_NODE);
const adc_dt_spec current = ADC_DT_SPEC_GET(CURRENT_NODE);

struct Reading {
    int32_t raw;
    int32_t pin_mv;
};

int read(const adc_dt_spec& channel, Reading& reading)
{
    int16_t sample;
    adc_sequence sequence = {
        .buffer = &sample,
        .buffer_size = sizeof(sample),
    };

    if (int ret = adc_sequence_init_dt(&channel, &sequence); ret) {
        return ret;
    }
    if (int ret = adc_read_dt(&channel, &sequence); ret) {
        return ret;
    }

    reading.raw = sample;
    reading.pin_mv = sample;
    return adc_raw_to_millivolts_dt(&channel, &reading.pin_mv);
}

int32_t battery_mv(int32_t pin_mv)
{
    return static_cast<int32_t>(
        int64_t { pin_mv } * DT_PROP(BATTERY_NODE, full_ohms) / DT_PROP(BATTERY_NODE, output_ohms));
}

int32_t current_ma(int32_t pin_mv)
{
    // I = V / (Rsense * gain), with Rsense in milliohms.
    return static_cast<int32_t>(int64_t { pin_mv } * 1000 * DT_PROP(CURRENT_NODE, sense_gain_div)
        / (DT_PROP(CURRENT_NODE, sense_resistor_milli_ohms) * DT_PROP(CURRENT_NODE, sense_gain_mult)));
}

int cmd_analog(const shell* sh, size_t, char**)
{
    Reading reading;

    if (int ret = read(battery, reading); ret) {
        shell_error(sh, "Battery voltage read failed (%d)", ret);
        return ret;
    }
    shell_print(sh, "battery: %d mV (raw %d, pin %d mV)", battery_mv(reading.pin_mv), reading.raw,
        reading.pin_mv);

    if (int ret = read(current, reading); ret) {
        shell_error(sh, "Current read failed (%d)", ret);
        return ret;
    }
    shell_print(sh, "current: %d mA (raw %d, pin %d mV)", current_ma(reading.pin_mv), reading.raw,
        reading.pin_mv);
    return 0;
}

} // namespace

int analog_init()
{
    for (const auto* channel : etl::array { &battery, &current }) {
        if (!adc_is_ready_dt(channel)) {
            return -ENODEV;
        }
        if (int ret = adc_channel_setup_dt(channel); ret) {
            return ret;
        }
    }
    return 0;
}

SHELL_CMD_REGISTER(analog, NULL, "Read battery voltage and total supply current", cmd_analog);
