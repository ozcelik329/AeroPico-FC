#include "EKF.h"

void EKF::init() {
    q[0] = 1.0f; q[1] = 0.0f; q[2] = 0.0f; q[3] = 0.0f;
    _bias[0] = _bias[1] = _bias[2] = 0.0f;
}

// SRAM üzerinde çalışır (Risk 1 FIX - Flash Latency Önleme)
void __not_in_flash_func(EKF::predict)(float gx, float gy, float gz, float dt) {
    gx -= _bias[0]; gy -= _bias[1]; gz -= _bias[2];
    
    float q0 = q[0], q1 = q[1], q2 = q[2], q3 = q[3];
    q[0] += (-q1 * gx - q2 * gy - q3 * gz) * (0.5f * dt);
    q[1] += (q0 * gx + q2 * gz - q3 * gy) * (0.5f * dt);
    q[2] += (q0 * gy - q1 * gz + q3 * gx) * (0.5f * dt);
    q[3] += (q0 * gz + q1 * gy - q2 * gx) * (0.5f * dt);

    float n = fast_invSqrt(q[0]*q[0] + q[1]*q[1] + q[2]*q[2] + q[3]*q[3]);
    q[0] *= n; q[1] *= n; q[2] *= n; q[3] *= n;
    _computeEuler();
}

void __not_in_flash_func(EKF::updateIMU)(float ax, float ay, float az) {
    float n = fast_invSqrt(ax*ax + ay*ay + az*az);
    if (n == 0) return;
    ax *= n; ay *= n; az *= n;

    // Tahmini yerçekimi vektörü
    float vx = 2.0f * (q[1] * q[3] - q[0] * q[2]);
    float vy = 2.0f * (q[0] * q[1] + q[2] * q[3]);
    float vz = q[0] * q[0] - q[1] * q[1] - q[2] * q[2] + q[3] * q[3];

    // Hata vektörü (Innovation)
    float ex = (ay * vz - az * vy);
    float ey = (az * vx - ax * vz);
    float ez = (ax * vy - ay * vx);

    // Bias güncelleme (A3/B12)
    _bias[0] -= ex * 0.001f;
    _bias[1] -= ey * 0.001f;
    _bias[2] -= ez * 0.001f;
}

void EKF::_computeEuler() {
    roll = atan2(2.0f * (q[0] * q[1] + q[2] * q[3]), 1.0f - 2.0f * (q[1] * q[1] + q[2] * q[2])) * 57.295f;
    pitch = asin(fast_clamp(2.0f * (q[0] * q[2] - q[3] * q[1]), -1.0f, 1.0f)) * 57.295f;
    yaw = atan2(2.0f * (q[0] * q[3] + q[1] * q[2]), 1.0f - 2.0f * (q[2] * q[2] + q[3] * q[3])) * 57.295f;
}