(function () {
  const STX_V1 = 0xfe;
  const STX_V2 = 0xfd;
  const MAVLINK_VERSION = 1;
  const SYS_ID = 255;
  const COMP_ID = 190;

  const MSG = {
    HEARTBEAT: 0,
    SYS_STATUS: 1,
    PARAM_REQUEST_LIST: 21,
    PARAM_VALUE: 22,
    PARAM_SET: 23,
    COMMAND_LONG: 76,
    COMMAND_ACK: 77,
    STATUSTEXT: 253
  };

  const CRC_EXTRA = {
    0: 50,
    1: 124,
    21: 159,
    22: 220,
    23: 168,
    76: 152,
    77: 143,
    253: 83
  };

  function crcAccumulate(byte, crc) {
    let tmp = byte ^ (crc & 0xff);
    tmp = (tmp ^ (tmp << 4)) & 0xff;
    return ((crc >> 8) ^ (tmp << 8) ^ (tmp << 3) ^ (tmp >> 4)) & 0xffff;
  }

  function x25(bytes, extra) {
    let crc = 0xffff;
    for (const byte of bytes) crc = crcAccumulate(byte, crc);
    crc = crcAccumulate(extra, crc);
    return crc;
  }

  function writeString(view, offset, length, value) {
    for (let i = 0; i < length; i++) {
      view.setUint8(offset + i, i < value.length ? value.charCodeAt(i) : 0);
    }
  }

  function readString(view, offset, length) {
    let out = "";
    for (let i = 0; i < length; i++) {
      const c = view.getUint8(offset + i);
      if (c === 0) break;
      out += String.fromCharCode(c);
    }
    return out;
  }

  class Encoder {
    constructor() {
      this.seq = 0;
    }

    frame(msgId, payload) {
      const header = new Uint8Array([payload.length, this.seq++ & 0xff, SYS_ID, COMP_ID, msgId]);
      const crcBytes = new Uint8Array(header.length + payload.length);
      crcBytes.set(header, 0);
      crcBytes.set(payload, header.length);
      const crc = x25(crcBytes, CRC_EXTRA[msgId] || 0);
      const frame = new Uint8Array(1 + header.length + payload.length + 2);
      frame[0] = STX_V1;
      frame.set(header, 1);
      frame.set(payload, 1 + header.length);
      frame[frame.length - 2] = crc & 0xff;
      frame[frame.length - 1] = crc >> 8;
      return frame;
    }

    paramRequestList(targetSystem = 1, targetComponent = 1) {
      const payload = new Uint8Array(2);
      payload[0] = targetSystem;
      payload[1] = targetComponent;
      return this.frame(MSG.PARAM_REQUEST_LIST, payload);
    }

    paramSet(name, value, targetSystem = 1, targetComponent = 1) {
      const payload = new Uint8Array(23);
      const view = new DataView(payload.buffer);
      view.setFloat32(0, Number(value), true);
      view.setUint8(4, targetSystem);
      view.setUint8(5, targetComponent);
      writeString(view, 6, 16, name);
      view.setUint8(22, 9);
      return this.frame(MSG.PARAM_SET, payload);
    }

    commandLong(command, params = [], targetSystem = 1, targetComponent = 1) {
      const payload = new Uint8Array(33);
      const view = new DataView(payload.buffer);
      for (let i = 0; i < 7; i++) {
        view.setFloat32(i * 4, Number(params[i] || 0), true);
      }
      view.setUint16(28, command, true);
      view.setUint8(30, targetSystem);
      view.setUint8(31, targetComponent);
      view.setUint8(32, 0);
      return this.frame(MSG.COMMAND_LONG, payload);
    }

    aeroPicoService(action, p2 = 0, p3 = 0, p4 = 0) {
      return this.commandLong(31010, [action, p2, p3, p4, 0, 0, 0]);
    }
  }

  class Parser {
    constructor(onMessage) {
      this.onMessage = onMessage;
      this.frame = new Uint8Array(263);
      this.reset();
    }

    reset() {
      this.state = "stx";
      this.expected = 0;
      this.index = 0;
      this.version = 1;
    }

    pushBytes(bytes) {
      for (const byte of bytes) this.push(byte);
    }

    push(byte) {
      if (this.state === "stx") {
        if (byte === STX_V1 || byte === STX_V2) {
          this.version = byte === STX_V2 ? 2 : 1;
          this.frame[0] = byte;
          this.index = 1;
          this.state = "len";
        }
        return;
      }

      if (this.index >= this.frame.length) {
        this.reset();
        return;
      }

      this.frame[this.index++] = byte;
      if (this.state === "len") {
        this.expected = byte + (this.version === 2 ? 12 : 8);
        this.state = "body";
        return;
      }

      if (this.index >= this.expected) {
        this.consumeFrame(this.frame);
        this.reset();
      }
    }

    consumeFrame(frame) {
      const len = frame[1];
      const isV2 = frame[0] === STX_V2;
      const payloadStart = isV2 ? 10 : 6;
      const msgId = isV2
        ? (frame[7] | (frame[8] << 8) | (frame[9] << 16))
        : frame[5];
      if (CRC_EXTRA[msgId] === undefined) return;
      const payloadEnd = payloadStart + len;
      const crcIn = frame[payloadEnd] | (frame[payloadEnd + 1] << 8);
      const crcBytes = frame.subarray(1, payloadEnd);
      const crcCalc = x25(crcBytes, CRC_EXTRA[msgId] || 0);
      if (crcCalc !== crcIn) return;

      const payload = frame.subarray(payloadStart, payloadEnd);
      const view = new DataView(payload.buffer, payload.byteOffset, payload.byteLength);
      if (msgId === MSG.HEARTBEAT && payload.length >= 9) {
        this.onMessage({
          type: "heartbeat",
          autopilot: view.getUint8(5),
          baseMode: view.getUint8(6),
          systemStatus: view.getUint8(7),
          mavlinkVersion: MAVLINK_VERSION
        });
        return;
      }

      if (msgId === MSG.PARAM_VALUE && payload.length >= 25) {
        this.onMessage({
          type: "param",
          name: readString(view, 8, 16),
          value: view.getFloat32(0, true),
          count: view.getUint16(4, true),
          index: view.getUint16(6, true)
        });
        return;
      }

      if (msgId === MSG.SYS_STATUS && payload.length >= 31) {
        this.onMessage({
          type: "sysStatus",
          voltageBatteryMv: view.getUint16(14, true),
          currentBatteryCa: view.getInt16(16, true),
          batteryRemaining: view.getInt8(30)
        });
        return;
      }

      if (msgId === MSG.COMMAND_ACK && payload.length >= 3) {
        this.onMessage({
          type: "commandAck",
          command: view.getUint16(0, true),
          result: view.getUint8(2)
        });
        return;
      }

      if (msgId === MSG.STATUSTEXT && payload.length >= 51) {
        this.onMessage({
          type: "statusText",
          severity: view.getUint8(0),
          text: readString(view, 1, 50)
        });
      }
    }
  }

  window.AeroPicoMavlink = { Encoder, Parser };
})();
