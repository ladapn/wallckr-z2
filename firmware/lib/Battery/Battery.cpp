#include "Battery.h"
#include "RobotPackets.h"
#include "RobotPrinter.h"

namespace {
// There's no dedicated motor-current sensor on this hardware, only a
// high-side sensor for total robot current draw (see ICurrentSensor). 0
// would read as "zero current draw," which is misleading, so send an
// implausibly high mA value to signal "not measured" instead.
constexpr uint16_t MOTOR_CURRENT_NOT_MEASURED_mA = 0xFFFF;
} // namespace

bool Battery::is_ok(uint32_t current_millis) {
  // TODO: add filtering
  auto battery_voltage_mVDC = battery_sensor.get_battery_voltage_mVDC();

  StatusPacket status_data(current_millis, battery_voltage_mVDC,
                           current_sensor.get_current_mA(),
                           MOTOR_CURRENT_NOT_MEASURED_mA);

  battery_printer.print(status_data);

  if (battery_dead) {
    if (battery_voltage_mVDC >
        (cutoff_mVDC + hysteresis_mVDC)) // Add some hysteresis
    {
      battery_dead = false;
      return true;
    }

    return false;
  }

  if (battery_voltage_mVDC <= cutoff_mVDC) {
    battery_dead = true;
    return false;
  }

  return true;
}
