#include "MavlinkTransport.h"

MavlinkTransport mavlinkTransport;

void MavlinkTransport::init(uint32_t baud) {
#if TELEMETRY_UART_ENABLED
    telemetryUart.init(baud);
#else
    (void)baud;
#endif
}

size_t MavlinkTransport::writePacket(const uint8_t* bytes, size_t len) {
#if TELEMETRY_UART_ENABLED
    const size_t pioWritten = telemetryUart.write(bytes, len);
#else
    const size_t pioWritten = 0;
#endif
#if MAVLINK_USB_ENABLED
    Serial.write(bytes, len);
#endif
#ifdef UNIT_TEST
    const size_t room = CAPTURE_CAPACITY - _captureSize;
    const size_t copyLen = len < room ? len : room;
    for (size_t i = 0; i < copyLen; i++) {
        _capture[_captureSize + i] = bytes[i];
    }
    _captureSize += copyLen;
#endif
    return pioWritten;
}

int MavlinkTransport::available() {
#if MAVLINK_USB_ENABLED && TELEMETRY_UART_ENABLED
    return telemetryUart.available() || Serial.available();
#elif MAVLINK_USB_ENABLED
    return Serial.available();
#elif TELEMETRY_UART_ENABLED
    return telemetryUart.available();
#else
    return 0;
#endif
}

int MavlinkTransport::read() {
#if MAVLINK_USB_ENABLED && TELEMETRY_UART_ENABLED
    if (_readUsbNext && Serial.available()) {
        _readUsbNext = false;
        return Serial.read();
    }
    if (telemetryUart.available()) {
        _readUsbNext = true;
        return telemetryUart.read();
    }
    if (Serial.available()) {
        return Serial.read();
    }
    return -1;
#elif MAVLINK_USB_ENABLED
    return Serial.available() ? Serial.read() : -1;
#elif TELEMETRY_UART_ENABLED
    return telemetryUart.read();
#else
    return -1;
#endif
}

#ifdef UNIT_TEST
void MavlinkTransport::resetCapture() {
    _captureSize = 0;
}
#endif
