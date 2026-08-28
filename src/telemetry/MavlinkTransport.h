#ifndef MAVLINK_TRANSPORT_H
#define MAVLINK_TRANSPORT_H

#include <Arduino.h>
#include "board/Config.h"
#include "../drivers/PioUart.h"
#include "../utils/UsbCdcTx.h"

class MavlinkTransport {
  public:
    void init(uint32_t baud);
    size_t writePacket(const uint8_t* bytes, size_t len);
    void serviceUsbTx();
    int available();
    int read();
    uint32_t usbDroppedPackets() const { return _usbDroppedPackets; }

#ifdef UNIT_TEST
    void resetCapture();
    const uint8_t* capture() const { return _capture; }
    size_t captureSize() const { return _captureSize; }
#endif

  private:
#ifdef UNIT_TEST
    static constexpr size_t CAPTURE_CAPACITY = 768;
    uint8_t _capture[CAPTURE_CAPACITY] = {};
    size_t _captureSize = 0;
#endif
    bool _readUsbNext = false;
    uint32_t _usbDroppedPackets = 0;
};

extern MavlinkTransport mavlinkTransport;

#endif
