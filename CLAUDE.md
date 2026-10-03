# CLAUDE.md

Guidance for AI-assisted (Claude Code) work in this repo — porting wallckr (v1, Arduino/C++) to wallckr-z2 (Zephyr).

## Read first

- `docs/DESIGN_NOTES.md` — hardware/firmware design reference.
  - For firmware porting work, read the **"Nucleo F401RE Pin Assignment"**, **"Connectors"**, and **"Firmware"** sections.
  - Skip **"Design Decisions & Rationale"** and **"Power Architecture"** unless the task touches analog sensing or power — that's PCB/analog context, not needed for firmware work.
  - Check **"Known Open Items"** before assuming a hardware detail is final.
- v1 firmware (Arduino/C++), the source of truth for *behavior* being ported: https://github.com/ladapn/wallckr — design notes cover the hardware side only, not what the firmware actually does.
- v1's porting guide: https://github.com/ladapn/wallckr/blob/master/PORTING.md — defines the hardware abstraction boundary (which interfaces to reimplement vs. reuse unchanged). Read this before starting any porting work; summarized below so it doesn't have to be re-fetched every session, but the original is the source of truth if it's updated.
- v1's README feature list — summarized in "Features to preserve" below; describes the behavior the port needs to keep, not just the code structure.

## Toolchain

- **RTOS:** Zephyr, pinned to latest stable (4.4.0 as of writing — reconfirm at https://docs.zephyrproject.org/latest/releases/ before assuming, since a new stable ships roughly every April/October).
- **Board:** no custom board — base board is the existing upstream `nucleo_f401re`. Everything wallckr-z2 adds is modeled as Zephyr shields instead of a custom board, combined at build time via multiple `--shield` args:
  - `x_nucleo_bnrg2a1` — the BLE shield. Already exists upstream in Zephyr; nothing to write for this one.
  - `motherboard_shield` (new) — Morpho-connector shield for the custom motherboard (motor, sensors, servo, power/current sensing, encoder). Confirmed: `nucleo_f401re`'s devicetree already exposes the `st_morpho_header` GPIO nexus node, so this shield can reference Morpho pins the normal way — no extra devicetree work needed just to get the connector recognized.
  - `buttonled_shield` (new) — Arduino-connector shield stacking on top of `x_nucleo_bnrg2a1`, for the Button/LED shield.
  - Custom shields live under `firmware/boards/shields/<name>/` (`<name>.overlay` + `Kconfig.shield` + `Kconfig.defconfig` each), auto-discovered the same way a custom board would be — via the repo-root `zephyr/module.yml`'s `board_root: firmware` (Zephyr resolves both boards and shields under `<board_root>/boards/`).
- **Apps:** `firmware/main` (vehicle firmware) and `firmware/bringup` (hardware bring-up/test firmware) are independent Zephyr applications, each with its own `CMakeLists.txt`/`prj.conf`:
  - `west build -b nucleo_f401re --shield motherboard_shield --shield x_nucleo_bnrg2a1 --shield buttonled_shield firmware/main`
  - same, with `firmware/bringup` in place of `firmware/main`
  - `west flash`
- **Shared code:** `firmware/lib/` holds the target-independent logic reused from v1's `lib/` (`Robot`, control, battery/sensing logic, protocol decoding, and the six interfaces). It must not include Zephyr or board headers, so it can be built by `main`, `bringup` and the Ztest suites alike. Zephyr-specific implementations of the interfaces live in the app that uses them, not in `lib/`.
- **Tests:** Ztest suites live in `firmware/tests/`, run on `native_sim` with `west twister -p native_sim -T firmware/tests`.

## Language & style

- **C++23.** Enable per-app in `prj.conf`: `CONFIG_CPP=y`, `CONFIG_STD_CPP23=y`. Confirm these Kconfig symbols still exist for whatever Zephyr version is actually pinned (`CONFIG_STD_CPP23` is fairly recent, present by v4.2 — don't assume it on an older release).
- **C++ standard library:** default to Zephyr's Minimal C++ Library (`CONFIG_MINIMAL_LIBCPP=y` — one option of the `LIBCPP_IMPLEMENTATION` Kconfig choice, and also its default) rather than full libstdc++, to keep flash/RAM footprint down — this pairs naturally with ETL rather than fighting it. Exceptions and RTTI off (`CONFIG_CPP_EXCEPTIONS=n`, `CONFIG_CPP_RTTI=n`) unless something concrete needs them.
- **Containers/utilities:** Embedded Template Library (ETL) — https://github.com/ETLCPP/etl. No heap allocation; doesn't require exceptions (falls back to an error-handler callback instead of throwing). Not bundled with Zephyr — needs adding as an external dependency (west module, or CMake `FetchContent`/submodule) rather than assumed present.
- **Code style:** WebKit style (https://webkit.org/code-style-guidelines/). Enforce via a `.clang-format` at the repo root rather than by manual review.

## Design principles

KISS, YAGNI, and SOLID govern implementation choices here — concretely, for this port:

- **YAGNI** — don't add configurability, abstraction, or generality beyond what wallckr-z2 actually needs right now. The closed-loop-control follow-on (see "Motor control" below) stays out of the initial port rather than being pre-built as a toggleable option; a second board variant isn't a design concern until one is actually planned.
- **KISS** — prefer the simplest implementation that satisfies the interface and the behavior being preserved over a "more flexible" one nobody asked for. This is also the reasoning behind Minimal C++ Library + ETL over a heavier alternative, and `native_sim`-only testing over chasing full hardware emulation.
- **SOLID** — v1's PORTING.md already gives a clean interface boundary (the six interfaces below); keep to it rather than growing new cross-cutting abstractions. Each target-specific implementation (e.g. the Zephyr `IMotorController`) should depend only on what it needs to do its one job.

When a design choice trades simplicity for flexibility, state the tradeoff rather than picking silently — these are a strong default, not a substitute for judgment.

## Notes for the port

- No dynamic allocation on the MCU beyond what Zephyr's kernel objects already use internally — this is the reasoning behind ETL + Minimal C++ Library over full STL.
- If a hardware detail in the design notes conflicts with the physical board or KiCad schematic, the physical board/schematic wins — flag the discrepancy back rather than guessing which is current.

## Porting reference (from v1's PORTING.md)

Reimplement these six target-specific interfaces for Zephyr; everything else in v1's `lib/` (control logic, protocol decoding, the `Robot` state machine) is meant to be reused unchanged:

- `IMotorController`, `ISteeringServo` — currently `lib/MotionArduino` (`<Arduino.h>`, `<Servo.h>`)
- `IBatterySensor` — currently `lib/BatteryArduino` (Arduino ADC read)
- `IDistanceSensor` — currently `lib/SensingArduino` (`<NewPing.h>`)
- `IRobotIndicators` — currently `lib/CommunicationArduino`
- `IRobotIOStream` — used both for the BLE command stream and outgoing telemetry, also currently in `lib/CommunicationArduino`
- Pin assignments (`lib/BoardConfig`) — replaced by the shield devicetree overlays under `firmware/boards/shields/` (`motherboard_shield`, `buttonled_shield`)

Write a new entry point wiring these into `Robot` the way v1's `src/main.cpp` does.

**Known wrinkles carried over from v1's own porting notes — worth re-reading before assuming a straight port:**
- `lib/Motion/MotionConstants.h` (servo min/max/center, max speed) and the battery cell-count/cutoff constants in `src/main.cpp` are target-independent *files* but V1-specific *values* — they'll compile and pass tests unchanged, but need re-tuning for wallckr-z2's actual servo/motor/battery.
- `TimeManager` and the `PollingRobotRunner` super-loop (`while(true)` + manual millis-based polling) are a bare-metal scheduling stand-in, not something to port as-is — on Zephyr, timer callbacks, work queues, or dedicated threads calling `robot.perform_status_check()` / `perform_automatic_action()` at the right periods are the natural replacement.
- v1's tests use PlatformIO + Unity on a `native` host build; Zephyr should use Ztest + Twister instead. Assert macros translate roughly 1:1, but test registration differs enough (manual `RUN_TEST()` vs. auto-discovered `ZTEST()`) that porting a test file means rewriting it against Ztest, not sharing the file between frameworks.

## Features to preserve (from v1's README)

- **Wall following** — holds a target distance from the right-hand wall using a steering-angle controller driven by the front-right and right distance sensors. P controller is the default; PD also exists but isn't used by default.
- **Obstacle avoidance** — entered when the front or front-right sensor reads below its threshold, exited back to wall-following once both clear their own (larger) recovery thresholds — i.e. front > 60cm **and** front-right > 25cm. (v1's README currently says "less than" for the recovery condition; that's a typo being fixed upstream, not the actual behavior — go with "greater than.")
- **Motor control** — open-loop by design in v1 (no speed feedback, so it slows on inclines or as the battery sags). wallckr-z2 adds a quadrature encoder v1 never had; closed-loop speed control is the planned **first new feature after the port** — keep the port itself open-loop/behavior-matching, and treat closed-loop control as separate follow-on work rather than folding it into the initial port.
- **BLE communication** — via the ovladacka protocol: accepts remote commands, sends back distance measurements and status telemetry.
- **Status LEDs** — v1 drove 5 LEDs on its connection shield (LED1 = wall-following, LED2 = obstacle-avoiding, LED3/4 = always unused, LED5 = blinking low-battery warning) plus 1 green "alive" LED on the power board. wallckr-z2's Button/LED shield has only 3 MCU-controlled LEDs plus the fixed power LED — LED3/4 were unused in v1 too, so nothing is actually lost. Mapping: `LED_STATE1`↔LED1, `LED_STATE2`↔LED2, `LED_ERR`↔LED5, `LED_PWR`↔v1's power LED.
- **Buttons** — not a v1 feature: v1's PORTING.md doesn't include a button/input interface because v1 never actually used its buttons. Treat button behavior (mode switching, calibration, debug) as new functionality to design for wallckr-z2, not something being ported from existing v1 logic.

## CI (from v1's `.github/workflows/build.yml`)

Confirmed from the actual workflow (runs on `pull_request` only, `ubuntu-26.04`). Four gates to preserve, each needing a Zephyr-appropriate equivalent rather than a literal command swap:

- **Build** — `pio run -e megaatmega2560`. Zephyr equivalent: `west build` for both `firmware/main` and `firmware/bringup` — both should gate CI, not just one, since they're independent applications now.
- **Tests** — `pio test --without-uploading`, which ran *two* test environments in v1: a host-only `native` one (target-independent `lib/` code, per PORTING.md's Testing section above) **and** an AVR-target one executed under **simavr emulation** (catches things a pure host build can't — real memory layout, real toolchain codegen, target-specific code). Decision: wallckr-z2 sticks to **Twister on `native_sim` only** — no emulated-target equivalent to simavr. This is a real reduction in coverage for target-specific code (no substitute for actually flashing `firmware/bringup` and checking on hardware), accepted as a reasonable tradeoff rather than chasing an equivalent that doesn't cleanly exist for STM32F401RE.
- **Static analysis** — `pio check --skip-packages -e megaatmega2560`, scoped to the real target build. Replacement: **clang-tidy**, scoped to the real board build (not just `native_sim`), covering `firmware/main` and `firmware/bringup` `.cpp`/`.h` sources the same way v1 scoped to `megaatmega2560`. Needs a `.clang-tidy` config at the repo root and a `compile_commands.json` — Zephyr/CMake can generate that (`west build` passes through to CMake, which supports `CMAKE_EXPORT_COMPILE_COMMANDS=ON`) so clang-tidy sees the actual Zephyr include paths/defines rather than guessing them.
- **Code formatting** — `clang-format==22.1.5 --dry-run -Werror` over every `.cpp`/`.h` in `lib`, `src`, `test`. Carry this gate over as-is, just pointed at wallckr-z2's paths and a WebKit-style `.clang-format` instead of v1's config. Worth pinning the same clang-format version (22.1.5) unless there's a specific reason to bump it — keeps formatting-rule drift out of the picture while porting.
