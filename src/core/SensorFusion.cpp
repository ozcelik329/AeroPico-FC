#include "SensorFusion.h"
#include <math.h>
#include <Arduino.h>

SensorFusion::SensorFusion() : q0(1.0f), q1(0.0f), q2(0.0f), q3(0.0f), roll(0.0f), pitch(0.0f), yaw(0.0f), beta(0.1f), lastUpdate(0) {}

void SensorFusion::init(float beta) {
    this->beta = beta;
    q0 = 1.0f;
    q1 = q2 = q3 = 0.0f;
    roll = pitch = yaw = 0.0f;
    lastUpdate = micros();
}

void SensorFusion::setTemperature(float tempC) {
    _tempC = tempC;
}

void __not_in_flash_func(SensorFusion::update)(float gx, float gy, float gz,
                                                float ax, float ay, float az,
                                                float mx, float my, float mz) {
    float magNorm = sqrtf(mx * mx + my * my + mz * mz);
    if (magNorm < 1e-6f) {
        updateIMU(gx, gy, gz, ax, ay, az);
        return;
    }

    float tempOffset = ((_tempC - 25.0f) * _gyroTempCoeff) * DEG_TO_RAD;
    gx -= tempOffset;
    gy -= tempOffset;
    gz -= tempOffset;

    uint32_t now = micros();
    float dt = (now - lastUpdate) / 1000000.0f;
    if (dt <= 0.0f) dt = 0.001f;
    lastUpdate = now;

    float q0q0 = q0 * q0;
    float q1q1 = q1 * q1;
    float q2q2 = q2 * q2;
    float q3q3 = q3 * q3;

    float norm = sqrtf(ax * ax + ay * ay + az * az);
    if (norm < 1e-6f) return;
    ax /= norm; ay /= norm; az /= norm;

    norm = magNorm;
    mx /= norm; my /= norm; mz /= norm;

    float _2q0mx = 2.0f * q0 * mx;
    float _2q0my = 2.0f * q0 * my;
    float _2q0mz = 2.0f * q0 * mz;
    float _2q1mx = 2.0f * q1 * mx;
    float _2q0   = 2.0f * q0;
    float _2q1   = 2.0f * q1;
    float _2q2   = 2.0f * q2;
    float _2q3   = 2.0f * q3;
    float _2q0q2 = 2.0f * q0 * q2;
    float _2q2q3 = 2.0f * q2 * q3;
    float q0q1   = q0 * q1;
    float q0q2   = q0 * q2;
    float q0q3   = q0 * q3;
    float q1q2   = q1 * q2;
    float q1q3   = q1 * q3;
    float q2q3   = q2 * q3;

    float hx = mx * q0q0 - _2q0my * q3 + _2q0mz * q2 + mx * q1q1 + _2q1 * my * q2 + _2q1 * mz * q3 - mx * q2q2 - mx * q3q3;
    float hy = _2q0mx * q3 + my * q0q0 - _2q0mz * q1 + _2q1mx * q2 - my * q1q1 + my * q2q2 + _2q2 * mz * q3 - my * q3q3;
    float _2bx = sqrtf(hx * hx + hy * hy);
    float _2bz = -_2q0mx * q2 + _2q0my * q1 + mz * q0q0 + _2q1mx * q3 - mz * q1q1 + _2q2 * my * q3 - mz * q2q2 + mz * q3q3;
    float _4bx = 2.0f * _2bx;
    float _4bz = 2.0f * _2bz;

    float f1 = 2.0f * (q1q3 - q0q2) - ax;
    float f2 = 2.0f * (q0q1 + q2q3) - ay;
    float f3 = 2.0f * (0.5f - q1q1 - q2q2) - az;
    float f4 = _2bx * (0.5f - q2q2 - q3q3) + _2bz * (q1q3 - q0q2) - mx;
    float f5 = _2bx * (q1q2 - q0q3) + _2bz * (q0q1 + q2q3) - my;
    float f6 = _2bx * (q0q2 + q1q3) + _2bz * (0.5f - q1q1 - q2q2) - mz;

    float J_11or24 = 2.0f * q2;
    float J_12or23 = 2.0f * q3;
    float J_13or22 = 2.0f * q0;
    float J_14or21 = 2.0f * q1;
    float J_32     = 2.0f * J_14or21;
    float J_33     = 2.0f * J_11or24;
    float J_41     = 2.0f * _2bz * q2;
    float J_42     = 2.0f * _2bz * q3;
    float J_43     = 4.0f * _2bx * q2 + 2.0f * _2bz * q0;
    float J_44     = 4.0f * _2bx * q3 - 2.0f * _2bz * q1;
    float J_51     = 2.0f * _2bx * q3;
    float J_52     = 2.0f * _2bx * q2 + 2.0f * _2bz * q1;
    float J_53     = 2.0f * _2bx * q1 + 2.0f * _2bz * q0;
    float J_54     = 2.0f * _2bx * q0 - 2.0f * _2bz * q2;
    float J_61     = 2.0f * _2bx * q2;
    float J_62     = 2.0f * _2bx * q3 - 4.0f * _2bz * q1;
    float J_63     = 2.0f * _2bx * q0 - 4.0f * _2bz * q2;
    float J_64     = 2.0f * _2bx * q1;

    float s0 = -J_14or21 * f2 + J_11or24 * f1 - J_41 * f4 - J_51 * f5 - J_61 * f6;
    float s1 =  J_13or22 * f1 + J_12or23 * f2 - J_42 * f4 + J_52 * f5 + J_62 * f6;
    float s2 =  J_13or22 * f2 - J_33     * f3 - J_43 * f4 + J_53 * f5 + J_63 * f6;
    float s3 =  J_12or23 * f1 + J_11or24 * f2 - J_44 * f4 - J_54 * f5 - J_64 * f6;

    norm = sqrtf(s0 * s0 + s1 * s1 + s2 * s2 + s3 * s3);
    if (norm < 1e-6f) return;
    s0 /= norm; s1 /= norm; s2 /= norm; s3 /= norm;

    float qDot1 = 0.5f * (-q1 * gx - q2 * gy - q3 * gz) - beta * s0;
    float qDot2 = 0.5f * ( q0 * gx + q2 * gz - q3 * gy) - beta * s1;
    float qDot3 = 0.5f * ( q0 * gy - q1 * gz + q3 * gx) - beta * s2;
    float qDot4 = 0.5f * ( q0 * gz + q1 * gy - q2 * gx) - beta * s3;

    q0 += qDot1 * dt; q1 += qDot2 * dt;
    q2 += qDot3 * dt; q3 += qDot4 * dt;

    norm = sqrtf(q0 * q0 + q1 * q1 + q2 * q2 + q3 * q3);
    q0 /= norm; q1 /= norm; q2 /= norm; q3 /= norm;

    computeAngles();
}

void __not_in_flash_func(SensorFusion::updateIMU)(float gx, float gy, float gz,
                                                   float ax, float ay, float az) {
    float tempOffset = ((_tempC - 25.0f) * _gyroTempCoeff) * DEG_TO_RAD;
    gx -= tempOffset;
    gy -= tempOffset;
    gz -= tempOffset;

    uint32_t now = micros();
    float dt = (now - lastUpdate) / 1000000.0f;
    if (dt <= 0.0f) dt = 0.001f;
    lastUpdate = now;

    float norm = sqrtf(ax * ax + ay * ay + az * az);
    if (norm < 1e-6f) return;
    ax /= norm; ay /= norm; az /= norm;

    // Mahony-stili ivme tabanlı düzeltme (6-DOF yolu)
    float vx = 2.0f * (q1 * q3 - q0 * q2);
    float vy = 2.0f * (q0 * q1 + q2 * q3);
    float vz = q0 * q0 - q1 * q1 - q2 * q2 + q3 * q3;
    float ex = (ay * vz - az * vy);
    float ey = (az * vx - ax * vz);
    float ez = (ax * vy - ay * vx);
    gx += beta * ex;
    gy += beta * ey;
    gz += beta * ez;

    float qDot1 = 0.5f * (-q1 * gx - q2 * gy - q3 * gz);
    float qDot2 = 0.5f * ( q0 * gx + q2 * gz - q3 * gy);
    float qDot3 = 0.5f * ( q0 * gy - q1 * gz + q3 * gx);
    float qDot4 = 0.5f * ( q0 * gz + q1 * gy - q2 * gx);

    q0 += qDot1 * dt; q1 += qDot2 * dt;
    q2 += qDot3 * dt; q3 += qDot4 * dt;

    norm = sqrtf(q0 * q0 + q1 * q1 + q2 * q2 + q3 * q3);
    q0 /= norm; q1 /= norm; q2 /= norm; q3 /= norm;

    computeAngles();
}

float SensorFusion::getRoll()  const { return roll; }
float SensorFusion::getPitch() const { return pitch; }
float SensorFusion::getYaw()   const { return yaw; }

#ifdef UNIT_TEST
void SensorFusion::setQuaternionForTest(float w, float x, float y, float z) {
    q0 = w;
    q1 = x;
    q2 = y;
    q3 = z;
    computeAngles();
}
#endif

void __not_in_flash_func(SensorFusion::computeAngles)() {
    roll  = atan2f(2.0f * (q0 * q1 + q2 * q3), 1.0f - 2.0f * (q1 * q1 + q2 * q2)) * 180.0f / PI;
    pitch = asinf(constrain(2.0f * (q0 * q2 - q3 * q1), -1.0f, 1.0f)) * 180.0f / PI;
    yaw   = atan2f(2.0f * (q0 * q3 + q1 * q2), 1.0f - 2.0f * (q2 * q2 + q3 * q3)) * 180.0f / PI;
}
