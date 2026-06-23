#include "Sensors.h"

void SensorManager::init() {
    mutex_init(&_mutex);

    _buf[0] = {};
    _buf[1] = {};

    Wire.begin();
    Wire.setClock(400000);

    if (!mpu.begin()) {
        Serial.println("[SENSOR] MPU6050 bulunamadi!");
    } else {
        mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
        mpu.setGyroRange(MPU6050_RANGE_500_DEG);
        mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
        Serial.println("[SENSOR] MPU6050 hazir.");
    }

    #ifdef USE_GY87
        hasMag = mag.begin();
        if (!hasMag) Serial.println("[SENSOR] HMC5883L bulunamadi!");
        else         Serial.println("[SENSOR] HMC5883L hazir.");

        hasBaro = bmp.begin();
        if (!hasBaro) Serial.println("[SENSOR] BMP085 bulunamadi!");
        else          Serial.println("[SENSOR] BMP085 hazir.");
    #endif
}

void SensorManager::update() {
    // Yazılacak buffer'ı belirle (aktif olmayan)
    uint8_t writeIdx = 1 - _writeIdx;
    SensorBuffer& buf = _buf[writeIdx];

    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    buf.ax = a.acceleration.x;
    buf.ay = a.acceleration.y;
    buf.az = a.acceleration.z;
    buf.gx = g.gyro.x;
    buf.gy = g.gyro.y;
    buf.gz = g.gyro.z;
    buf.timestamp = micros();
    buf.valid = true;

    #ifdef USE_GY87
        if (hasMag) {
            sensors_event_t mag_event;
            mag.getEvent(&mag_event);
            buf.mx = mag_event.magnetic.x;
            buf.my = mag_event.magnetic.y;
            buf.mz = mag_event.magnetic.z;
        } else {
            buf.mx = buf.my = buf.mz = 0.0f;
        }

        if (hasBaro) {
            sensors_event_t bmp_event;
            bmp.getEvent(&bmp_event);
            buf.pressure = bmp_event.pressure;
        } else {
            buf.pressure = 1013.25f;
        }
    #endif

    // Atomik index değişimi — okuyucu her zaman tam veriyi görür
    mutex_enter_blocking(&_mutex);
    _writeIdx = writeIdx;
    mutex_exit(&_mutex);
}

SensorBuffer SensorManager::getLatest() {
    mutex_enter_blocking(&_mutex);
    SensorBuffer copy = _buf[_writeIdx];
    mutex_exit(&_mutex);
    return copy;
}