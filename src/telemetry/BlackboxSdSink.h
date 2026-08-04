#ifndef BLACKBOX_SD_SINK_H
#define BLACKBOX_SD_SINK_H

#include "BlackboxSink.h"

#if !defined(UNIT_TEST)
#include <SD.h>

class BlackboxSdSink : public IBlackboxSink {
  public:
    BlackboxSdSink(uint8_t csPin, uint32_t spiHz, const char* path)
        : _csPin(csPin), _spiHz(spiHz), _path(path) {}

    bool begin() override;
    size_t availableForWrite() const override;
    size_t write(const uint8_t* data, size_t length) override;

  private:
    static constexpr uint16_t FLUSH_BYTES = 1024;

    uint8_t _csPin;
    uint32_t _spiHz;
    const char* _path;
    File _file;
    uint16_t _bytesSinceFlush = 0;
    bool _ready = false;
};
#endif

#endif
