#ifndef BUZZER_FEEDBACK_H
#define BUZZER_FEEDBACK_H

#include <Arduino.h>

class BuzzerFeedback {
  public:
    void init(uint8_t pin);
    void bootChirp();

  private:
    uint8_t _pin = 255;
    bool _enabled = false;
};

#endif
