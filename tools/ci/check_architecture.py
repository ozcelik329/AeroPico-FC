#!/usr/bin/env python3
from pathlib import Path
import sys


ROOT = Path(__file__).resolve().parents[2]
SENSOR_FUSION_SOURCES = tuple((ROOT / "src").rglob("SensorFusion.cpp"))

POLICY_TARGETS = {
    "main scheduler/task policy target": (ROOT / "src/main.cpp",),
    "PIO UART policy target": (ROOT / "src/drivers/PioUart.cpp",),
    "servo output policy target": (ROOT / "src/drivers/Output.cpp",),
    "servo PIO policy target": (ROOT / "src/drivers/pwm.pio",),
    "sensor DMA bus policy target": (ROOT / "src/drivers/sensors/SensorDmaBus.cpp",),
    "sensor aux bus policy target": (ROOT / "src/drivers/sensors/SensorAuxBus.cpp",),
    "sensor fusion policy target": SENSOR_FUSION_SOURCES,
}

FORBIDDEN = {
    "dynamic flight task allocation": (
        (ROOT / "src/main.cpp", "xTaskCreateAffinitySet("),
    ),
    "blocking PIO telemetry": (
        (ROOT / "src/drivers/PioUart.cpp", "pio_sm_put_blocking"),
    ),
    "servo FIFO clear jitter": (
        (ROOT / "src/drivers/Output.cpp", "pio_sm_clear_fifos(targetPio"),
    ),
    "double precision sensor fusion math": tuple(
        (path, token)
        for path in SENSOR_FUSION_SOURCES
        for token in ("sqrt(", "asin(", "atan2(")
    ),
    "sensor hot path busy wait": tuple(
        (path, token)
        for path in (
            ROOT / "src/drivers/Sensors.cpp",
            ROOT / "src/drivers/sensors/SensorDmaBus.cpp",
            ROOT / "src/drivers/sensors/SensorAuxBus.cpp",
        )
        for token in ("tight_loop_contents(", "busy_wait_us(", "sleep_us(")
    ),
    "direct Serial logging in critical modules": tuple(
        (path, "Serial.")
        for root in (
            ROOT / "src/core",
            ROOT / "src/drivers/Sensors.cpp",
            ROOT / "src/drivers/sensors",
        )
        for path in ([root] if root.is_file() else root.rglob("*"))
        if path.suffix in {".cpp", ".h"}
    ),
    "legacy control-loop documentation": tuple(
        (path, "400Hz")
        for path in (
            ROOT / "README.md",
            ROOT / "docs/Project_Structure.md",
            ROOT / "docs/Bench_Test_Checklist.md",
            ROOT / "docs/AeroPico_FC_v0.2.0_Professional_Software_Architecture_Review.md",
        )
    ),
    "heap allocation in flight core": tuple(
        (path, token)
        for path in (ROOT / "src/core").rglob("*")
        if path.suffix in {".cpp", ".h"}
        for token in ("malloc(", "calloc(", "realloc(", "new ")
    ),
}


def main() -> int:
    failures = []
    for label, paths in POLICY_TARGETS.items():
        if not paths or not any(path.exists() for path in paths):
            failures.append(f"{label} is missing")

    for rule, checks in FORBIDDEN.items():
        for path, token in checks:
            if path.exists() and token in path.read_text(errors="ignore"):
                failures.append(f"{rule}: {path.relative_to(ROOT)} contains {token!r}")

    pwm_program = ROOT / "src/drivers/pwm.pio"
    if pwm_program.exists():
        pwm_text = pwm_program.read_text(errors="ignore")
        if "set pins, 1" not in pwm_text or "set pins, 0" not in pwm_text:
            failures.append("servo PWM PIO program must drive output pins high and low")
        if "pull noblock" in pwm_text:
            failures.append("servo PWM PIO program must not reuse stale pulse values with pull noblock")

    output_cpp = ROOT / "src/drivers/Output.cpp"
    if output_cpp.exists():
        output_text = output_cpp.read_text(errors="ignore")
        for fn in ("void ServoOutput::setServoPulse", "static void writePulse"):
            start = output_text.find(fn)
            if start >= 0:
                end = output_text.find("\n}", start)
                body = output_text[start:end if end >= 0 else len(output_text)]
                if "pio_sm_clear_fifos" in body:
                    failures.append(f"servo write path must not clear PIO FIFOs: {fn}")

    pio_uart = ROOT / "src/drivers/PioUart.cpp"
    if pio_uart.exists():
        uart_text = pio_uart.read_text(errors="ignore")
        for required in ("pioUartIrqHandler", "serviceRx()", "pio_get_tx_fifo_not_full_interrupt_source"):
            if required not in uart_text:
                failures.append(f"PIO UART must use background IRQ service: missing {required}")

    main_cpp = ROOT / "src/main.cpp"
    if main_cpp.exists() and 'addTask("rc", 50,' in main_cpp.read_text(errors="ignore"):
        failures.append("RC scheduler rate must not remain at 50Hz")

    oversized = []
    for path in (ROOT / "src").rglob("*"):
        if path.suffix not in {".cpp", ".h"}:
            continue
        lines = sum(1 for _ in path.open(errors="ignore"))
        if lines > 500:
            oversized.append(f"{path.relative_to(ROOT)}={lines}")
    if oversized:
        failures.append("source files over 500 lines: " + ", ".join(oversized))

    fault_matrix = ROOT / "tools/fault_injection/fault_matrix.json"
    if not fault_matrix.exists():
        failures.append("fault injection matrix is missing")

    if failures:
        print("\n".join(f"[architecture] FAIL {failure}" for failure in failures))
        return 1
    print("[architecture] all policy checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
