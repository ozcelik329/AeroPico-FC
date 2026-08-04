#ifndef BLACKBOX_SINK_H
#define BLACKBOX_SINK_H

#include <Arduino.h>

class IBlackboxSink {
  public:
    virtual ~IBlackboxSink() {}
    virtual bool begin() = 0;
    virtual size_t availableForWrite() const = 0;
    virtual size_t write(const uint8_t* data, size_t length) = 0;
};

#endif
