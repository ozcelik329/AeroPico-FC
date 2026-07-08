#!/usr/bin/env python3
import argparse
import subprocess
import sys


DEFAULT_TESTS = (
    "test_watchdog_gate",
    "test_failsafe_manager",
    "test_rc_pipeline",
    "test_sensor_health_monitor",
    "test_battery_monitor",
    "test_preflight",
    "test_sensor_preflight",
    "test_sensor_pipeline",
    "test_ekf_lite_estimator",
    "test_param_manager",
    "test_param_storage",
    "test_calibration_storage",
)


def run_test(name: str) -> int:
    cmd = [
        "pio",
        "test",
        "-e",
        "native",
        "--without-uploading",
        "--filter",
        name,
    ]
    print(f"[fault-injection] running {name}")
    return subprocess.call(cmd)


def main() -> int:
    parser = argparse.ArgumentParser(description="Run AeroPico native fault-injection smoke tests")
    parser.add_argument("--test", action="append", dest="tests", help="Specific PlatformIO test name")
    args = parser.parse_args()

    tests = tuple(args.tests) if args.tests else DEFAULT_TESTS
    failed = []

    for test in tests:
        if run_test(test) != 0:
            failed.append(test)

    if failed:
        print("[fault-injection] failed: " + ", ".join(failed), file=sys.stderr)
        return 1

    print("[fault-injection] passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
