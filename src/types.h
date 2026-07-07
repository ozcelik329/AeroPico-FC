#ifndef TYPES_H
#define TYPES_H

#include <Arduino.h>
#include "config.h"
// Ortak veri tipleri
enum class SensorHealth : uint8_t {
    Invalid = 0,
    WarmingUp,
    Ok,
    Stale,
    Timeout
};

enum class SensorFaultCode : uint8_t {
    None = 0,
    I2cWhoamiWriteFailed,
    I2cWhoamiReadFailed,
    WhoamiMismatch,
    I2cRawWriteFailed,
    I2cRawReadFailed,
    DmaChannelClaimFailed,
    DmaTransferTimeout,
    AuxI2cWriteFailed,
    AuxDmaTransferTimeout,
    MagReadFailed,
    BaroReadFailed
};

struct SensorBuffer {
	float ax, ay, az;
	float gx, gy, gz;
	float tempC;
#ifdef USE_GY87
	float mx, my, mz;
	float pressure;
#endif
	uint32_t timestamp;
	bool valid;
	SensorHealth health;
};

struct ImuCalibration {
    float gyroBiasX;
    float gyroBiasY;
    float gyroBiasZ;
    float accelBiasX;
    float accelBiasY;
    float accelBiasZ;
    bool valid;
};

struct MagCalibration {
    float hardIronX;
    float hardIronY;
    float hardIronZ;
    bool valid;
};

struct FlightData {
    float roll, pitch, yaw;
    float gyroX, gyroY, gyroZ;
    uint16_t aileron, elevator, throttle, rudder;
    bool failsafe;
    SensorHealth sensorHealth;
    uint32_t timestamp;
};

struct RcInputState {
    uint16_t aileron;
    uint16_t elevator;
    uint16_t throttle;
    uint16_t rudder;
    bool failsafe;
    bool overrideActive;
    uint32_t timestampMs;
};

struct VehicleState {
    float rollDeg;
    float pitchDeg;
    float yawDeg;
    float gyroX;
    float gyroY;
    float gyroZ;
    SensorHealth sensorHealth;
    uint32_t timestampUs;
    bool valid;
};

struct TimingBudgetStatus {
    uint32_t consumeUs;
    uint32_t pidUs;
    uint32_t mixerUs;
    uint32_t totalUs;
    bool consumeExceeded;
    bool pidExceeded;
    bool mixerExceeded;
    bool totalExceeded;
};

struct EstimatedState {
    float rollDeg;
    float pitchDeg;
    float yawDeg;
    float altitudeM;
    float verticalSpeedMps;
    SensorHealth health;
    uint32_t timestamp;
    bool valid;
};

#endif
