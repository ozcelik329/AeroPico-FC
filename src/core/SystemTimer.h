#ifndef SYSTEM_TIMER_H
#define SYSTEM_TIMER_H

#include <Arduino.h>
#include "pico/multicore.h"

class SystemTimer {
  public:
    static void init();           // Servo çıkışı + Core 1 başlatma
    static void core1_entry();    // PID + Mixer döngüsü (Core 1)

    static volatile bool is_running;
};

// Geriye uyumluluk için alias
using SystemManager = SystemTimer;

#endif
