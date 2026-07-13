#ifndef GPS_MANAGER_H
#define GPS_MANAGER_H

#include "../../config.h"
#include "../../hal/HAL_UART.h"
#include "../../types.h"
#include "GpsParser.h"

class GpsManager {
  public:
    void init(IHALUART* uart, bool enabled = GPS_MODULE_ENABLED, uint32_t baud = GPS_UART_BAUD);
    void update(uint32_t nowMs);
    GpsStatus status() const { return _status; }
    bool isAvailable() const { return _status.enabled && _status.fix.valid; }
    SensorCapabilityStatus capabilities() const;

  private:
    IHALUART* _uart = nullptr;
    GpsParser _parser;
    GpsStatus _status = {};
};

#endif
