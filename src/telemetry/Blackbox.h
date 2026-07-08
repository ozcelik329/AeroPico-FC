#ifndef BLACKBOX_H
#define BLACKBOX_H

#include <Arduino.h>
#include "../drivers/PioUart.h"
#include "../types.h"

// Blackbox kayıt formatı (CSV benzeri, ESP32 SD'ye yazar)
// Format: "BB,<timestamp>,<roll>,<pitch>,<yaw>,<gx>,<gy>,<gz>,<thr>,<ail>,<ele>,<rud>,<failsafe>,<sensorHealth>\n"

class Blackbox {
  public:
    void init();
    void setLogRateHz(uint8_t hz);
    void log(float roll, float pitch, float yaw,
             float gx, float gy, float gz,
             uint16_t throttle, uint16_t aileron,
             uint16_t elevator, uint16_t rudder,
             bool failsafe,
             SensorHealth sensorHealth);
    void logTimingBudget(const TimingBudgetStatus& status);

  private:
    bool _enabled = false;
    uint16_t _logPeriodMs = 200;
    uint32_t _lastLogMs = 0;
};

extern Blackbox blackbox;

#endif
