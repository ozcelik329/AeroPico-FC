#include "BatteryMonitor.h"

void BatteryMonitor::init(VoltageProvider provider, float minVoltage, float maxVoltage, float brownoutVoltage) {
    _provider = provider;
    _minVoltage = minVoltage;
    _maxVoltage = maxVoltage;
    _brownoutVoltage = brownoutVoltage;
}

BatteryStatus BatteryMonitor::evaluate() const {
    if (!_provider) {
        return {false, false, false, 0.0f, "Battery monitor not configured"};
    }

    float voltage = 0.0f;
    if (!_provider(voltage)) {
        return {true, false, false, 0.0f, "Battery voltage unavailable"};
    }

    if (_brownoutVoltage > 0.0f && voltage < _brownoutVoltage) {
        return {true, false, true, voltage, "Battery brownout risk"};
    }

    if (voltage < _minVoltage) {
        return {true, false, false, voltage, "Battery voltage low"};
    }

    if (voltage > _maxVoltage) {
        return {true, false, false, voltage, "Battery voltage out of range"};
    }

    return {true, true, false, voltage, ""};
}
