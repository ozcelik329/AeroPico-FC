#ifndef HAL_H
#define HAL_H

#include <Arduino.h>
#include "../config.h"

// Donanım Soyutlama Katmanı (HAL)
// Gerçek donanım ve SITL simülasyonu aynı arayüzü kullanır

struct HAL_IMUData {
    float ax, ay, az;   // ivme (g)
    float gx, gy, gz;   // açısal hız (°/s)
    float tempC;         // sıcaklık
    bool  valid;
};

struct HAL_RCData {
    uint16_t ch[4];     // 1000-2000 µs
    bool valid;
};

struct HAL_ServoOutput {
    uint16_t aileron;
    uint16_t elevator;
    uint16_t rudder;
    uint16_t throttle;
};

class HAL {
  public:
    static void init();

    // Sensör okuma
    static HAL_IMUData  readIMU();
    static HAL_RCData   readRC();

    // Servo yazma
    static void writeServos(const HAL_ServoOutput& out);

    // Zaman
    static uint32_t micros_hal();
    static uint32_t millis_hal();

    static bool isSITL() {
        #ifdef SITL_MODE
            return true;
        #else
            return false;
        #endif
    }
};

#endif