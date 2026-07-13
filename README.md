# AeroPico-FC

[![PlatformIO](https://img.shields.io/badge/PlatformIO-compatible-orange)](https://platformio.org/)
[![Target](https://img.shields.io/badge/Target-RP2350%20%2F%20Pico%202-7546C6)](https://www.raspberrypi.com/products/raspberry-pi-pico-2/)
[![Language](https://img.shields.io/badge/Language-C%2B%2B17-00599C)](https://isocpp.org/)
[![Protocol](https://img.shields.io/badge/Protocol-MAVLink-red)](https://mavlink.io/)
[![License](https://img.shields.io/badge/License-MIT-green)](LICENSE)

AeroPico-FC is an open-source fixed-wing flight controller firmware for the
RP2350-based Raspberry Pi Pico 2. The v1.0.0-rc1 target is a professional
manual-flight software release candidate: deterministic scheduling, isolated
flight-control pipelines, MAVLink ground-station compatibility, runtime
parameters, blackbox logging, preflight gates, failsafe policy, and a dedicated
Configurator.

This project is not a certified flight system. v1.0.0-rc1 is a software RCI
release. Physical bench validation, HIL evidence, airframe-specific tuning, and
field test records are required before any real flight or commercial use.

## Scope

v1.0 focuses on manual fixed-wing infrastructure:

- `MANUAL` and stabilized control foundations
- SBUS receiver input and RC failsafe handling
- cascaded attitude/rate PID control
- fixed-wing mixer and PIO servo output
- MPU6050 / GY-87 sensor stack with DMA-assisted I2C
- adaptive Madgwick attitude estimator
- 2-state barometric vertical Kalman estimator
- typed state publishing and blackboard-style data flow
- MAVLink Common telemetry and command handling
- Mission Planner / QGroundControl generic autopilot compatibility
- AeroPico Configurator for setup, parameters, calibration, and bench service commands

The following are deliberately not active v1.0 flight features:

- RTL
- waypoint mission execution
- loiter
- auto landing
- full-state navigation EKF

The infrastructure for GPS capability, mission stubs, navigation state, and
future modes is present so those features can be added in later releases without
rewriting the core manual-flight architecture.

## Architecture Highlights

| Area | Implementation |
| --- | --- |
| Real-time model | FreeRTOS tasks, Core 0 acquisition/telemetry, Core 1 flight loop |
| Scheduler | Time-triggered scheduler for telemetry, logging, health, and task budgets |
| Control loop | 500 Hz flight control path with timing monitor and watchdog gate |
| Sensor I/O | RP2350 I2C, DMA fast path for IMU, fallback HAL I2C path |
| Sensor backends | Role/backend split: `Mpu6050Backend`, `Hmc5883lBackend`, `Bmp085Backend` |
| Estimation | Adaptive Madgwick attitude + barometric vertical Kalman filter |
| Safety | PreflightHealth, FailsafeManager, battery/brownout monitor, watchdog gating |
| Actuation | PIO PWM with dynamic system-clock divider |
| Telemetry | MAVLink over USB Serial and PIO UART companion transport |
| Persistence | Flash-backed calibration and parameter storage envelopes |
| Testing | Native unit tests, native full-link test, static architecture policy, fault injection |

## Ground-Station Compatibility

AeroPico-FC behaves as a generic MAVLink autopilot. It does not emulate
ArduPilot internals or ArduPilot-specific setup wizards.

Supported v1.0.0-rc1 MAVLink/GCS behavior:

- `HEARTBEAT` with real armed bit
- `COMMAND_LONG`
- `MAV_CMD_COMPONENT_ARM_DISARM`
- `COMMAND_ACK`
- `SYS_STATUS`
- `VFR_HUD`
- `GPS_RAW_INT` with `GPS_FIX_TYPE_NO_GPS` when GPS is absent
- `MISSION_REQUEST_LIST` response with `MISSION_COUNT = 0`
- `STATUSTEXT` for preflight, arm denial, service command, and failsafe reasons
- MAVLink parameters with runtime safety gates

Recommended bench order:

1. AeroPico Configurator over Pico USB Serial
2. QGroundControl over Pico USB Serial
3. Mission Planner over Pico USB Serial
4. Optional ESP32/WiFi MAVLink bridge after USB behavior is validated

## AeroPico Configurator

The Configurator lives in `tools/aeropico-configurator`.

It provides:

- serial port selection and baud selection
- MAVLink parameter read/write
- `PARAM_SAVE` flash persistence
- PID, mixer, servo, RC, failsafe, stream-rate, and blackbox settings
- module and heartbeat status
- armed/disarmed status
- command pending/accepted/rejected tracking
- safe service commands:
  - IMU calibration
  - two-step magnetometer hard-iron calibration
  - sensor check
  - preflight check
  - RC monitor
  - disarmed-only servo direction test
- Pico 2 pin mapper and local configuration audit

Run it locally:

```bash
cd tools/aeropico-configurator
npm install
npm start
```

Static check:

```bash
npm run check
```

## Hardware Pinout

Default v1.0 bench pinout:

| Function | Pico 2 pin | Signal |
| --- | --- | --- |
| I2C SDA | GP4 | GY-87 / MPU6050 / mag / baro |
| I2C SCL | GP5 | GY-87 / MPU6050 / mag / baro |
| SBUS RX | GP1 | Receiver input through transistor inverter |
| Companion TX | GP12 | PIO UART MAVLink/blackbox TX |
| Companion RX | GP13 | PIO UART MAVLink RX |
| Battery ADC | GP26 / ADC0 | Voltage divider input |
| Aileron | GP16 | PIO PWM |
| Elevator | GP17 | PIO PWM |
| Rudder | GP18 | PIO PWM |
| Throttle | GP19 | PIO PWM / ESC signal |

Use 3.3 V logic for Pico-side signals. Never feed 5 V pull-ups or 5 V signal
levels into RP2350 GPIO or ADC pins.

## Build

Install PlatformIO, then run:

```bash
pio run -e pico
```

The UF2 firmware is generated at:

```text
.pio/build/pico/firmware.uf2
```

To flash manually, hold BOOTSEL while connecting the Pico 2 and copy the UF2
file to the mounted drive.

## Verification

Software verification used for v1.0.0-rc1:

```bash
pio test -e native
pio run -e native_link
pio test -e native_link
python3 tools/ci/check_architecture.py
python3 tools/fault_injection/fault_injection.py
pio run -e pico
cd tools/aeropico-configurator && npm run check
```

Expected release-gate status:

- native tests pass
- native full-link integration passes
- architecture policy passes
- fault-injection smoke passes
- Pico firmware build passes
- Configurator JavaScript syntax check passes

## Bench Validation Required Before Flight

The following hardware evidence must be captured before moving from software RCI
to real flight readiness:

- servo PWM capture at 1000/1500/2000 us with a logic analyzer
- SBUS GP1 receiver input validation
- RC failsafe timeout and mode-channel validation
- battery ADC divider calibration with a multimeter
- GY-87 sensor health and stale-sample behavior
- watchdog behavior when the flight task stalls
- Mission Planner and QGroundControl USB connection records
- blackbox log capture and replay/inspection
- at least one documented dry-run checklist with propeller removed

## Documentation

Important project documents:

- `docs/AeroPico_FC_Kullanma_Kilavuzu.md`
- `docs/AeroPico_FC_Gelistirici_Kullanma_Kilavuzu.md`
- `docs/AeroPico_FC_v1_0_RCI_Release_Notes.md`
- `docs/Bench_Test_Checklist.md`
- `docs/First_Flight_Checklist.md`
- `docs/HIL_Bench_Artifact_Template.md`
- `docs/MAVLink_Security_Policy.md`
- `docs/Project_Structure.md`

## Release

Current software release candidate:

- Tag: `v1.0.0-rc1`
- Branch source: `pico2-catalyi`
- Target branch after promotion: `main`

The release is intentionally labeled RCI/RC because physical bench and HIL proof
remain mandatory before declaring flight-ready v1.0 final.

## License

AeroPico-FC is released under the MIT License. See `LICENSE`.

Copyright (c) 2026 Muhammed Fatih Emre Ozcelik.
