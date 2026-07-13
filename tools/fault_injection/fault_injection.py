#!/usr/bin/env python3
import argparse
import json
from pathlib import Path
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
    "test_baro_vertical_kalman",
    "test_param_manager",
    "test_param_storage",
    "test_calibration_storage",
    "test_blackboard",
    "test_event_bus",
    "test_blackbox",
    "test_controllers",
)

ROOT = Path(__file__).resolve().parents[2]
MATRIX_PATH = ROOT / "tools" / "fault_injection" / "fault_matrix.json"


def load_matrix():
    if not MATRIX_PATH.exists():
        return []
    return json.loads(MATRIX_PATH.read_text())


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


def test_exists(name: str) -> bool:
    return (ROOT / "test" / name).is_dir()


def main() -> int:
    parser = argparse.ArgumentParser(description="Run AeroPico native fault-injection smoke tests")
    parser.add_argument("--test", action="append", dest="tests", help="Specific PlatformIO test name")
    parser.add_argument("--json-report", dest="json_report", help="Write machine-readable JSON result")
    args = parser.parse_args()

    tests = tuple(args.tests) if args.tests else DEFAULT_TESTS
    failed = []
    results = []

    for test in tests:
        if not test_exists(test):
            print(f"[fault-injection] missing test target: {test}", file=sys.stderr)
            failed.append(test)
            results.append({"test": test, "passed": False, "missing": True})
            continue
        code = run_test(test)
        results.append({"test": test, "passed": code == 0})
        if code != 0:
            failed.append(test)

    if args.json_report:
        matrix = load_matrix()
        covered = {result["test"]: result["passed"] for result in results}
        report = {
            "passed": not failed,
            "tests": results,
            "matrix": [
                {
                    **entry,
                    "passed": covered.get(entry["test"], False),
                }
                for entry in matrix
                if entry["test"] in tests
            ],
        }
        Path(args.json_report).write_text(json.dumps(report, indent=2) + "\n")

    if failed:
        print("[fault-injection] failed: " + ", ".join(failed), file=sys.stderr)
        return 1

    print("[fault-injection] passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
