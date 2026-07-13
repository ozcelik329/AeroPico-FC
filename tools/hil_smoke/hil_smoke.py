#!/usr/bin/env python3
import argparse
import sys
import time


REQUIRED_TOKENS = (
    "AeroPico",
    "HEALTH",
    "READY",
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
    parser.add_argument("--require", action="append", default=[], help="Additional required serial token")
    args = parser.parse_args()

    try:
        import serial
    except ImportError:
        print("pyserial is required: python3 -m pip install pyserial", file=sys.stderr)
        return 2

    required_tokens = REQUIRED_TOKENS + tuple(args.require)
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

            for token in required_tokens:
                if token in line:
                    seen_pass.add(token)

    missing = [token for token in required_tokens if token not in seen_pass]
    if missing:
        print("HIL smoke failed: missing tokens: " + ", ".join(missing), file=sys.stderr)
        return 1

    print("HIL smoke passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
