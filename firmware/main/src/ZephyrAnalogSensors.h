#ifndef ZEPHYR_ANALOG_SENSORS_H
#define ZEPHYR_ANALOG_SENSORS_H

#include "IBatterySensor.h"
#include "ICurrentSensor.h"

#include <zephyr/drivers/adc.h>

/**
 * IBatterySensor on an ADC channel behind the battery voltage divider.
 * Reports mV at the ADC pin, as v1 did; the cutoff constants account for
 * the divider.
 */
class ZephyrBatterySensor : public IBatterySensor {
    adc_dt_spec channel;

public:
    explicit ZephyrBatterySensor(const adc_dt_spec& channel)
        : channel(channel)
    {
    }

    /**
     * Set up the ADC channel.
     * @return 0 on success, a negative errno otherwise
     */
    int init();

    /**
     * @return raw 12-bit ADC value, or 0 if the read fails
     */
    uint16_t get_battery_voltage_adc() override;

    /**
     * @return mV at the ADC pin, or 0 if the read fails, which Battery treats
     * as a dead battery and so stops the robot
     */
    uint16_t get_battery_voltage_mVDC() override;
};

/**
 * ICurrentSensor for the INA169 high-side monitor: its output current
 * through the load resistor gives a voltage proportional to the total robot
 * current.
 */
class ZephyrCurrentSensor : public ICurrentSensor {
    adc_dt_spec channel;
    uint32_t sense_resistor_milli_ohms;
    uint32_t gain_mult;
    uint32_t gain_div;

public:
    /**
     * @param[in] channel ADC channel on the INA169 output
     * @param[in] sense_resistor_milli_ohms sense resistor value
     * @param[in] gain_mult, gain_div voltage gain from the sense resistor to
     * the ADC pin, as a fraction (INA169 transconductance x load resistor)
     */
    ZephyrCurrentSensor(
        const adc_dt_spec& channel, uint32_t sense_resistor_milli_ohms, uint32_t gain_mult, uint32_t gain_div)
        : channel(channel)
        , sense_resistor_milli_ohms(sense_resistor_milli_ohms)
        , gain_mult(gain_mult)
        , gain_div(gain_div)
    {
    }

    /**
     * Set up the ADC channel.
     * @return 0 on success, a negative errno otherwise
     */
    int init();

    /**
     * @return total robot current in mA, or 0 if the read fails
     */
    uint16_t get_current_mA() override;
};

#endif // ZEPHYR_ANALOG_SENSORS_H
