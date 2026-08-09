#include "BuzzerFeedback.h"

void BuzzerFeedback::init(uint8_t pin) {
    if (pin > 28) {
        _enabled = false;
        return;
    }

    _pin = pin;
    _enabled = true;
    pinMode(_pin, OUTPUT);
    digitalWrite(_pin, LOW);
}

void BuzzerFeedback::bootChirp() {
    if (!_enabled) return;

    digitalWrite(_pin, HIGH);
    delay(25);
    digitalWrite(_pin, LOW);
}
