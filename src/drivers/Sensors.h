#ifndef SENSORS_H
#define SENSORS_H

void initSensors();    // MPU6050, HMC5883L ve BMP180 başlatma
void updateSensors();  // Üç sensörden de veri okuma
void readAccelGyro();
void readCompass();
void readBaro();


#endif



