#ifndef MAVLINK_TRANSPORT_H
#define MAVLINK_TRANSPORT_H

#include <Arduino.h>
#include "../config.h"
#include "../drivers/PioUart.h"

class MavlinkTransport {
  public:
    void init(uint32_t baud);
    size_t writePacket(const uint8_t* bytes, size_t len);
    int available();
    int read();

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
};

extern MavlinkTransport mavlinkTransport;

#endif
