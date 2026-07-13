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

static unsigned long __arduino_mock_millis = 0;
static unsigned long __arduino_mock_micros = 0;
inline void setMockMillis(unsigned long value) { __arduino_mock_millis = value; }
inline void setMockMicros(unsigned long value) { __arduino_mock_micros = value; }
inline unsigned long millis() { return __arduino_mock_millis; }
inline unsigned long micros() { return __arduino_mock_micros; }
inline void delay(unsigned long) {}

struct {
    void begin(int) {}
    void println(const char*) {}
    void println(const char*) const {}
    void println(int) {}
    void println(unsigned long) {}
    void printf(const char*, ...) {}
    size_t write(const uint8_t*, size_t len) { bytesWritten += len; return len; }
    size_t write(uint8_t) { bytesWritten++; return 1; }
    int available() { return 0; }
    int read() { return -1; }
    size_t bytesWritten = 0;
} Serial;

#define __not_in_flash_func(x) x
#define __dmb() do {} while (0)

#endif
