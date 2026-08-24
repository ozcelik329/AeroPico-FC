#include "BuzzerFeedback.h"

namespace {
constexpr BuzzerFeedback::ToneStep ARM_MELODY[] = {
    {523, 120},
    {0, 20},
    {659, 120},
    {0, 20},
    {784, 120},
    {0, 20},
    {1047, 200},
};

constexpr BuzzerFeedback::ToneStep DISARM_MELODY[] = {
    {1047, 120},
    {0, 20},
    {784, 120},
    {0, 20},
    {659, 120},
    {0, 20},
    {440, 250},
};
}  // namespace

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

void BuzzerFeedback::playArmMelody() {
    startSequence(ARM_MELODY, (uint8_t)(sizeof(ARM_MELODY) / sizeof(ARM_MELODY[0])));
}

void BuzzerFeedback::playDisarmMelody() {
    startSequence(DISARM_MELODY, (uint8_t)(sizeof(DISARM_MELODY) / sizeof(DISARM_MELODY[0])));
}

void BuzzerFeedback::update(uint32_t nowMs) {
    if (!_enabled || !_sequence) return;
    if (nowMs >= _stepUntilMs) {
        advance(nowMs);
    }
}

void BuzzerFeedback::startSequence(const ToneStep* sequence, uint8_t length) {
    if (!_enabled || !sequence || length == 0) return;
    stopTone();
    _sequence = sequence;
    _sequenceLength = length;
    _sequenceIndex = 0;
    _stepUntilMs = 0;
    advance(millis());
}

void BuzzerFeedback::stopTone() {
    if (!_enabled) return;
    if (_toneActive) {
        noTone(_pin);
        _toneActive = false;
    }
    digitalWrite(_pin, LOW);
}

void BuzzerFeedback::advance(uint32_t nowMs) {
    stopTone();
    if (!_sequence || _sequenceIndex >= _sequenceLength) {
        _sequence = nullptr;
        _sequenceLength = 0;
        _sequenceIndex = 0;
        _stepUntilMs = 0;
        return;
    }

    const ToneStep step = _sequence[_sequenceIndex++];
    if (step.frequencyHz > 0) {
        tone(_pin, step.frequencyHz);
        _toneActive = true;
    }
    _stepUntilMs = nowMs + step.durationMs;
}
