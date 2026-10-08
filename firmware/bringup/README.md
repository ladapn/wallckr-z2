# wallckr-z2 bring-up firmware

Standalone hardware bring-up/test firmware: a Zephyr shell on the ST-LINK
virtual COM port (115200 8N1) with one command per peripheral, so each part
of the motherboard and the Button/LED shield can be checked in isolation
during assembly, plus a minimal BLE link test. No `Robot` logic and no
ovladacka protocol.

## Build and flash

From the west workspace root (the parent of this repo):

```sh
west build -b nucleo_f401re --shield motherboard_shield \
    --shield x_nucleo_bnrg2a1 --shield buttonled_shield wallckr-z2/firmware/bringup
west flash
```

On boot the three status LEDs light in turn (STATE1, STATE2, ERR).

## Commands

| Command | What it checks |
|---|---|
| `led <state1\|state2\|err> <on\|off>`, `led_test` | Status LEDs on the Button/LED shield |
| `buttons` | Current button levels; presses/releases are also logged as they happen |
| `motor speed <-255..255>`, `motor stop`, `motor status` | DRV8874: EN PWM (25 kHz), PH direction, nFAULT. A fault is also logged when it occurs |
| `servo angle <0..180>`, `servo pulse <us>`, `servo off` | Steering servo. Angle maps to 544–2400 µs, same as v1's Arduino `Servo` |
| `sonar [1..4]` | HY-SRF05 distance, one sensor or all four (sensor N = connector J(N+1)) |
| `analog` | Battery voltage (PB1) and total supply current (PC5): raw ADC, pin voltage, scaled value |
| `encoder` | Raw quadrature count from TIM4 (wraps; counts down when reversing) |
| `ble status` | BLE link: connection time and parameters, connect/disconnect counts, last disconnect reason, heartbeats sent |

Lift the wheels off the ground before running `motor speed`.

## BLE link test

The firmware advertises as `wallckr-bringup` with the Nordic UART Service
(based on Zephyr's `peripheral_nus` sample). To check that the link survives
motor and servo activity:

1. Connect with a phone app that speaks NUS, e.g. nRF Connect or a "Bluetooth
   serial terminal" app, and enable notifications on the TX characteristic.
2. The board sends `tick <n>` once per second; a gap in the numbers means
   lost notifications. Text written to the RX characteristic is logged on the
   console.
3. Run the motor and servo from the shell while connected.
4. Any drop is logged with its reason (e.g. `Supervision Timeout`), and
   `ble status` keeps the counts. Advertising restarts automatically after a
   disconnect.

There is no pairing or encryption. The STM32F401 has no hardware random number
generator, so the Bluetooth stack uses Zephyr's timer-based test generator,
which is fine for this test but not for a secured link.
