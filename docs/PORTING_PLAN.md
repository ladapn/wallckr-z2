# Porting plan: wallckr v1 → wallckr-z2

How the v1 firmware (https://github.com/ladapn/wallckr) gets ported to
`firmware/main`, as a sequence of PRs. `CLAUDE.md` has the background
(interfaces, scheduling decision, constants); this file is the order of work
and the decisions made along the way. Tick a step off when its PR merges.

## Layout

```
firmware/lib/     v1's target-independent folders, same names: Battery, Control,
                  Communication, IOStream, Motion, Robot, Sensing, Telemetry;
                  CMakeLists.txt builds them as a library apps and tests link
firmware/main/    the vehicle app: prj.conf, CMakeLists.txt, VERSION,
                  src/ with the Zephyr adapters and main.cpp
firmware/tests/   one Ztest suite per v1 test suite, a fake IRobotIOStream in common/
```

- Adapters keep v1's naming with `Zephyr` in place of `Arduino`
  (`ZephyrMotorController`, `ZephyrUltrasonicSensor`, ...).
- `lib/Control/TimeManager.h` and its test are not copied: the control thread
  replaces it (see "Scheduling" in `CLAUDE.md`).

## Steps

- [x] **1. `feat(lib): import v1's target-independent library`**
  - Commits:
    - verbatim copy of v1's portable folders;
    - reformat only;
    - `CMakeLists.txt`, building them as a library;
    - firmware version from Zephyr's `VERSION` file (see below);
    - the v1 suites rewritten against Ztest: battery, command decoder,
      robot packets, auto-steering, filter, regulator, robot;
    - a `west twister -p native_sim -T firmware/tests` CI job.
  - Library and tests land together: until `firmware/main` exists, the tests
    are the only thing that compiles `lib/` in CI.
- [x] **2. `feat(main): add the vehicle firmware skeleton`**
  - `prj.conf` (C++23, minimal libcpp, ETL, BT/NUS, GPIO, PWM, ADC), a `main.cpp`
    that only boots and logs, `VERSION`, and linking `lib/`.
  - CI: `main` joins the build matrix, and clang-tidy is extended to
    `firmware/lib`. This is the first board build that compiles `lib/`. Small
    behavior-neutral fixes clang-tidy asks for are accepted.
- [ ] **3. `feat(main): add the motor and servo adapters`**
  - Motor (DRV8874): EN is PWM at 25 kHz with duty = |speed| / 255, PH sets the
    direction. Out-of-range speeds return `false`. `stop()` sets the duty to 0
    and leaves PH alone, as in v1. nFAULT is ignored, as v1 had nothing like it.
  - Servo: 0–180° maps to 544–2400 µs, as in bring-up and v1's Arduino `Servo`.
- [ ] **4. `feat(main): add the ultrasonic distance adapter`**
  - Echo edges timestamped from the GPIO interrupt, as in bring-up, but the
    wait stops at ~12 ms (200 cm × 57 µs/cm). A timeout or missing echo
    reports 200 cm, as v1 did with NewPing's 0.
  - A falling edge arriving after the timeout must be ignored, so it can't
    complete the next ping.
  - The filtered reading uses `ExpFilter<int, 4>` inside the adapter, as v1's
    `ArduinoUltraSoundSensor` does.
  - Worst case for three sensors is ~36 ms of the 100 ms tick.
- [ ] **5. `feat(main): add the battery and current adapters`**
  - Battery: mV at the ADC pin (`adc_raw_to_millivolts_dt`), keeping v1's
    meaning so `Battery` works unchanged. `get_battery_voltage_adc()` now
    returns 12-bit raw values; only test stubs call it.
  - Current: pin mV × 20 / 33 (÷ 1.65 in integer maths).
- [ ] **6. `feat(main): add the status LED adapter`**
  - Same logic as v1: STATE1/STATE2 show following/avoiding, ERR toggles on
    every battery warning (v1's 0.5 Hz blink). Polarity comes from the
    devicetree flags.
- [ ] **7. `feat(main): add the NUS command and telemetry stream`**
  - The NUS receive callback copies bytes into a spinlock-guarded ring buffer
    and posts COMMAND; `available()`/`read()` drain it from the control thread.
  - `write()` calls `bt_nus_send(NULL, ...)` and drops the packet when nobody
    is subscribed. Packets are 10 or 16 bytes, so each fits in one
    notification at the default MTU.
  - Advertises as `wallckr`, the name v1 set with `AT NAME`.
- [ ] **8. `feat(main): wire the robot and control thread`**
  - Builds the objects the way v1's `loop()` does.
  - One thread waits on TICK (a 100 ms `k_timer`) or COMMAND. Every wake
    drains the command stream first. On TICK it runs
    `perform_automatic_action()`, plus `perform_status_check()` on ticks
    0, 10, 20, ..., so the first check runs at boot as in v1. Every wake ends
    with `apply_motion_command()`.
  - Constants (see below).
  - First hardware check of the whole port: wall following and avoiding,
    ovladacka control, low-battery cutoff from a bench supply.

Steps 3–7 are separate PRs for easier review. None of them can be checked on
hardware until step 8 wires them in.

## Decisions

- **Battery thresholds** are in mV at the ADC pin, and the divider changed from
  ÷16 to ÷5:
  - cutoff: 900 mV/cell × 8 cells / 5 = 1440 mV;
  - hysteresis: 160 mV, which keeps v1's 800 mV at the pack (v1's 50 mV × 16).
- **Steering:** P regulator, gain 5, setpoint 25 cm, dead band 2 cm, limits
  +45/−30; servo and speed constants in `MotionConstants.h` unchanged until
  they're checked on hardware.
- **Firmware version:** v1 sent the git short hash (`GIT_REV`, from
  `git_rev_macro.py`) as `StatusPacket::version_ID`. wallckr-z2 uses Zephyr's
  application `VERSION` file instead: the library's `CMakeLists.txt` passes the build's
  `APPVERSION` (`0xMMmmppTT`: major, minor, patch, tweak) to `lib/` as a
  compile definition, so `lib/` still includes no Zephyr headers. Builds
  without a `VERSION` file (the tests) get 0. ovladacka shows this field, so
  it needs to read it as a version rather than a hash.
- **Command draining:** `OvladackaParser` handles at most 10 characters per
  call. That was harmless when v1 called it on every loop pass; with an
  event-driven thread, leftover bytes would wait for the next tick. So the
  thread calls `check_external_command()` until the stream is empty.
- **Kept from v1 as is:**
  - after a battery cutoff, `Motion` and `Sensing` stay disabled until reset,
    even if the voltage recovers;
  - telemetry is sent from the first tick; with no connection it's dropped.

## After the port

These are the next steps once the port is done, not part of it:

- closed-loop speed control using the encoder;
- button behavior (mode switching, calibration, debug);
- the spare J5 sensor;
- BLE pairing and encryption.
