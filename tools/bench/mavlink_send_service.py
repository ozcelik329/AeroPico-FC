#!/usr/bin/env python3
"""Send small AeroPico MAVLink service commands during bench bring-up.

This script intentionally depends only on pyserial at runtime so it works on
Windows where the dependency-free POSIX probe cannot use termios/fcntl.
"""

import argparse
import struct
import sys
import time


SYS_ID = 255
COMP_ID = 190
TARGET_SYSTEM = 1
TARGET_COMPONENT = 1

MSG_COMMAND_LONG = 76
MSG_COMMAND_ACK = 77
MSG_RC_CHANNELS_OVERRIDE = 70
MSG_STATUSTEXT = 253

CRC_EXTRA = {
    MSG_RC_CHANNELS_OVERRIDE: 124,
    MSG_COMMAND_LONG: 152,
    MSG_COMMAND_ACK: 143,
    MSG_STATUSTEXT: 83,
}

MAV_CMD_COMPONENT_ARM_DISARM = 400
MAV_CMD_USER_1 = 31010

AEROPICO_CMD_SERVO_TEST = 4

SURFACES = {
    "all": 0,
    "aileron": 1,
    "elevator": 2,
    "rudder": 3,
    "throttle": 4,
}


def crc_accumulate(byte: int, crc: int) -> int:
    tmp = byte ^ (crc & 0xFF)
    tmp = (tmp ^ (tmp << 4)) & 0xFF
    return ((crc >> 8) ^ (tmp << 8) ^ (tmp << 3) ^ (tmp >> 4)) & 0xFFFF


def x25(data: bytes, extra: int) -> int:
    crc = 0xFFFF
    for byte in data:
        crc = crc_accumulate(byte, crc)
    return crc_accumulate(extra, crc)


def mavlink_v1_frame(seq: int, msgid: int, payload: bytes) -> bytes:
    header = struct.pack("<BBBBB", len(payload), seq & 0xFF, SYS_ID, COMP_ID, msgid)
    crc = x25(header + payload, CRC_EXTRA[msgid])
    return b"\xFE" + header + payload + struct.pack("<H", crc)


def command_long(seq: int, command: int, params) -> bytes:
    padded = list(params[:7]) + [0.0] * (7 - len(params))
    payload = struct.pack(
        "<7fHBBB",
        *[float(value) for value in padded],
        command,
        TARGET_SYSTEM,
        TARGET_COMPONENT,
        0,
    )
    return mavlink_v1_frame(seq, MSG_COMMAND_LONG, payload)


def rc_override_frame(seq: int,
                      aileron: int,
                      elevator: int,
                      throttle: int,
                      rudder: int,
                      mode: int) -> bytes:
    channels = [
        aileron, elevator, throttle, rudder, mode,
        0, 0, 0,
    ]
    payload = struct.pack(
        "<8HBB",
        *[int(value) for value in channels],
        TARGET_SYSTEM,
        TARGET_COMPONENT,
    )
    return mavlink_v1_frame(seq, MSG_RC_CHANNELS_OVERRIDE, payload)


def servo_test_frame(seq: int, surface: int, pulse_us: int, duration_ms: int) -> bytes:
    return command_long(
        seq,
        MAV_CMD_USER_1,
        [AEROPICO_CMD_SERVO_TEST, surface, pulse_us, duration_ms, 0, 0, 0],
    )


def parse_statustext(payload: bytes) -> str:
    if len(payload) < 2:
        return ""
    return payload[1:51].split(b"\0", 1)[0].decode("ascii", errors="replace")


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


def listen_for_ack(port, seconds: float) -> int:
    start = time.monotonic()
    buffer = bytearray()
    saw_ack = False
    while time.monotonic() - start < seconds:
        chunk = port.read(256)
        if not chunk:
            continue
        buffer.extend(chunk)
        for msgid, _sysid, _compid, _seq, payload in feed_parser(buffer):
            if msgid == MSG_COMMAND_ACK and len(payload) >= 3:
                command, result = struct.unpack_from("<HB", payload, 0)
                print(f"COMMAND_ACK command={command} result={result}")
                saw_ack = True
            elif msgid == MSG_STATUSTEXT:
                text = parse_statustext(payload)
                if text:
                    print(f"STATUSTEXT {text}")
    return 0 if saw_ack else 2


def main() -> int:
    parser = argparse.ArgumentParser(description="AeroPico MAVLink bench service sender")
    parser.add_argument("--port", required=True, help="Serial port, e.g. COM8 or /dev/cu.usbmodem1301")
    parser.add_argument("--baud", type=int, default=115200)
    sub = parser.add_subparsers(dest="command", required=True)

    servo = sub.add_parser("servo-test", help="Run a disarmed-safe firmware servo test")
    servo.add_argument("--surface", choices=sorted(SURFACES), default="all")
    servo.add_argument("--pulse", type=int, default=1800, help="Pulse width in microseconds")
    servo.add_argument("--duration", type=int, default=1000, help="Duration in milliseconds")
    servo.add_argument("--listen", type=float, default=3.0, help="Seconds to wait for ACK/status")

    rc = sub.add_parser("rc-override", help="Send temporary RC override for bench arm/preflight")
    rc.add_argument("--aileron", type=int, default=1500)
    rc.add_argument("--elevator", type=int, default=1500)
    rc.add_argument("--throttle", type=int, default=1000)
    rc.add_argument("--rudder", type=int, default=1500)
    rc.add_argument("--mode", type=int, default=1000, help="Mode channel pulse; >=1500 means stabilize in firmware mapping")
    rc.add_argument("--seconds", type=float, default=3.0)
    rc.add_argument("--rate-hz", type=float, default=10.0)

    arm = sub.add_parser("arm", help="Send MAV_CMD_COMPONENT_ARM_DISARM")
    arm.add_argument("--disarm", action="store_true")
    arm.add_argument("--force", action="store_true")
    arm.add_argument("--listen", type=float, default=3.0)

    args = parser.parse_args()
    try:
        import serial  # type: ignore
    except ImportError:
        print("pyserial is required: py -3 -m pip install pyserial", file=sys.stderr)
        return 1

    with serial.Serial(args.port, args.baud, timeout=0.2, write_timeout=1.0) as port:
        port.reset_input_buffer()
        if args.command == "servo-test":
            surface = SURFACES[args.surface]
            frame = servo_test_frame(0, surface, args.pulse, args.duration)
            port.write(frame)
            port.flush()
            print(
                f"sent servo-test surface={args.surface} "
                f"pulse={args.pulse}us duration={args.duration}ms"
            )
            return listen_for_ack(port, args.listen)
        if args.command == "rc-override":
            period = 1.0 / max(1.0, args.rate_hz)
            end = time.monotonic() + max(0.1, args.seconds)
            seq = 0
            sent = 0
            while time.monotonic() < end:
                port.write(rc_override_frame(
                    seq,
                    args.aileron,
                    args.elevator,
                    args.throttle,
                    args.rudder,
                    args.mode,
                ))
                seq = (seq + 1) & 0xFF
                sent += 1
                time.sleep(period)
            port.flush()
            print(
                f"sent rc-override frames={sent} "
                f"ail={args.aileron} ele={args.elevator} thr={args.throttle} rud={args.rudder} mode={args.mode}"
            )
            return 0
        if args.command == "arm":
            arm_value = 0.0 if args.disarm else 1.0
            force_value = 21196.0 if args.force else 0.0
            port.write(command_long(0, MAV_CMD_COMPONENT_ARM_DISARM, [arm_value, force_value, 0, 0, 0, 0, 0]))
            port.flush()
            print("sent disarm" if args.disarm else "sent arm")
            return listen_for_ack(port, args.listen)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
