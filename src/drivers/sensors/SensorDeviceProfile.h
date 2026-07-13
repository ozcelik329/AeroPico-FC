#ifndef SENSOR_DEVICE_PROFILE_H
#define SENSOR_DEVICE_PROFILE_H

#include <Arduino.h>

struct ImuDeviceProfile {
    uint8_t address;
    uint8_t powerReg;
    uint8_t powerWakeValue;
    uint8_t accelReg;
    uint8_t rawSampleLen;
    uint8_t gyroConfigReg;
    uint8_t gyroConfigValue;
    uint8_t accelConfigReg;
    uint8_t accelConfigValue;
    uint8_t dlpfReg;
    uint8_t dlpfValue;
    uint8_t whoAmIReg;
    uint8_t whoAmIValue;
};

struct MagDeviceProfile {
    uint8_t address;
    uint8_t configAReg;
    uint8_t configAValue;
    uint8_t configBReg;
    uint8_t configBValue;
    uint8_t modeReg;
    uint8_t modeValue;
    uint8_t dataReg;
    uint8_t sampleLen;
};

struct BaroDeviceProfile {
    uint8_t address;
    uint8_t calibrationReg;
    uint8_t calibrationLen;
    uint8_t controlReg;
    uint8_t resultReg;
    uint8_t temperatureCommand;
    uint8_t pressureCommand;
    uint8_t pressureOversampling;
    uint32_t temperatureWaitUs;
    uint32_t pressureWaitUs;
};

#endif
