#ifndef RX_H
#define RX_H

#include <Arduino.h>
#include "../config.h"
#include "sbus.h"
#include "IDrivers.h"

class RXManager : public IRxDriver {
  public:
    void init();
    void update();
    bool isValid() const;
    bool isFailsafe() const;       // Failsafe aktif mi?
    uint32_t lastValidMs() const;  // Son geçerli sinyal zamanı
    uint16_t getChannel(int ch) const;

  private:
    uint16_t channels[16];
    bool valid         = false;
    bool _failsafe     = false;
    uint32_t _lastValidTime = 0;
};

#endif