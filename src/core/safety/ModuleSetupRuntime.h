#ifndef MODULE_SETUP_RUNTIME_H
#define MODULE_SETUP_RUNTIME_H

#include <Arduino.h>
#include "board/Config.h"
#include "../../types.h"

struct ModuleServoPinConfig {
    uint8_t aileron;
    uint8_t elevator;
    uint8_t rudder;
    uint8_t throttle;
};

struct ModuleSetupSnapshot {
    bool baroEnabled = true;
    bool magEnabled = true;
    bool gpsEnabled = false;
    bool batteryEnabled = false;
    bool rcEnabled = true;
    uint8_t imuType = 1;
    uint8_t baroType = 1;
    uint8_t magType = 0;
    uint8_t gpsType = 1;
    uint8_t rcType = 1;
    uint8_t batteryType = 0;
    uint8_t i2cSda = PIN_SDA;
    uint8_t i2cScl = PIN_SCL;
    uint8_t buzzerPin = PIN_BUZZER;
    ModuleServoPinConfig servoPins = {};
    bool bootServoPinConfigValid = true;
};

struct ModuleSetupEvaluation {
    bool passed;
    bool batteryRequired;
    bool rcRequired;
    uint16_t enabledCapabilityMask;
    char reason[96];
};

class ModuleSetupRuntime {
  public:
    void update(const ModuleSetupSnapshot& snapshot);
    const ModuleSetupSnapshot& snapshot() const { return _snapshot; }

    bool batteryRequired() const;
    bool rcRequired() const;
    uint16_t enabledSensorMask(uint16_t detectedMask) const;
    ModuleSetupEvaluation evaluate(uint16_t detectedMask) const;

    static bool validateServoPinSetup(const ModuleServoPinConfig& pins);
    static bool validateServoPinSetup(const ModuleServoPinConfig& pins,
                                      uint8_t i2cSda,
                                      uint8_t i2cScl,
                                      uint8_t buzzerPin);
    static bool validateI2cPinSetup(uint8_t sdaPin, uint8_t sclPin);
    static bool validateBuzzerPinSetup(uint8_t pin);

  private:
    static bool isReservedServoSetupPin(uint8_t pin, uint8_t i2cSda, uint8_t i2cScl, uint8_t buzzerPin);
    static bool isValidServoSetupPin(uint8_t pin);
    static bool isValidServoSetupPin(uint8_t pin, uint8_t i2cSda, uint8_t i2cScl, uint8_t buzzerPin);
    static bool isSupportedImuType(uint8_t type);
    static bool isSupportedBaroType(uint8_t type);
    static bool isSupportedMagType(uint8_t type);
    static bool isSupportedGpsType(uint8_t type);
    static bool isSupportedRcType(uint8_t type);
    static bool isSupportedBatteryType(uint8_t type);

    ModuleSetupSnapshot _snapshot = {};
};

#endif
