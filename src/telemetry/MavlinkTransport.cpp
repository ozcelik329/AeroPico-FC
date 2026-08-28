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
    if (!bytes || len == 0) {
        return 0;
    }
#if TELEMETRY_UART_ENABLED
    const size_t pioWritten = telemetryUart.write(bytes, len);
#else
    const size_t pioWritten = 0;
#endif
#if MAVLINK_USB_ENABLED
    const bool usbQueued = UsbCdcTx::enqueue(bytes, len);
    const size_t usbWritten = usbQueued ? len : 0;
    if (!usbQueued) {
        _usbDroppedPackets++;
    }
#else
    const size_t usbWritten = 0;
#endif
#ifdef UNIT_TEST
    const size_t room = CAPTURE_CAPACITY - _captureSize;
    const size_t copyLen = len < room ? len : room;
    for (size_t i = 0; i < copyLen; i++) {
        _capture[_captureSize + i] = bytes[i];
    }
    _captureSize += copyLen;
#endif
    return pioWritten > usbWritten ? pioWritten : usbWritten;
}

void MavlinkTransport::serviceUsbTx() {
#if MAVLINK_USB_ENABLED
    UsbCdcTx::service();
#endif
}

int MavlinkTransport::available() {
#if MAVLINK_USB_ENABLED && TELEMETRY_UART_ENABLED
#if MAVLINK_TELEMETRY_RX_ENABLED
    return telemetryUart.available() || Serial.available();
#else
    return Serial.available();
#endif
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
#if MAVLINK_TELEMETRY_RX_ENABLED
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
#else
    return Serial.available() ? Serial.read() : -1;
#endif
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
