#ifndef IMU_H
#define IMU_H

#include <Arduino.h>

class IMUManager {
  public:
    void init();
    // Baro ve Mag verileri eklendi
    void update(float ax, float ay, float az, float gx, float gy, float gz, float mx, float my, float mz, float pressure);
    
    float getRoll();
    float getPitch();
    float getYaw();
    float getAltitude(); // Basınçtan yükseklik hesabı için

  private:
    float roll, pitch, yaw, altitude;
};

#endif