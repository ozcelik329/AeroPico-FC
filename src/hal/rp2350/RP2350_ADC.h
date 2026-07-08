#ifndef RP2350_ADC_H
#define RP2350_ADC_H

#include "../HAL_ADC.h"

class RP2350ADC final : public IHALADC {
  public:
    void init(uint8_t pin, uint8_t channel) override;
    bool readRaw12(uint8_t channel, uint16_t& raw) override;
    bool readVoltage(uint8_t channel, float scale, float& voltage) override;

  private:
    static constexpr float ADC_REF_V = 3.3f;
    static constexpr float ADC_COUNTS = 4095.0f;
    static constexpr uint8_t OVERSAMPLE_COUNT = 4;
    bool _initialized = false;
};

#endif
