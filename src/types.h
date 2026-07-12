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

enum SensorCapabilityBits : uint16_t {
    SENSOR_CAP_IMU  = 1u << 0,
    SENSOR_CAP_MAG  = 1u << 1,
    SENSOR_CAP_BARO = 1u << 2,
    SENSOR_CAP_GPS  = 1u << 3
};

struct SensorCapabilityStatus {
    uint16_t functionMask;
    bool imuAvailable;
    bool magAvailable;
    bool baroAvailable;
    bool gpsAvailable;
};

static inline bool hasSensorCapability(uint16_t mask, SensorCapabilityBits bit) {
    return (mask & (uint16_t)bit) != 0;
}

enum class ControlMode : uint8_t {
    Manual = 0,
    Stabilize = 1
};

struct SensorQuality {
    SensorHealth health;
    uint8_t score;
    uint32_t ageUs;
};

struct SensorBuffer {
	float ax, ay, az;
	float gx, gy, gz;
    float gyroTempCoeff;
	float tempC;
	float mx, my, mz;
	float pressureHpa;
	uint32_t timestamp;
    uint32_t sampleAgeUs;
    uint8_t qualityScore;
	bool valid;
    bool baroValid;
	SensorHealth health;
};

struct ImuCalibration {
    float gyroBiasX;
    float gyroBiasY;
    float gyroBiasZ;
    float accelBiasX;
    float accelBiasY;
    float accelBiasZ;
    float gyroTempCoeff;
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
    float altitudeM;
    float verticalSpeedMps;
    uint16_t aileron, elevator, throttle, rudder;
    bool failsafe;
    bool timingExceeded;
    bool batteryCritical;
    bool actuatorFault;
    ControlMode controlMode;
    SensorHealth sensorHealth;
    SensorHealth estimatorHealth;
    uint8_t sensorQualityScore;
    uint32_t sensorAgeUs;
    uint32_t timestamp;
    bool estimatorValid;
};

struct SensorState {
    float rollDeg;
    float pitchDeg;
    float yawDeg;
    float gyroX;
    float gyroY;
    float gyroZ;
    float altitudeM;
    float verticalSpeedMps;
    SensorHealth health;
    SensorHealth estimatorHealth;
    uint8_t sensorQualityScore;
    uint32_t sensorAgeUs;
    uint32_t timestampUs;
    bool valid;
    bool estimatorValid;
};

struct ActuatorState {
    uint16_t throttle;
    uint16_t aileron;
    uint16_t elevator;
    uint16_t rudder;
    bool outputsReady;
    bool failsafe;
};

struct NavigationState {
    float targetAltitudeM;
    bool navigationActive;
    bool altitudeHoldActive;
};

struct RcInputState {
    uint16_t aileron;
    uint16_t elevator;
    uint16_t throttle;
    uint16_t rudder;
    ControlMode controlMode;
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
    float altitudeM;
    float verticalSpeedMps;
    SensorHealth sensorHealth;
    SensorHealth estimatorHealth;
    uint8_t sensorQualityScore;
    uint32_t sensorAgeUs;
    uint32_t timestampUs;
    bool valid;
    bool estimatorValid;
};

struct TimingBudgetStatus {
    uint32_t consumeUs;
    uint32_t pidUs;
    uint32_t mixerUs;
    uint32_t totalUs;
    uint32_t consumeAvgUs;
    uint32_t pidAvgUs;
    uint32_t mixerAvgUs;
    uint32_t totalAvgUs;
    uint32_t consumeJitterUs;
    uint32_t pidJitterUs;
    uint32_t mixerJitterUs;
    uint32_t totalJitterUs;
    uint16_t consumeDeadlineMisses;
    uint16_t pidDeadlineMisses;
    uint16_t mixerDeadlineMisses;
    uint16_t totalDeadlineMisses;
    uint16_t totalLoadPermille;
    uint16_t windowSamples;
    bool consumeExceeded;
    bool pidExceeded;
    bool mixerExceeded;
    bool totalExceeded;
};

struct RuntimeHealthStatus {
    uint16_t sensorStackHighWaterWords;
    uint16_t flightStackHighWaterWords;
    uint16_t telemetryStackHighWaterWords;
    uint16_t eventQueueDrops;
    uint16_t blackboxDrops;
};

struct EstimatorInput {
    float rollDeg;
    float pitchDeg;
    float yawDeg;
    float verticalAccelMps2;
    SensorHealth sensorHealth;
    uint32_t timestampUs;
    bool failsafe;
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
