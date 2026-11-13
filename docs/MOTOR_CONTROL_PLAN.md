# Closed-loop motor control plan

v1 drove the motor open loop: the speed command was the PWM duty, so the robot
slowed on inclines and as the battery sagged. wallckr-z2 has a quadrature
encoder on the motor (`motherboard_shield`'s TIM4, x4, 380 counts per gearbox
output revolution), so the speed can be regulated. This file is the order of
work (one PR per step) and the decisions taken for it. Tick a step off when its
PR merges.

## Design

- `Robot` and `Motion` stay as they are. The loop closes below the
  `IMotorController` interface:
  - `IWheelEncoder` (new, in `firmware/lib`): counts moved since the previous
    read.
  - `ClosedLoopMotorController` (new, in `firmware/lib`) is an
    `IMotorController` wrapping the open-loop one. `set_speed()` stores the
    target; `update()`, called once per tick, measures the speed and sets the
    PWM duty from a PI controller. `stop()` zeroes the target, the duty and the
    integrator, so a battery cutoff still stops the motor at once.
  - `main.cpp` wires the wrapper in place of `ZephyrMotorController` and calls
    `update()` on every tick.
- The PI controller clamps its output to the duty range (±255) and stops
  integrating while clamped (anti-windup).

## Decisions

- **Speed command:** ovladacka's ±255 stays, and now means a fraction of a
  maximum wheel speed: target = command × `V_MAX` / 255. Internally speeds are
  in encoder counts per second. `V_MAX` is set from the step test, a little
  below the speed the motor reaches at full duty on a low battery, so the loop
  keeps some headroom.
- **Loop rate:** the existing 100 ms tick, in the control thread. No new thread
  or locking. At 300 rpm on the gearbox output that is ~190 counts per tick.
  Revisit if the step test shows the motor responding much faster than 100 ms.
- **Telemetry:** a new packet, `MOTOR_SPEED_ID` = 91, sent every tick: tick,
  target and measured speed (counts/s, `int16_t`) and duty (`int16_t`), 12
  bytes with the CRC. ovladacka needs it in `packet_definition.json`.

## Steps

- [x] **1. `feat(bringup): add a motor step-response command`**
  - `motor step <duty> <ms>`: applies a PWM step and prints the encoder count
    every 10 ms, then stops the motor.
  - Gives the direction sign (forward must count up), the gain (speed per
    duty) and the time constant, which set `V_MAX` and the PI gains.
- [ ] **2. `feat(lib): add a PI speed controller`**, with Ztest cases for the
  proportional and integral terms, the clamp and the anti-windup.
- [ ] **3. `feat(lib): add the closed-loop motor controller`**:
  `IWheelEncoder`, `ClosedLoopMotorController`, and tests against a fake
  encoder and a fake motor.
- [ ] **4. `feat(main): close the motor speed loop`**: `ZephyrWheelEncoder`
  (qdec `SENSOR_CHAN_ENCODER_COUNT`; the driver counts 0–65,359 and wraps,
  so differences are taken modulo 65,360, which is 172 revolutions) and the
  wiring. Gains and `V_MAX` are placeholders, marked
  as untuned.
- [ ] **5. `feat(lib): send motor speed telemetry`**, plus the packet in
  ovladacka.
- [ ] **6. `feat(main): tune the motor speed loop`**: `V_MAX` and the PI gains
  from the step-test data, checked on the floor.
