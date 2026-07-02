#ifndef ARDUINO_STUB_H
#define ARDUINO_STUB_H

#include <stdint.h>
#include <stdio.h>
#include <algorithm>

using byte = uint8_t;

#ifndef PI
#define PI 3.14159265358979323846f
#endif

#ifndef DEG_TO_RAD
#define DEG_TO_RAD 0.017453292519943295769f
#endif

template<typename T>
T constrain(T val, T lo, T hi) {
    return std::max(lo, std::min(val, hi));
}

inline unsigned long millis() { return 0; }
inline unsigned long micros() { return 0; }
inline void delay(unsigned long) {}

struct {
    void begin(int) {}
    void println(const char*) {}
    void println(const char*) const {}
    void println(int) {}
    void println(unsigned long) {}
    void printf(const char*, ...) {}
} Serial;

#define __not_in_flash_func(x) x
#define __dmb() do {} while (0)

#endif
