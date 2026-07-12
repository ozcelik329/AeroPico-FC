#include "BatteryMonitor.h"

void BatteryMonitor::init(VoltageProvider provider, float minVoltage, float maxVoltage, float brownoutVoltage) {
    _provider = provider;
    _minVoltage = minVoltage;
    _maxVoltage = maxVoltage;
    _brownoutVoltage = brownoutVoltage;
    _filteredVoltage = 0.0f;
    _filterInitialized = false;
    _lowLatched = false;
    _brownoutLatched = false;
    _lowSamples = 0;
    _unavailableSamples = 0;
}

BatteryStatus BatteryMonitor::evaluate() {
    if (!_provider) {
        return {false, false, false, false, 0.0f, "Battery monitor not configured"};
    }

    float voltage = 0.0f;
    if (!_provider(voltage)) {
        _unavailableSamples = _unavailableSamples < 255 ? (uint8_t)(_unavailableSamples + 1u) : _unavailableSamples;
        return {true, false, false, false, 0.0f, "Battery voltage unavailable"};
    }
    _unavailableSamples = 0;

    constexpr float FILTER_ALPHA = 0.25f;
    constexpr float RECOVERY_HYSTERESIS_V = 0.30f;
    constexpr float BROWNOUT_RECOVERY_HYSTERESIS_V = 0.50f;
    constexpr uint8_t LOW_DEBOUNCE_SAMPLES = 2;
    if (!_filterInitialized) {
        _filteredVoltage = voltage;
        _filterInitialized = true;
    } else {
        _filteredVoltage += FILTER_ALPHA * (voltage - _filteredVoltage);
    }
    voltage = _filteredVoltage;

    if (_brownoutVoltage > 0.0f &&
        (voltage < _brownoutVoltage ||
         (_brownoutLatched && voltage < _brownoutVoltage + BROWNOUT_RECOVERY_HYSTERESIS_V))) {
        _lowLatched = true;
        _brownoutLatched = true;
        return {true, false, true, true, voltage, "Battery brownout risk"};
    }
    _brownoutLatched = false;

    const bool lowNow = voltage < _minVoltage ||
        (_lowLatched && voltage < _minVoltage + RECOVERY_HYSTERESIS_V);
    if (lowNow) {
        _lowSamples = _lowSamples < 255 ? (uint8_t)(_lowSamples + 1u) : _lowSamples;
    } else {
        _lowSamples = 0;
    }

    if (lowNow && (_lowLatched || _lowSamples >= LOW_DEBOUNCE_SAMPLES)) {
        _lowLatched = true;
        return {true, false, false, true, voltage, "Battery voltage low"};
    }
    _lowLatched = false;

    if (voltage > _maxVoltage) {
        return {true, false, false, false, voltage, "Battery voltage out of range"};
    }

    return {true, true, false, false, voltage, ""};
}
