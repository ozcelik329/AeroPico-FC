# AeroPico HIL Smoke Test

Purpose: verify the minimum hardware boot path after flashing a Pico 2.

This is not a full flight test. It checks that the firmware boots, emits health
telemetry, and keeps the flight loop alive long enough for the watchdog gate to
stay healthy.

Fault semantics that do not require hardware are covered by
`tools/fault_injection/fault_injection.py` in CI.

## Required Hardware

- Pico 2 / RP2350 board flashed with the current firmware
- USB serial connection
- Connected IMU/I2C stack used by the target build
- Optional: SBUS receiver and ESP32-CAM telemetry path

## Run

```bash
python3 tools/hil_smoke/hil_smoke.py --port /dev/tty.usbmodemXXXX --seconds 12
```

Expected pass signals:

- Boot banner appears.
- Health report appears.
- No stack overflow or malloc failure appears.
- No watchdog reset warning appears during the observation window.
