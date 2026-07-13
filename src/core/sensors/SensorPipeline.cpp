#include "SensorPipeline.h"

void SensorPipeline::init(IImuDriver* imuDriver, float fusionBeta) {
    _imu = imuDriver;
    if (_imu) {
        _imu->init();
    }
    _fusion.init(fusionBeta);
    _baroAltitude.init();
    _verticalKalman.init();
    _fallbackEstimator.init();
    _state = {};
    _state.sensorHealth = SensorHealth::WarmingUp;
    _state.estimatorHealth = SensorHealth::WarmingUp;
    _state.valid = false;
    _state.estimatorValid = false;
}

VehicleState SensorPipeline::update() {
    if (_imu) {
        _imu->update();
    }

    SensorBuffer buffer = _imu ? _imu->getLatest() : SensorBuffer{};
    updateFusion(buffer);

    _state.rollDeg = _fusion.getRoll();
    _state.pitchDeg = _fusion.getPitch();
    _state.yawDeg = _fusion.getYaw();
    _state.gyroX = buffer.gx;
    _state.gyroY = buffer.gy;
    _state.gyroZ = buffer.gz;
    _state.sensorHealth = buffer.health;
    _state.sensorQualityScore = buffer.qualityScore;
    _state.sensorAgeUs = buffer.sampleAgeUs;
    _state.timestampUs = micros();
    _state.valid = buffer.valid && buffer.health == SensorHealth::Ok;

    float baroAltitudeM = 0.0f;
    const bool baroAltitudeValid = _baroAltitude.update(buffer.pressureHpa, buffer.baroValid, baroAltitudeM);
    const EstimatorInput estimatorInput = buildEstimatorInput();
    EstimatedState estimated = _verticalKalman.update(estimatorInput, baroAltitudeM, baroAltitudeValid);
    EstimatedState fallback = _fallbackEstimator.update(estimatorInput, baroAltitudeM, baroAltitudeValid);
    if (!estimated.valid && fallback.valid) {
        estimated = fallback;
    }
    _state.altitudeM = estimated.altitudeM;
    _state.verticalSpeedMps = estimated.verticalSpeedMps;
    _state.estimatorHealth = estimated.health;
    _state.estimatorValid = estimated.valid;
    return _state;
}

VehicleState SensorPipeline::getState() const {
    return _state;
}

void SensorPipeline::updateFusion(const SensorBuffer& buffer) {
    if (!buffer.valid || buffer.health != SensorHealth::Ok) {
        _lastVerticalAccelMps2 = 0.0f;
        return;
    }

    _fusion.setTemperature(buffer.tempC);
    _fusion.setGyroTempCoeff(buffer.gyroTempCoeff);

    float gx = buffer.gx * DEG_TO_RAD;
    float gy = buffer.gy * DEG_TO_RAD;
    float gz = buffer.gz * DEG_TO_RAD;

    #ifdef USE_GY87
        const float magNormSq = buffer.mx * buffer.mx + buffer.my * buffer.my + buffer.mz * buffer.mz;
        if (magNormSq > 1.0e-6f) {
            _fusion.update(gx, gy, gz, buffer.ax, buffer.ay, buffer.az, buffer.mx, buffer.my, buffer.mz);
        } else {
            _fusion.updateIMU(gx, gy, gz, buffer.ax, buffer.ay, buffer.az);
        }
    #else
        _fusion.updateIMU(gx, gy, gz, buffer.ax, buffer.ay, buffer.az);
    #endif
    _lastVerticalAccelMps2 = _fusion.getVerticalAccelerationMps2(buffer.ax, buffer.ay, buffer.az);
}

EstimatorInput SensorPipeline::buildEstimatorInput() const {
    EstimatorInput input = {};
    input.rollDeg = _state.rollDeg;
    input.pitchDeg = _state.pitchDeg;
    input.yawDeg = _state.yawDeg;
    input.verticalAccelMps2 = _lastVerticalAccelMps2;
    input.sensorHealth = _state.sensorHealth;
    input.timestampUs = _state.timestampUs;
    input.failsafe = false;
    return input;
}
