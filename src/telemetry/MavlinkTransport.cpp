#include "MavlinkTransport.h"

MavlinkTransport mavlinkTransport;

void MavlinkTransport::init(uint32_t baud) {
    espUart.init(baud);
}

size_t MavlinkTransport::writePacket(const uint8_t* bytes, size_t len) {
    const size_t pioWritten = espUart.write(bytes, len);
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
#if MAVLINK_USB_ENABLED
    return espUart.available() || Serial.available();
#else
    return espUart.available();
#endif
}

int MavlinkTransport::read() {
#if MAVLINK_USB_ENABLED
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
#else
    return espUart.read();
#endif
}

#ifdef UNIT_TEST
void MavlinkTransport::resetCapture() {
    _captureSize = 0;
}
#endif
