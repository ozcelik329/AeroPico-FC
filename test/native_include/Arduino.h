#ifndef ARDUINO_STUB_H
#define ARDUINO_STUB_H

#include <stdint.h>
#include <stdio.h>

using byte = uint8_t;

inline unsigned long millis() { return 0; }
inline unsigned long micros() { return 0; }

struct {
    void begin(int) {}
    void println(const char*) {}
    void println(const char*) const {}
    void println(int) {}
    void println(unsigned long) {}
} Serial;

#define __not_in_flash_func(x) x

#endif
