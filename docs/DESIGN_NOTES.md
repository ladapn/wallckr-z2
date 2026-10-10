# wallckr-z2 — Design Notes

## Project Summary

**wallckr-z2** — next generation of [wallckr](https://github.com/ladapn/wallckr) — a small indoor autonomous vehicle
with Ackermann (car-like) steering that follows walls and avoids obstacles.

> **Name:** "wallckr" = "wall" + "walker" with vowels dropped. "z2" = Zephyr RTOS (Z) + generation 2.

**v1 was built on:**
- Arduino Mega 2560
- L298P Motor Shield
- 3x HY-SRF05 ultrasonic sensors (front, front-right, right)
- Hitec HS-422 servo (steering)
- GM37 300 RPM DC motor
- HM-10 BLE module
- Custom power distribution PCB + custom IO extension PCB

**v2 goals:**
- Replace the messy v1 stack (Arduino Mega + L298P shield + two custom PCBs) with a clean intentional stack: custom motherboard + Nucleo F401RE + X-NUCLEO-BNRG2A1 (all designed to fit together)
- Upgrade brain to STM32-based Nucleo F401RE (plugs into motherboard as a shield)
- Cleaner power architecture with dedicated voltage regulators
- Proper H-bridge motor driver on-board
- Closed-loop motor control via custom quadrature encoder (Hall probes)
- Same sensors and servo as v1, BLE via X-NUCLEO-BNRG2A1 shield — motivated in part by ditching the HM-10 BLE module from v1, which was unreliable

> **Fabrication note:** Hobby project — mixed THT and SMD, hand-soldered. Parts sourced from gme.cz and farnell.cz. Prefer hand-solderable packages where there's a choice; SMD passives/small-signal parts in 0805 or larger are generally comfortable, smaller packages (SOD-323, SOT-23) are workable by hand with practice but worth flagging when chosen.

---

## Hardware — Key Components

| Component | Part | Notes |
|---|---|---|
| MCU board | ST Nucleo F401RE | Plugs into motherboard via Morpho connectors (CN7, CN10) |
| Motor driver | DRV8874 (Pololu #4035) | Single channel 37V/2.1A H-bridge. **Off-board carrier board** — motherboard provides female headers (J6, J9) that the Pololu carrier plugs into; the DRV8874 IC itself is not soldered onto the motherboard. Carrier has two 7-pin header rows (motor side / logic side), confirmed against physical board. **Control mode: PH/EN** (PMODE tied low) — single PWM on EN for speed, PH for direction. |
| Current sensing | INA169 (U1) + Rsense1 100mΩ | High-side monitor of total robot current, output into R3 (16k5) → PC5. See Design Decisions |
| Voltage regulator 1 | L7805 | 5V for sensors — TO-220, no heatsink (v1 ran without one too); will evaluate under real operation rather than pre-emptively adding one |
| Voltage regulator 2 | L7806 | 6V for servo — lower dropout vs 5V rail saves heat |
| Drive motor | GM37 300 RPM DC | Carried over from v1 |
| Steering servo | Hitec HS-422 | Carried over from v1, runs on 6V |
| Distance sensors | HY-SRF05 x4 | Front, front-right, right, + one more — schematic confirms 4 sensor headers (v1 had 3; this note was stale) |
| BLE | X-NUCLEO-BNRG2A1 | ST BLE shield, stacks on top of Nucleo via Arduino headers |
| Encoder | Custom quadrature | Two Hall probes on GM37 motor — connects via J10 on motherboard (5-pin: 5V, GND, Ch A, Ch B + 1 spare) |
| Button/LED shield | Custom small shield (new) | Stacks on top of the X-NUCLEO-BNRG2A1 BLE shield, connecting through the same Arduino header pins (pass-through stacking). Carries all buttons and MCU-controlled LEDs — everything except the fixed "alive" power LED, which stays on the motherboard. **Deliberately has no ground plane**, to minimize copper near the BLE shield that could obstruct/detune the antenna. |
| Status LEDs | 4x LEDs total | **1x fixed "alive" LED (green)** — stays on the motherboard, not MCU controllable, wired directly across the 5V rail with its own current-limiting resistor, always on whenever 5V is present. **3x MCU-controlled LEDs** (2x green state, 1x red error) — now live on the Button/LED shield, sourced from 3V3, active low, GPIO sinks current. Down from v1's 5 status LEDs, but the 2 dropped ones (v1's LED3/LED4) were always unused — see `CLAUDE.md` for the v1→v2 LED mapping. |
| Buttons | 2x general purpose | Now live on the Button/LED shield (moved off the motherboard). Carried over from v1 — mode switching, calibration, debug. External 10kΩ pull-ups (not relying on internal MCU pull-ups) |
| Battery | 8x AA NiMH 1.2V | 9.6V nominal, ~8.8V discharged. Voltage sensing divider also checked/works with 8x AA alkaline as an alternative — see Design Decisions. |

---

## Power Architecture

| Rail | Voltage | Consumers | Source | Notes |
|---|---|---|---|---|
| VMOT | 9.6V nominal | H-bridge, motors | Battery direct | 8x AA NiMH |
| 5V | 5V | Sensors (HY-SRF05) | L7805 | ~4.6V drop at full charge — no heatsink for now (see Key Components), will evaluate under real operation |
| 6V | 6V | Servo (HS-422) | L7806 | ~3.6V drop — chosen to reduce regulator heat vs 5V |
| 3V3 | 3.3V | Nucleo | Nucleo onboard 3V3 LDO, fed by Nucleo's own onboard 5V LDO | Nucleo powered via **VIN** (battery, direct) — engages the Nucleo's otherwise-unused onboard 5V LDO to supply its 3V3 stage, rather than drawing that current from motherboard's L7805 |

---

## Connectors

| Interface | Connector type | Notes |
|---|---|---|
| Nucleo ↔ Motherboard | Morpho connectors (CN7, CN10) | Full STM32 pin access |
| Nucleo ↔ BLE shield | Arduino headers (CN5, CN6, CN8, CN9) | X-NUCLEO-BNRG2A1 stacks on top of Nucleo |
| BLE shield ↔ Button/LED shield | Arduino headers (pass-through) | New small shield stacks on top of the BLE shield, riding the same Arduino header footprint through to the top. LED/button GPIOs use Arduino header pins left free by both the BLE shield and the motherboard: D2 (PA10), D6 (PB10), D8 (PA9), D4 (PB5), D9 (PC7) — see the pin table. Confirmed against the physical X-NUCLEO-BNRG2A1 that its Arduino headers are stacking/pass-through type, so the new shield can reach those pins. |

Bus separation is now: Morpho carries all motherboard-native signals (motor, sensors, servo, encoder, power sensing) as before, while the Arduino header bus is shared between the BLE shield and the new Button/LED shield via stacking — no longer fully and exclusively consumed by the BLE shield alone as previously stated.

### Motherboard Connector Reference (designators from KiCad schematic)

| Designator | Purpose | Notes |
|---|---|---|
| J1 | Battery / barrel jack power input | |
| J2–J5 | HY-SRF05 sensor headers (x4) | Each: 5V, TRIG, ECHO, GND, +1 spare pin |
| J7 | Servo connector (HS-422) | PWM signal, 6V supply, GND |
| J11, J12 | Nucleo Morpho headers (CN7, CN10) | |
| J6, J9 | Pololu DRV8874 carrier headers (#4035) | Mating female headers for the off-board motor driver carrier — DRV8874 IC is **not** on the motherboard itself. |
| J8 | Motor output (to GM37 DC motor) | |
| J10 | Encoder connector | Quadrature Hall probe inputs (5V, GND, Ch A, Ch B) |

## Nucleo F401RE Pin Assignment

> Finalized and wired into the schematic (J11=CN7, J12=CN10). Checked against X-NUCLEO-BNRG2A1 default pinout (PA0/PA1/PA5/PA6/PA7/PA8 reserved for BLE SPI+IRQ+RESET) and Nucleo onboard peripherals (PA5=LD2, PC13=B1, PA13/PA14=SWD, PB3=SWO, PA2/PA3=ST-LINK VCP) — no conflicts. Subject to revision during PCB layout if trace routing favors different pin choices.

| Nucleo Pin | Function | Connector | Notes |
|---|---|---|---|
| PA15 | EN — motor speed PWM | J11/17 | TIM2_CH1 (alt mapping) |
| PB8 | PH — motor direction | J12/3 | Plain GPIO |
| PB9 | FLT — motor fault in | J12/5 | Open-drain, pull-up R4 (10k) to MCU_3V3 |
| PB4 | Servo PWM | J12/27 | TIM3_CH1 (alt mapping) |
| PB6 | Encoder Ch A | J12/17 | TIM4_CH1, hardware encoder mode |
| PB7 | Encoder Ch B | J11/21 | TIM4_CH2, hardware encoder mode |
| PC5 | Current sensing (U1 INA169 output) | J12/6 | ADC1_IN15 — reassigned from PC0 (J11/38); PC5 was freed when Sensor 2 moved to J11 and is the lowest-numbered free ADC-capable pin on J12. Load resistor R3 (16k5) and filter cap C3 (6.8nF) to GND at the pin; together they form the RC low-pass (see Design Decisions) |
| PB1 | Battery voltage sensing | J12/24 | ADC1_IN9 — reassigned from PC1 (J11/36); PB1 was freed when Sensor 3's ECHO moved to J11 and is the next-lowest free ADC-capable pin on J12. Filter cap already present |
| PC11 / PC10 | Sensor 1 TRIG / ECHO | J11/2, J11/1 | Reassigned from PC2/PC3 — swapped with the former Sensor 1 pins so pin numbers ascend top-to-bottom, matching the physical top-to-bottom order of the sensor connectors next to J11 (lowest pins → sensor closest to the top). Repurposed from the now-dropped spare SPI3 header (see below). |
| PB0 / PA4 | Sensor 2 TRIG / ECHO | J11/34, J11/32 | Reassigned from PC5/PC6 (J12) — moved to J11 and given the middle pin pair per the top-to-bottom ordering above. |
| PC3 / PC2 | Sensor 3 TRIG / ECHO | J11/37, J11/35 | Reassigned from PB0/PB1 — swapped onto the former Sensor 1 pins (highest of the three pairs), per the top-to-bottom ordering above. |
| PA12 / PA11 | Sensor 4 TRIG / ECHO | J12/12, J12/14 | Deliberately kept on J12. Dual-use: also USART6_RX/TX (AF8) — unused UART, chosen so sensor 4 can be repurposed for serial comms later. TRIG↔RX (PA12), ECHO↔TX (PA11) — if repurposed, the MCU's TX comes out on J5's ECHO pin and RX on its TRIG pin. Changed from PC7/PA4 — PA4 had no UART alternate function. |
| PA10 | LED_STATE1 (green) | Arduino D2 (Button/LED shield) | Active low, LED sourced from 3V3, GPIO sinks the current |
| PB10 | LED_STATE2 (green) | Arduino D6 (Button/LED shield) | Active low, LED sourced from 3V3, GPIO sinks the current |
| PA9 | LED_ERR (red) | Arduino D8 (Button/LED shield) | Active low, LED sourced from 3V3, GPIO sinks the current |
| — | LED_PWR (green, fixed "alive" indicator) | n/a | **Not MCU controlled** — no GPIO pin. Stays on the motherboard, wired directly across the 5V rail with its own current-limiting resistor; on whenever 5V rail is powered. Placed in schematic. |
| PB5 | Button 1 | Arduino D4 (Button/LED shield) | External 10kΩ pull-up to 3V3, active low, other leg to GND. EXTI line 5 |
| PC7 | Button 2 | Arduino D9 (Button/LED shield) | External 10kΩ pull-up to 3V3, active low, other leg to GND. EXTI line 7 |

> **Note — Button/LED shield pins reassigned.** The shield originally used PB2, PB10, PB15, PB13 and PC8, on the assumption that all five were reachable from the Arduino header. On the Nucleo F401RE only PB10 (D6) is; the others are Morpho-only. LED_STATE1, LED_ERR and both buttons therefore moved to the remaining free Arduino pins:
> - Free Arduino pins (not used by the BLE shield — A0, A1, D7, D11–D13 — nor the motherboard — A2/PA4, A3/PB0, D5/PB4, D10/PB6, D14/PB9, D15/PB8): D2 (PA10), D4 (PB5), D8 (PA9), D9 (PC7), A4 (PC1), A5 (PC0). D0/D1 (ST-LINK VCP) and D3 (PB3, SWO) are deliberately avoided.
> - Buttons need GPIO interrupts, and STM32 EXTI lines are shared per pin number across ports, so they take PB5 and PC7 (lines 5 and 7, otherwise unused). PA10, PA9 and PC0 would collide with Sensor 1 ECHO (PC10), FLT (PB9) and the BLE IRQ (PA0) respectively, so PA10/PA9 carry LEDs instead. Moving Button 1 off PB13 also removes its EXTI clash with the Nucleo's B1 button (PC13).
> - A4/A5 (PC1/PC0, both ADC-capable) stay free as spare analog inputs.

> **Spare SPI3 header dropped.** PC10/PC11 (J11/1, J11/2) are now used for Sensor 1 ECHO/TRIG instead. PC12 and PD2 (J11/3, J11/4) are unused/free.

**Pins deliberately avoided:**
- PA0, PA1, PA5, PA6, PA7, PA8 — reserved by X-NUCLEO-BNRG2A1 (SPI IRQ, CS, SCK, MISO, MOSI, RESET in default config)
- PA2, PA3 — left free for ST-LINK virtual COM (debug console)
- PC13, PA13, PA14, PB3 — Nucleo onboard B1 button, SWD, SWO

---

## Design Decisions & Rationale

- **Morpho connectors for motherboard:** Keeps all motherboard-native signals (motor, sensors, servo, encoder, power sensing) on a bus separate from the Arduino headers, giving full STM32 pin access. The Arduino header bus itself is now shared between the BLE shield and the new Button/LED shield — see below — rather than being exclusive to the BLE shield as originally planned.
- **Spare SPI3 header — dropped:** Originally reserved PC10/PC11/PC12/PD2 (J11/1–4) for future display connectivity. Dropped in favor of consolidating all four ultrasonic sensors' TRIG/ECHO onto J11 (see Connectors table) — PC10/PC11 now carry Sensor 1, and the PCB layout has all four sensor headers physically grouped near J11, so this avoids routing traces across to J12. PC12 and PD2 remain free if a future need arises.
- **Sensor TRIG/ECHO consolidated onto J11 (sensors 1–3), ordered by pin number:** Sensor 4 deliberately stays on J12 to preserve its USART6 dual-use option (see below) — no UART is broken out on J11, so moving it would have meant giving up that future repurposing path. Sensors 1–3 had no such constraint, so they were assigned to J11 pin pairs in ascending order (J11/1-2, J11/32-34, J11/35-37) to match the top-to-bottom physical order of the sensor connectors on the PCB, next to J11.
- **New Button/LED shield — buttons and non-fixed LEDs move off the motherboard:** All buttons and LEDs except the fixed "alive" power LED (which stays on the motherboard, wired directly across the 5V rail with no GPIO) move to a dedicated small shield that stacks on top of the X-NUCLEO-BNRG2A1 BLE shield, connecting through the same Arduino header pins (confirmed stacking/pass-through, per the Connectors table above). The stack is now: motherboard → Nucleo → BLE shield → Button/LED shield. This shield is deliberately given **no ground plane**, to keep copper away from the BLE shield/antenna area and minimize any risk of obstructing or detuning the BLE signal. Its LEDs and buttons use free Arduino header pins (D2, D6, D8, D4, D9 — see the pin table and the note below it); the originally planned PB2/PB13/PB15/PC8 are not on the Arduino header.
- **Clean intentional stack:** Replaces the messy v1 combination of Arduino Mega, L298P shield, and two custom PCBs with a purposeful stack: custom motherboard + Nucleo F401RE + BNRG2A1 BLE shield + the new Button/LED shield. Each board has a clear role and they are designed to fit together.
- **Nucleo fed from VIN, not E5V:** Battery (post diode+fuse) connects to the Nucleo's VIN pin rather than E5V. This deliberately makes use of the Nucleo's onboard 5V LDO (otherwise idle), which in turn feeds the Nucleo's own 3V3 LDO — so the Nucleo regulates its own logic rail directly from battery voltage instead of drawing that current through the motherboard's L7805. Reduces L7805 load/heat since it now only needs to supply the external 5V loads (sensors, fixed power LED), not the Nucleo's internal rail too.
- **L7806 for servo:** 6V is within HS-422 spec and reduces voltage drop vs a 5V rail, meaning less heat on the regulator.
- **NiMH battery (8x AA):** 9.6V nominal — reused from v1. Adequate for current design, may revisit in future.
- **Current-sensing ADC filter cap:** C3 (6.8nF) from the `CURRENT_SENSING` net (PC5) to GND, right at the ADC pin. Since the INA169 output is a current source, C3 forms the RC low-pass with the load resistor R3 (16k5): τ ≈ 112µs, f₋₃dB ≈ 1.4kHz. C3 was sized by the rule of thumb of a cutoff at roughly 1/15 of the motor PWM frequency (1.4kHz ≈ 21kHz / 15); with the firmware's 25kHz PWM the cutoff is ≈1/18 of it, i.e. slightly more filtering than the rule — revisit C3 if the PWM frequency ever changes substantially. Filters H-bridge PWM switching noise and improves ADC sample-and-hold settling, per ST's ADC accuracy app notes. No tradeoff of concern here since overcurrent protection is already handled by the hardware fuse (F1); the ADC reading doesn't need to track fast transients. Battery voltage sensing (PB1) already has an equivalent filter cap in place — no change needed there.
- **Current/voltage sensing moved to J12 (PC0→PC5, PC1→PB1):** Both were originally on J11 (PC0/PC1). Reassigned to the lowest-numbered free ADC-capable pins on J12 — PC5 (J12/6) and PB1 (J12/24), both vacated by the sensor 2/3 moves above — per request to keep pin numbers as low as possible on J12.
- **Buttons: external 10kΩ pull-ups, not internal:** Chose external over the STM32's internal pull-ups (weak, ~40kΩ typical, tolerance not tightly specified). Originally justified partly by the buttons sitting off-board through longer Morpho wiring near motor/H-bridge switching noise — that specific routing no longer applies now that the buttons live on the Button/LED shield (Arduino header path, away from the motor), but external pull-ups are kept for the same general robustness reasons, and it still leaves room for an optional RC debounce cap later if needed. Same approach the Nucleo itself uses for its onboard B1 button (external pull-up, not internal).
- **Sensor 4 TRIG/ECHO doubles as USART6 (PA11/PA12):** Moved sensor 4 off its original PC7/PA4 assignment onto PA12 (TRIG) / PA11 (ECHO) so the same two Morpho pins (J12/12, J12/14) can alternatively be reconfigured in firmware as USART6_RX/TX — the only USART on the F401RE not already claimed by the BLE shield, ST-LINK VCP, or another peripheral. Gives a spare debug/telemetry UART without adding a connector, at the cost of losing sensor 4 if the UART is ever active at the same time (mutually exclusive use, not simultaneous).
- **Custom Hall probe encoder:** Two Hall probes on GM37 motor for quadrature feedback. Enables closed-loop PID speed control (upgrade from v1 open loop). STM32 hardware timer encoder mode to be used.
- **ADC protection diodes (D1, D2, BAT54S) dropped — simplification:** The dual-Schottky BAT54S clamps that had been designed for the current-sensing (U1 output → PC5) and voltage-sensing (PB1) ADC inputs — protecting against overvoltage and undershoot faults, and against fault current backfeeding into the Nucleo's onboard `MCU_3V3` rail — have been removed entirely. Accepted as reasonable risk for a hobby project rather than carrying the extra parts/complexity. No replacement clamp is planned; the sensing paths rely on component sizing instead (see the current-sensing and battery-divider notes below) to keep ADC pins within the STM32's absolute-max ratings.
- **Current sensing: INA169 high-side monitor (U1) — replaces an earlier, different sensing part.** Rsense1 = 100mΩ sits between `Vbat` and `Vin`, so it measures the **total robot current** (motor driver, both regulators and the Nucleo's VIN all hang off `Vin`), not just the motor — same as v1's `ICurrentSensor`; U1 (INA169, transconductance 1000µA/V, powered from `Vbat`) turns the sense voltage into an output current, and load resistor R3 (16k5) to GND turns that into the voltage on PC5. Overall: **V_PC5 = I × 0.1Ω × 1mA/V × 16.5kΩ = 1.65V per amp** (gain 16.5 on the sense voltage). Sized around the **fuse (F1), which enforces a 1A continuous ceiling** — the real worst case for sensing purposes, not DRV8874's rated 2.1A: 1A → 1.65V at the ADC, leaving headroom up to the 3.3V full scale (≈2A). Firmware: `I_mA = V_mV × 1000 / (100mΩ × 16.5)`, i.e. `V_mV / 1.65` (the `supply_current` node in the motherboard shield overlay carries these values). Because the INA169 output is a current source, an overcurrent transient beyond ≈2A can only push its small output current (≈gm × V_sense, e.g. 300µA at 3A) into the pin's protection structure, rather than a stiff voltage — worth keeping in mind given the ADC clamp diodes were dropped (see above).
- **Battery voltage divider (R1, R2):** v1 used R1=1M5/R2=100k (1/16 ratio), inherited without reconsidering fit for this application — at 9.6V nominal that's only 0.6V on the ADC, using ~18% of the 0–3.3V range and wasting resolution/noise margin. Resized to **R1=300k, R2=75k (1/5 ratio)** — an integer ratio chosen deliberately so firmware can do `Vbat_mV = Vadc_mV * 5` without floating point. Both values are standard E24 1% resistors (unlike the originally-considered 400k, which isn't E24), so the ratio is exact with no series/parallel workaround needed. Checked against both battery chemistries the pack might use:
  - NiMH (nominal 9.6V, ~8x1.2V): fresh peak ~12V → 2.4V ADC; nominal 9.6V → 1.92V ADC; discharged ~8V → 1.6V ADC
  - Alkaline (~8x1.5V, higher fresh voltage): fresh peak ~13.6V → **2.72V ADC**, the worst case. Since the D2 clamp has since been removed (see ADC protection diodes decision above), this ~0.88V margin below the STM32's absolute max in analog mode (~VDDA+0.3V ≈ 3.6V) is now the only thing standing between a battery-voltage fault and the MCU — no hardware clamp left as backup. Judged acceptable for a hobby project, but worth keeping in mind if the divider or battery chemistry ever changes.
  - 1/4 ratio was considered but rejected — fresh alkaline peak would hit ~3.4V, uncomfortably close to that ~3.6V absolute max with no clamp left to catch a fault
  - Divider quiescent current: 9.6V/375kΩ ≈ 25.6µA (was ≈19.2µA with the original 500kΩ total) — negligible for this battery-powered application; lower total resistance is also slightly friendlier to ADC source-impedance/settling
  - Note for firmware: NiMH holds a flat voltage most of its life then drops sharply near empty, so voltage alone is a poor state-of-charge estimate for NiMH (fine for a low-battery cutoff, not for a % gauge). Alkaline discharges more linearly, so voltage-based estimation is more meaningful there if ever wanted.

---

## Firmware

- **RTOS:** Zephyr, pinned to latest stable (4.4.0, released 2026-04-14 — reconfirm at docs.zephyrproject.org/latest/releases before assuming, since a new stable ships roughly every April/October)
- **MCU:** STM32F401RE (Nucleo board)
- **BLE stack:** via X-NUCLEO-BNRG2A1, using Zephyr's BLE/HCI support
- **Language/style/libraries:** C++23, WebKit code style, Embedded Template Library (ETL) — see `CLAUDE.md` for the concrete Kconfig/toolchain setup

No custom board — wallckr-z2 builds on the existing upstream `nucleo_f401re` board and adds everything else as Zephyr shields instead (motherboard, Button/LED shield; the BLE shield, `x_nucleo_bnrg2a1`, already exists upstream). Shield overlays live under `firmware/boards/shields/`, defining pin mappings that must match the KiCad schematic. `zephyr/module.yml` at the repo root declares `board_root: firmware`, so Zephyr auto-discovers them via west's module scan — no manual `-DBOARD_ROOT=` needed on the build command line. (That repo-level `zephyr/` is just the fixed module-manifest marker directory Zephyr's tooling looks for — not to be confused with the actual Zephyr RTOS clone at the workspace root, `wallckr-z2-workspace/zephyr/`.) `west.yml` sits at the repo root too: `west init -l` treats the *parent* of the manifest directory as the workspace root, so a manifest nested under `firmware/` would put the Zephyr clone inside the git repo. See `CLAUDE.md` for the shield breakdown (Morpho nexus node confirmed present on `nucleo_f401re`, so no extra work needed there).

### West Workspace Structure

```
wallckr-z2-workspace/        ← west workspace root (not a repo)
  .west/
  zephyr/                    ← Zephyr RTOS itself, managed by west
  modules/                   ← west-managed modules
  wallckr-z2/                ← the actual repo (west manifest repo: `west init -l wallckr-z2`)
    west.yml                 ← declares Zephyr + external modules (incl. ETL)
    zephyr/
      module.yml             ← makes the repo a Zephyr module; board_root: firmware
    CLAUDE.md
    .clang-format
    .clang-tidy
    .github/workflows/build.yml
    firmware/
      lib/                   ← target-independent C++ shared by main, bringup and tests (ported from v1's lib/: Robot, control, protocol)
      main/                  ← the real vehicle firmware
        src/
        CMakeLists.txt
        prj.conf
      bringup/               ← hardware bring-up / test firmware
        src/
        CMakeLists.txt
        prj.conf
      tests/                 ← Ztest suites, run by Twister on native_sim
      boards/                ← shield definitions (no custom board — base is upstream nucleo_f401re)
    kicad/
      motherboard/
      button-led-shield/
    tools/
      ovladacka/             ← Python BLE controller script (moved from own repo)
    docs/
      DESIGN_NOTES.md
```

### Repo Strategy

- **wallckr** (v1) — existing repo, Arduino-based, being refactored for portability
- **wallckr-z2** — new repo, contains Zephyr firmware, KiCad hardware design (two projects: motherboard + Button/LED shield), and tools
- **ovladacka** — to be archived, moved into `wallckr-z2/tools/ovladacka/`
- **wallckr-common** (future) — shared logic/algorithms extracted as a west module when needed
- **`firmware/main` vs `firmware/bringup`** — two independent Zephyr applications, each with its own `CMakeLists.txt`/`prj.conf`, sharing the same `boards/`, `lib/` and `west.yml`. `bringup` is standalone hardware bring-up/test firmware (checking peripherals during assembly), kept separate so its settings — shell/console enabled, drivers exercised in isolation, only a minimal BLE link test (Nordic UART Service heartbeat, no ovladacka protocol) — never leak into the real vehicle firmware in `main`.

---

## Known Open Items

- **Schematic R1 value is wrong** — the schematic (and PCB file) say R1 = 360k, but the board carries 300k (verified), matching the 1/5 divider described under Design Decisions. Firmware uses 300k; the schematic needs correcting.
- **ovladacka repo migration** — not yet done. Still lives in its own repo; needs moving into `wallckr-z2/tools/ovladacka/`.
- **Plan B: Nucleo-L476RG instead of the F401RE** — not planned yet, but the fallback if either limit below starts to bite:
  - *No hardware RNG on the F401* — Zephyr falls back to a timer-based test generator, which isn't good enough for BLE pairing/encryption. The L476 has a TRNG.
  - *Flash for FOTA* — the F401's 512 KB in large sectors (16/64/128 KB) allows only 128 KB MCUboot swap slots; a BLE firmware-update sample alone (~134 KB) already overflows one. The L476's 1 MB in 2 KB pages allows two ~470 KB slots with rollback. On the F401, overwrite-only updates (no rollback, ~192 KB image) would be the fallback.
  - *Motherboard: no PCB change needed* — Morpho and Arduino pinouts match, and the timer (PA15, PB4, PB6/PB7), SPI1 and ADC (PB1, PC5) functions exist on the same pins.
  - *Only loss:* the L476 has no USART6, so Sensor 4's PA11/PA12 UART option (which reused J5, no hardware change) goes away. UART5 on PC12/PD2 (J11/3, J11/4) would be the alternative, but those pins aren't routed on the motherboard — it would need bodge wires or a header in a future PCB revision.
  - *Next PCB revision (L476 only):* route J5 to PC12/PD2 to keep Sensor 4's GPIO-or-UART dual use — **ECHO → PC12** (EXTI 12, free; UART5 TX) and **TRIG → PD2** (UART5 RX). PD2 can't take ECHO: EXTI 2 is already Sensor 3's ECHO (PC2). As today, the connector's ECHO/TRIG pins become TX/RX in UART mode. Check PC12's 5V tolerance for the ECHO signal.
  - *To check first:* 5V tolerance of the L476 pins receiving 5V signals (sensor ECHO lines, encoder inputs). Firmware changes (ADC channel numbers, 80 MHz timer prescaler, board-specific shield overlays) are software-only.

---

## Links

- v1 repository: https://github.com/ladapn/wallckr
- Nucleo F401RE datasheet: https://www.st.com/en/evaluation-tools/nucleo-f401re.html
- ovladacka (BLE remote control app): https://github.com/ladapn/ovladacka

---

*This file is intended to be pasted as context at the start of AI-assisted design sessions.*
