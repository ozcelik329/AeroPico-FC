#ifndef BATTERY_MONITOR_H
#define BATTERY_MONITOR_H

#include <Arduino.h>

struct BatteryStatus {
    bool configured;
    bool healthy;
    bool brownout;
    float voltage;
    const char* reason;
};

class BatteryMonitor {
  public:
    using VoltageProvider = bool (*)(float& voltage);

    void init(VoltageProvider provider = nullptr,
              float minVoltage = 0.0f,
              float maxVoltage = 100.0f,
              float brownoutVoltage = 0.0f);
    BatteryStatus evaluate() const;

  private:
    VoltageProvider _provider = nullptr;
    float _minVoltage = 0.0f;
    float _maxVoltage = 100.0f;
    float _brownoutVoltage = 0.0f;
};

#endif
