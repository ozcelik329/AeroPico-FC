#include "Sensors.h"

void SensorManager::init() {
    Wire.begin();
    
    // MPU6050 Başlatma
    if (!mpu.begin()) {
        Serial.println("MPU6050 bulunamadi!");
    }
    
    #ifdef USE_GY87
      mag.begin();
      bmp.begin();
    #endif
}

void SensorManager::update() {
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    // İvme ve Jiroskop verilerini ata
    this->ax = a.acceleration.x; this->ay = a.acceleration.y; this->az = a.acceleration.z;
    this->gx = g.gyro.x;         this->gy = g.gyro.y;         this->gz = g.gyro.z;

    #ifdef USE_GY87
      sensors_event_t mag_event, bmp_event;
      mag.getEvent(&mag_event);
      bmp.getEvent(&bmp_event);

      this->mx = mag_event.magnetic.x;
      this->my = mag_event.magnetic.y;
      this->mz = mag_event.magnetic.z;
      this->pressure = bmp_event.pressure;
    #endif
}