#ifndef SENSOR_FUSION_H
#define SENSOR_FUSION_H

#include <Arduino.h>
#include "pico/platform.h"

class SensorFusion {
  public:
    SensorFusion();
    void init(float beta = 0.1f);

    void __not_in_flash_func(update)(float gx, float gy, float gz,
                                     float ax, float ay, float az,
                                     float mx, float my, float mz);

    void __not_in_flash_func(updateIMU)(float gx, float gy, float gz,
                                        float ax, float ay, float az);

    // Sıcaklık kompanzasyonu
    void setTemperature(float tempC);

    float getRoll() const;
    float getPitch() const;
    float getYaw() const;

  private:
    float q0, q1, q2, q3;
    float roll, pitch, yaw;
    float beta;
    uint32_t lastUpdate;

    // Sıcaklık kompanzasyonu
    float _tempC         = 25.0f;  // Referans sıcaklık
    float _gyroTempCoeff = 0.004f; // °C başına drift katsayısı (MPU6050 datasheet)

    void __not_in_flash_func(computeAngles)();
    static float invSqrt(float x);
};

#endif