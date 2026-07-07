#include "BatteryMonitor.h"

void BatteryMonitor::init(VoltageProvider provider, float minVoltage, float maxVoltage) {
    _provider = provider;
    _minVoltage = minVoltage;
    _maxVoltage = maxVoltage;
}

BatteryStatus BatteryMonitor::evaluate() const {
    if (!_provider) {
        return {false, false, 0.0f, "Battery monitor not configured"};
    }

    float voltage = 0.0f;
    if (!_provider(voltage)) {
        return {true, false, 0.0f, "Battery voltage unavailable"};
    }

    if (voltage < _minVoltage) {
        return {true, false, voltage, "Battery voltage low"};
    }

    if (voltage > _maxVoltage) {
        return {true, false, voltage, "Battery voltage out of range"};
    }

    return {true, true, voltage, ""};
}
