#ifndef ESP32_CAM_LINK_H
#define ESP32_CAM_LINK_H

#include <stdint.h>
#include "../../config.h"
#include "../../hal/HAL_UART.h"

struct Esp32CamStatus {
    bool enabled;
    bool linkActive;
    uint32_t lastByteMs;
    uint32_t bytesRx;
};

class Esp32CamLink {
  public:
    void init(IHALUART* uart, bool enabled = ESP32_CAM_LINK_ENABLED,
              uint32_t baud = ESP32_CAM_UART_BAUD);
    void update(uint32_t nowMs);
    Esp32CamStatus status() const { return _status; }

  private:
    static constexpr uint32_t LINK_TIMEOUT_MS = 3000;
    IHALUART* _uart = nullptr;
    Esp32CamStatus _status = {};
};

#endif
