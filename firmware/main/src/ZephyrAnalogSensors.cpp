#include "ZephyrAnalogSensors.h"

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(wallckr);

namespace {

int setup(const adc_dt_spec& channel)
{
    if (!adc_is_ready_dt(&channel)) {
        return -ENODEV;
    }
    return adc_channel_setup_dt(&channel);
}

// One conversion; raw and pin_mv are left untouched on failure.
int read(const adc_dt_spec& channel, int32_t& raw, int32_t& pin_mv)
{
    int16_t sample = 0;
    adc_sequence sequence = {
        .buffer = &sample,
        .buffer_size = sizeof(sample),
    };

    if (int ret = adc_sequence_init_dt(&channel, &sequence)) {
        return ret;
    }
    if (int ret = adc_read_dt(&channel, &sequence)) {
        return ret;
    }

    int32_t mv = sample;
    if (int ret = adc_raw_to_millivolts_dt(&channel, &mv)) {
        return ret;
    }
    raw = sample;
    pin_mv = mv;
    return 0;
}

// Readings are small positive values; clamp anyway rather than wrap.
uint16_t to_uint16(int64_t value)
{
    if (value < 0) {
        return 0;
    }
    return value > UINT16_MAX ? UINT16_MAX : static_cast<uint16_t>(value);
}

} // namespace

int ZephyrBatterySensor::init()
{
    return setup(channel);
}

uint16_t ZephyrBatterySensor::get_battery_voltage_adc()
{
    int32_t raw = 0;
    int32_t pin_mv = 0;
    if (int ret = read(channel, raw, pin_mv)) {
        LOG_ERR("Battery voltage read failed (%d)", ret);
    }
    return to_uint16(raw);
}

uint16_t ZephyrBatterySensor::get_battery_voltage_mVDC()
{
    int32_t raw = 0;
    int32_t pin_mv = 0;
    if (int ret = read(channel, raw, pin_mv)) {
        LOG_ERR("Battery voltage read failed (%d)", ret);
    }
    return to_uint16(pin_mv);
}

int ZephyrCurrentSensor::init()
{
    return setup(channel);
}

uint16_t ZephyrCurrentSensor::get_current_mA()
{
    int32_t raw = 0;
    int32_t pin_mv = 0;
    if (int ret = read(channel, raw, pin_mv)) {
        LOG_ERR("Current read failed (%d)", ret);
        return 0;
    }

    // I = V / (Rsense * gain), with Rsense in milliohms: mV / 1.65 on this
    // board (v1's circuit was 1 mV = 1 mA).
    return to_uint16(int64_t { pin_mv } * 1000 * gain_div / (int64_t { sense_resistor_milli_ohms } * gain_mult));
}
