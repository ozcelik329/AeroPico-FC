#ifndef MATH_UTILS_H
#define MATH_UTILS_H

#include <Arduino.h>

// Özgün Öneri: Quake III Fast Inverse Square Root (A4/B3 Performans Fix)
inline float fast_invSqrt(float x) {
    if (x <= 0.0f) return 0.0f; // Hata Raporu Fix: Sıfır kontrolü
    float xhalf = 0.5f * x;
    int i = *(int*)&x;
    i = 0x5f3759df - (i >> 1);
    x = *(float*)&i;
    x = x * (1.5f - xhalf * x * x);
    return x;
}

inline float fast_clamp(float val, float min, float max) {
    if (val < min) return min;
    if (val > max) return max;
    return val;
}

// Bit Manipülasyonu ile Deadband (abs() yerine hızlı XOR)
inline int applyDeadband(int val, int neutral, int deadband) {
    int diff = val - neutral;
    unsigned int abs_diff = (diff ^ (diff >> 31)) - (diff >> 31);
    if (abs_diff < (unsigned int)deadband) return neutral;
    return val;
}

#endif