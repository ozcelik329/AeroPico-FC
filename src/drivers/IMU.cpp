#include "IMU.h"
#include <math.h>

void IMUManager::init() {
    roll = pitch = yaw = altitude = 0;
}

void IMUManager::update(float ax, float ay, float az, float gx, float gy, float gz, float mx, float my, float mz, float pressure) {
    // 1. Açı Hesaplama (Tilt-compensated olmayan basit yöntem)
    this->roll = atan2(ay, az) * RAD_TO_DEG;
    this->pitch = atan2(-ax, sqrt(ay * ay + az * az)) * RAD_TO_DEG;
    
    // 2. Pusula (Yaw) - GY-87 Magnetometre kullanımı
    // Manyetik kuzeyi hesapla
    float heading = atan2(my, mx);
    if(heading < 0) heading += 2 * PI;
    this->yaw = heading * RAD_TO_DEG;

    // 3. Basınçtan İrtifa (Barometre)
    // Basit deniz seviyesi formülü: h = 44330 * (1 - (p/p0)^(1/5.255))
    this->altitude = 44330.0 * (1.0 - pow(pressure / 1013.25, 0.1903));
}

float IMUManager::getRoll() { return roll; }
float IMUManager::getPitch() { return pitch; }
float IMUManager::getYaw() { return yaw; }
float IMUManager::getAltitude() { return altitude; }