#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include "../config.h"

#ifdef USE_GY87
  #include <Adafruit_HMC5883_U.h>
  #include <Adafruit_BMP085_U.h>
#endif

// DMA-ready ping-pong buffer
struct SensorBuffer {
    float ax, ay, az;
    float gx, gy, gz;
    #ifdef USE_GY87
        float mx, my, mz;
        float pressure;
    #endif
    uint32_t timestamp;
    bool valid;
};

class SensorManager {
  public:
    void init();
    void update();

    // PID tarafı bu fonksiyonu çağırır — mutex korumalı
    SensorBuffer getLatest();

    #ifdef USE_GY87
        bool hasMag  = false;
        bool hasBaro = false;
    #endif

  private:
    Adafruit_MPU6050 mpu;
    #ifdef USE_GY87
        Adafruit_HMC5883_Unified mag = Adafruit_HMC5883_Unified(12345);
        Adafruit_BMP085_Unified  bmp = Adafruit_BMP085_Unified(10085);
    #endif

    // Ping-pong buffer — biri yazılırken diğeri okunur
    SensorBuffer _buf[2];
    volatile uint8_t _writeIdx = 0;
    mutex_t _mutex;
};

#endif