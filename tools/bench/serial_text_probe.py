#!/usr/bin/env python3
"""Tiny serial text reader for USB smoke tests."""

import argparse
import fcntl
import os
import select
import struct
import sys
import termios
import time


BAUD_MAP = {
    57600: termios.B57600,
    115200: termios.B115200,
    230400: termios.B230400,
}


def configure(fd: int, baud: int) -> None:
    speed = BAUD_MAP.get(baud)
    if speed is None:
        raise SystemExit(f"unsupported baud: {baud}")
    attrs = termios.tcgetattr(fd)
    attrs[0] = 0
    attrs[1] = 0
    attrs[2] = termios.CS8 | termios.CREAD | termios.CLOCAL
    attrs[3] = 0
    attrs[4] = speed
    attrs[5] = speed
    attrs[6][termios.VMIN] = 0
    attrs[6][termios.VTIME] = 0
    termios.tcsetattr(fd, termios.TCSANOW, attrs)

    tiocm_dtr = getattr(termios, "TIOCM_DTR", 0x002)
    tiocm_rts = getattr(termios, "TIOCM_RTS", 0x004)
    tiocmset = getattr(termios, "TIOCMBIS", None)
    if tiocmset is not None:
        fcntl.ioctl(fd, tiocmset, struct.pack("I", tiocm_dtr | tiocm_rts))


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", required=True)
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--seconds", type=float, default=8.0)
    args = parser.parse_args()

    fd = os.open(args.port, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
    try:
        configure(fd, args.baud)
        print(f"[text-probe] opened {args.port} @ {args.baud}")
        start = time.monotonic()
        seen = 0
        while time.monotonic() - start < args.seconds:
            readable, _, _ = select.select([fd], [], [], 0.25)
            if not readable:
                continue
            data = os.read(fd, 512)
            if not data:
                continue
            seen += len(data)
            sys.stdout.write(data.decode("utf-8", errors="replace"))
            sys.stdout.flush()
        print(f"\n[text-probe] read {seen} bytes")
        return 0 if seen else 2
    finally:
        os.close(fd)


if __name__ == "__main__":
    raise SystemExit(main())
