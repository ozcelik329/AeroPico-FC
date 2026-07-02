#ifndef BLACKBOX_H
#define BLACKBOX_H

#include <Arduino.h>
#include "../drivers/PioUart.h"

// Blackbox kayıt formatı (CSV benzeri, ESP32 SD'ye yazar)
// Format: "BB,<timestamp>,<roll>,<pitch>,<yaw>,<gx>,<gy>,<gz>,<thr>,<ail>,<ele>,<rud>,<failsafe>\n"

class Blackbox {
  public:
    void init();
    void log(float roll, float pitch, float yaw,
             float gx, float gy, float gz,
             uint16_t throttle, uint16_t aileron,
             uint16_t elevator, uint16_t rudder,
             bool failsafe);

  private:
    bool _enabled = false;
};

extern Blackbox blackbox;

#endif