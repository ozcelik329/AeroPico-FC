#include "SensorPipeline.h"

void SensorPipeline::init(IImuDriver* imuDriver, float fusionBeta) {
    _imu = imuDriver;
    if (_imu) {
        _imu->init();
    }
    _fusion.init(fusionBeta);
    _state = {};
    _state.sensorHealth = SensorHealth::WarmingUp;
    _state.valid = false;
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
    _state.timestampUs = micros();
    _state.valid = buffer.valid && buffer.health == SensorHealth::Ok;
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
