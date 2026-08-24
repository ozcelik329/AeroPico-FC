#ifndef BUZZER_FEEDBACK_H
#define BUZZER_FEEDBACK_H

#include <Arduino.h>

class BuzzerFeedback {
  public:
    struct ToneStep {
        uint16_t frequencyHz;
        uint16_t durationMs;
    };

    void init(uint8_t pin);
    void bootChirp();
    void playArmMelody();
    void playDisarmMelody();
    void update(uint32_t nowMs);

  private:
    void startSequence(const ToneStep* sequence, uint8_t length);
    void stopTone();
    void advance(uint32_t nowMs);

    uint8_t _pin = 255;
    bool _enabled = false;
    const ToneStep* _sequence = nullptr;
    uint8_t _sequenceLength = 0;
    uint8_t _sequenceIndex = 0;
    uint32_t _stepUntilMs = 0;
    bool _toneActive = false;
};

#endif
