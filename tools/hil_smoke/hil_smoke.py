#!/usr/bin/env python3
import argparse
import sys
import time


PASS_TOKENS = (
    "AeroPico",
    "HEALTH",
    "READY",
    "Baslatildi",
)

FAIL_TOKENS = (
    "Stack overflow",
    "Malloc failed",
    "watchdog ile resetlendi",
    "DMA..............FAIL",
    "IMU..............FAIL",
)


def main() -> int:
    parser = argparse.ArgumentParser(description="AeroPico FC HIL smoke test")
    parser.add_argument("--port", required=True, help="Serial port, e.g. /dev/tty.usbmodemXXXX")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--seconds", type=float, default=12.0)
    args = parser.parse_args()

    try:
        import serial
    except ImportError:
        print("pyserial is required: python3 -m pip install pyserial", file=sys.stderr)
        return 2

    seen_pass = set()
    captured = []
    deadline = time.time() + args.seconds

    with serial.Serial(args.port, args.baud, timeout=0.25) as ser:
        while time.time() < deadline:
            raw = ser.readline()
            if not raw:
                continue

            line = raw.decode("utf-8", errors="replace").strip()
            captured.append(line)
            print(line)

            for token in FAIL_TOKENS:
                if token in line:
                    print(f"HIL smoke failed: saw '{token}'", file=sys.stderr)
                    return 1

            for token in PASS_TOKENS:
                if token in line:
                    seen_pass.add(token)

    if len(seen_pass) < 2:
        print("HIL smoke failed: boot/health output was not observed", file=sys.stderr)
        return 1

    print("HIL smoke passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
