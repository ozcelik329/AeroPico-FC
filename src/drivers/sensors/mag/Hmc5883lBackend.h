#ifndef HMC5883L_BACKEND_H
#define HMC5883L_BACKEND_H

#include <Arduino.h>

class Hmc5883lBackend {
  public:
    static constexpr float SCALE_MILLI_GAUSS_PER_COUNT = 0.92f;

    void scaleRaw(int16_t rawX,
                  int16_t rawY,
                  int16_t rawZ,
                  float& mx,
                  float& my,
                  float& mz) const;
};

#endif
