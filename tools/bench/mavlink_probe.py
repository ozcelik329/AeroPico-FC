#!/usr/bin/env python3
"""Small dependency-free MAVLink serial probe for AeroPico bench bring-up."""

import argparse
import fcntl
import os
import select
import signal
import struct
import subprocess
import sys
import termios
import time


BAUD_MAP = {
    57600: termios.B57600,
    115200: termios.B115200,
    230400: termios.B230400,
}

MSG_NAMES = {
    0: "HEARTBEAT",
    1: "SYS_STATUS",
    22: "PARAM_VALUE",
    24: "GPS_RAW_INT",
    30: "ATTITUDE",
    33: "GLOBAL_POSITION_INT",
    44: "MISSION_COUNT",
    74: "VFR_HUD",
    77: "COMMAND_ACK",
    253: "STATUSTEXT",
}


def configure_serial(fd: int, baud: int) -> None:
    attrs = termios.tcgetattr(fd)
    speed = BAUD_MAP.get(baud)
    if speed is None:
        raise SystemExit(f"Unsupported baud {baud}; use one of {sorted(BAUD_MAP)}")

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
    tiocmbis = getattr(termios, "TIOCMBIS", None)
    if tiocmbis is not None:
        fcntl.ioctl(fd, tiocmbis, struct.pack("I", tiocm_dtr | tiocm_rts))


def parse_statustext(payload: bytes) -> str:
    if not payload:
        return ""
    text = payload[1:51].split(b"\0", 1)[0]
    return text.decode("ascii", errors="replace")


def parse_heartbeat(payload: bytes) -> str:
    if len(payload) < 9:
        return ""
    custom_mode, mav_type, autopilot, base_mode, system_status, mav_ver = struct.unpack_from("<IBBBBB", payload)
    armed = "yes" if base_mode & 0x80 else "no"
    return (
        f"type={mav_type} autopilot={autopilot} armed={armed} "
        f"base_mode=0x{base_mode:02X} status={system_status} custom={custom_mode} mav={mav_ver}"
    )


def parse_param_value(payload: bytes) -> str:
    if len(payload) < 25:
        return ""
    value, param_count, param_index = struct.unpack_from("<fHH", payload, 0)
    name = payload[8:24].split(b"\0", 1)[0].decode("ascii", errors="replace")
    return f"{param_index + 1}/{param_count} {name}={value:.5g}"


def parse_vfr_hud(payload: bytes) -> str:
    if len(payload) < 20:
        return ""
    airspeed, groundspeed, alt, climb = struct.unpack_from("<ffff", payload, 0)
    heading, throttle = struct.unpack_from("<hH", payload, 16)
    return f"alt={alt:.2f}m climb={climb:.2f}m/s throttle={throttle}% heading={heading} air={airspeed:.1f}"


def parse_gps_raw_int(payload: bytes) -> str:
    if len(payload) < 30:
        return ""
    fix_type = payload[28]
    return f"fix_type={fix_type}"


def describe(msgid: int, payload: bytes) -> str:
    if msgid == 0:
        return parse_heartbeat(payload)
    if msgid == 22:
        return parse_param_value(payload)
    if msgid == 24:
        return parse_gps_raw_int(payload)
    if msgid == 44 and len(payload) >= 4:
        count = struct.unpack_from("<H", payload, 0)[0]
        return f"count={count}"
    if msgid == 74:
        return parse_vfr_hud(payload)
    if msgid == 253:
        return parse_statustext(payload)
    return ""


def feed_parser(buffer: bytearray):
    while buffer:
        stx = buffer[0]
        if stx not in (0xFE, 0xFD):
            del buffer[0]
            continue

        if stx == 0xFE:
            if len(buffer) < 8:
                return
            payload_len = buffer[1]
            frame_len = 6 + payload_len + 2
            if len(buffer) < frame_len:
                return
            seq, sysid, compid, msgid = buffer[2], buffer[3], buffer[4], buffer[5]
            payload = bytes(buffer[6:6 + payload_len])
            del buffer[:frame_len]
            yield msgid, sysid, compid, seq, payload
            continue

        if len(buffer) < 12:
            return
        payload_len = buffer[1]
        incompat = buffer[2]
        signature_len = 13 if incompat & 0x01 else 0
        frame_len = 10 + payload_len + 2 + signature_len
        if len(buffer) < frame_len:
            return
        seq, sysid, compid = buffer[4], buffer[5], buffer[6]
        msgid = buffer[7] | (buffer[8] << 8) | (buffer[9] << 16)
        payload = bytes(buffer[10:10 + payload_len])
        del buffer[:frame_len]
        yield msgid, sysid, compid, seq, payload


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", required=True)
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--seconds", type=float, default=20.0)
    args = parser.parse_args()

    if os.environ.get("AEROPICO_PROBE_CHILD") != "1":
        env = os.environ.copy()
        env["AEROPICO_PROBE_CHILD"] = "1"
        cmd = [sys.executable, __file__, "--port", args.port, "--baud", str(args.baud), "--seconds", str(args.seconds)]
        child = subprocess.Popen(cmd, env=env)
        try:
            return child.wait(timeout=args.seconds + 8.0)
        except subprocess.TimeoutExpired:
            child.kill()
            child.wait(timeout=2.0)
            print(f"[probe] serial open/read timed out; killed child process for {args.port}")
            return 3

    try:
        import serial  # type: ignore

        print(f"[probe] opening {args.port} @ {args.baud} with pyserial", flush=True)
        port = serial.Serial(
            args.port,
            args.baud,
            timeout=0.25,
            write_timeout=1.0,
            rtscts=False,
            dsrdtr=False,
            exclusive=False,
        )
        try:
            port.dtr = True
            port.rts = True
            port.reset_input_buffer()
            print(f"[probe] listening on {args.port} @ {args.baud} for {args.seconds:g}s")
            start = time.monotonic()
            last_packet = start
            raw_bytes = 0
            counts = {}
            buf = bytearray()
            while time.monotonic() - start < args.seconds:
                chunk = port.read(512)
                if not chunk:
                    continue
                raw_bytes += len(chunk)
                buf.extend(chunk)
                for msgid, sysid, compid, seq, payload in feed_parser(buf):
                    last_packet = time.monotonic()
                    counts[msgid] = counts.get(msgid, 0) + 1
                    name = MSG_NAMES.get(msgid, f"MSG_{msgid}")
                    detail = describe(msgid, payload)
                    suffix = f" {detail}" if detail else ""
                    print(f"[{time.monotonic() - start:6.2f}s] {name:<16} sys={sysid} comp={compid} seq={seq}{suffix}")

            print("[probe] summary:")
            print(f"  raw bytes: {raw_bytes}")
            if counts:
                for msgid, count in sorted(counts.items()):
                    print(f"  {MSG_NAMES.get(msgid, f'MSG_{msgid}')}: {count}")
                return 0
            print("  no MAVLink frames decoded")
            print(f"  last raw activity: {time.monotonic() - last_packet:.1f}s ago")
            return 2
        finally:
            port.close()
    except ImportError:
        pass

    def _open_timeout(_signum, _frame):
        raise TimeoutError(f"opening {args.port} timed out")

    print(f"[probe] opening {args.port} @ {args.baud}", flush=True)
    old_handler = signal.signal(signal.SIGALRM, _open_timeout)
    signal.alarm(4)
    try:
        fd = os.open(args.port, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
    finally:
        signal.alarm(0)
        signal.signal(signal.SIGALRM, old_handler)
    try:
        configure_serial(fd, args.baud)
        print(f"[probe] listening on {args.port} @ {args.baud} for {args.seconds:g}s")
        start = time.monotonic()
        last_packet = start
        raw_bytes = 0
        counts = {}
        buf = bytearray()
        while time.monotonic() - start < args.seconds:
            readable, _, _ = select.select([fd], [], [], 0.25)
            if not readable:
                continue
            chunk = os.read(fd, 512)
            if not chunk:
                continue
            raw_bytes += len(chunk)
            buf.extend(chunk)
            for msgid, sysid, compid, seq, payload in feed_parser(buf):
                last_packet = time.monotonic()
                counts[msgid] = counts.get(msgid, 0) + 1
                name = MSG_NAMES.get(msgid, f"MSG_{msgid}")
                detail = describe(msgid, payload)
                suffix = f" {detail}" if detail else ""
                print(f"[{time.monotonic() - start:6.2f}s] {name:<16} sys={sysid} comp={compid} seq={seq}{suffix}")

        print("[probe] summary:")
        print(f"  raw bytes: {raw_bytes}")
        if counts:
            for msgid, count in sorted(counts.items()):
                print(f"  {MSG_NAMES.get(msgid, f'MSG_{msgid}')}: {count}")
            return 0
        print("  no MAVLink frames decoded")
        print(f"  last raw activity: {time.monotonic() - last_packet:.1f}s ago")
        return 2
    finally:
        os.close(fd)


if __name__ == "__main__":
    sys.exit(main())
