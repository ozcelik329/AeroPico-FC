#include "SensorPipeline.h"

void SensorPipeline::init(IImuDriver* imuDriver, float fusionBeta) {
    _imu = imuDriver;
    if (_imu) {
        _imu->init();
    }
    _fusion.init(fusionBeta);
    _baroAltitude.init();
    _ekfLite.init();
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
    EstimatedState estimated = _ekfLite.update(buildEstimatorInput(), baroAltitudeM, baroAltitudeValid);
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
        return;
    }

    _fusion.setTemperature(buffer.tempC);

    float gx = buffer.gx * DEG_TO_RAD;
    float gy = buffer.gy * DEG_TO_RAD;
    float gz = buffer.gz * DEG_TO_RAD;

    #ifdef USE_GY87
        _fusion.update(gx, gy, gz, buffer.ax, buffer.ay, buffer.az, buffer.mx, buffer.my, buffer.mz);
    #else
        _fusion.updateIMU(gx, gy, gz, buffer.ax, buffer.ay, buffer.az);
    #endif
}

EstimatorInput SensorPipeline::buildEstimatorInput() const {
    EstimatorInput input = {};
    input.rollDeg = _state.rollDeg;
    input.pitchDeg = _state.pitchDeg;
    input.yawDeg = _state.yawDeg;
    input.sensorHealth = _state.sensorHealth;
    input.timestampUs = _state.timestampUs;
    input.failsafe = false;
    return input;
}
