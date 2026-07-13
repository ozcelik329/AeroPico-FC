#include "RP2350_ADC.h"

#include "hardware/adc.h"

void RP2350ADC::init(uint8_t pin, uint8_t channel) {
    adc_init();
    adc_gpio_init(pin);
    adc_select_input(channel);
    _initialized = true;
}

bool RP2350ADC::readRaw12(uint8_t channel, uint16_t& raw) {
    if (!_initialized) {
        raw = 0;
        return false;
    }

    adc_select_input(channel);

    uint32_t sum = 0;
    for (uint8_t i = 0; i < OVERSAMPLE_COUNT; i++) {
        sum += adc_read();
    }

    raw = (uint16_t)(sum >> 2);
    return true;
}

bool RP2350ADC::readVoltage(uint8_t channel, float scale, float& voltage) {
    uint16_t raw = 0;
    if (!readRaw12(channel, raw)) {
        voltage = 0.0f;
        return false;
    }

    voltage = ((float)raw * ADC_REF_V / ADC_COUNTS) * scale;
    return true;
}
