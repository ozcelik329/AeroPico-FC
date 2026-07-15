#include "MavlinkTransport.h"

MavlinkTransport mavlinkTransport;

void MavlinkTransport::init(uint32_t baud) {
#if ESP32_CAM_LINK_ENABLED
    espUart.init(baud);
#else
    (void)baud;
#endif
}

size_t MavlinkTransport::writePacket(const uint8_t* bytes, size_t len) {
    size_t written = 0;
#if ESP32_CAM_LINK_ENABLED
    written = espUart.write(bytes, len);
#endif
#if MAVLINK_USB_ENABLED
    written = Serial.write(bytes, len);
#ifndef UNIT_TEST
    Serial.flush();
#endif
#endif
#ifdef UNIT_TEST
    const size_t room = CAPTURE_CAPACITY - _captureSize;
    const size_t copyLen = len < room ? len : room;
    for (size_t i = 0; i < copyLen; i++) {
        _capture[_captureSize + i] = bytes[i];
    }
    _captureSize += copyLen;
#endif
    return written;
}

int MavlinkTransport::available() {
#if ESP32_CAM_LINK_ENABLED && MAVLINK_USB_ENABLED
    return espUart.available() || Serial.available();
#elif MAVLINK_USB_ENABLED
    return Serial.available();
#elif ESP32_CAM_LINK_ENABLED
    return espUart.available();
#else
    return 0;
#endif
}

int MavlinkTransport::read() {
#if ESP32_CAM_LINK_ENABLED && MAVLINK_USB_ENABLED
    if (_readUsbNext && Serial.available()) {
        _readUsbNext = false;
        return Serial.read();
    }
    if (espUart.available()) {
        _readUsbNext = true;
        return espUart.read();
    }
    if (Serial.available()) {
        return Serial.read();
    }
    return -1;
#elif MAVLINK_USB_ENABLED
    return Serial.available() ? Serial.read() : -1;
#elif ESP32_CAM_LINK_ENABLED
    return espUart.read();
#else
    return -1;
#endif
}

#ifdef UNIT_TEST
void MavlinkTransport::resetCapture() {
    _captureSize = 0;
}
#endif
