#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>
#include <Adafruit_MPU6050.h>
#include "../config.h"

#ifdef USE_GY87
  #include <Adafruit_HMC5883_U.h>
  #include <Adafruit_BMP085_U.h>
#endif

class SensorManager {
  public:
    void init();
    void update();

    // Ham veriler (main.cpp ve IMU.cpp üzerinden erişilebilir)
    float ax, ay, az;
    float gx, gy, gz;
    
    #ifdef USE_GY87
      float mx, my, mz;
      float pressure;
    #endif

  private:
    Adafruit_MPU6050 mpu;
    #ifdef USE_GY87
      Adafruit_HMC5883_Unified mag;
      Adafruit_BMP085_Unified bmp;
    #endif
};

#endif