# wallckr-z2 bring-up firmware

Standalone hardware bring-up/test firmware: a Zephyr shell on the ST-LINK
virtual COM port (115200 8N1) with one command per peripheral, so each part
of the motherboard and the Button/LED shield can be checked in isolation
during assembly. No BLE stack, no `Robot` logic.

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
| `analog` | Battery voltage (PB1) and motor current (PC5): raw ADC, pin voltage, scaled value |
| `encoder` | Raw quadrature count from TIM4 (wraps; counts down when reversing) |

Lift the wheels off the ground before running `motor speed`.
