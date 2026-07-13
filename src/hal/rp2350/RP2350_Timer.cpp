#include "RP2350_Timer.h"

uint32_t RP2350Timer::micros() const {
    return ::micros();
}

uint32_t RP2350Timer::millis() const {
    return ::millis();
}

void RP2350Timer::delayMs(uint32_t ms) {
    ::delay(ms);
}
