#ifndef EKF_H
#define EKF_H

#include <Arduino.h>
#include "MathUtils.h"
#include "pico/platform.h"

class EKF {
public:
    void init();
    void predict(float gx, float gy, float gz, float dt);
    void updateIMU(float ax, float ay, float az);
    
    float roll, pitch, yaw;

private:
    float q[4];    // Quaternion: [w, x, y, z]
    float _bias[3]; // Gyro bias takibi (A3)
    void _computeEuler();
};

#endif