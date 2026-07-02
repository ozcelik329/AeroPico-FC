#include "FlightManager.h"

void FlightManager::init() {
    // Deprecated: preserve default behavior if concrete drivers were set earlier
    if (_imu) _imu->init();
    fusion.init(0.08f);
    if (_rx) _rx->init();
    _modeController.init();
    _navController.init();
    _altController.init();
}

void FlightManager::init(IImuDriver* imuDrv, IRxDriver* rxDrv) {
    _imu = imuDrv;
    _rx  = rxDrv;
    init();
}

void FlightManager::update() {
    if (_imu) _imu->update();
    performSensorFusion();
    if (_rx) _rx->update();

    SensorBuffer buf = _imu ? _imu->getLatest() : SensorBuffer{};

    FlightData data;
    data.gyroX = buf.gx;
    data.gyroY = buf.gy;
    data.gyroZ = buf.gz;
    data.roll  = fusion.getRoll();
    data.pitch = fusion.getPitch();
    data.yaw   = fusion.getYaw();
    data.timestamp = micros();

    if (!_rx || !_rx->isValid()) {
        data.aileron  = PWM_NEUTRAL;
        data.elevator = PWM_NEUTRAL;
        data.throttle = PWM_MIN;
        data.rudder   = PWM_NEUTRAL;
        data.failsafe = true;
    } else {
        data.aileron  = _rx->getChannel(RC_ROLL_CHANNEL);
        data.elevator = _rx->getChannel(RC_PITCH_CHANNEL);
        data.throttle = _rx->getChannel(RC_THROTTLE_CHANNEL);
        data.rudder   = _rx->getChannel(RC_YAW_CHANNEL);
        data.failsafe = false;
    }

    updateControllers(data);
    _ringBuf.push(data);  // Lock-free yazma
}

void FlightManager::performSensorFusion() {
    SensorBuffer buf = _imu ? _imu->getLatest() : SensorBuffer{};

    fusion.setTemperature(buf.tempC);

    float gx = buf.gx * DEG_TO_RAD;
    float gy = buf.gy * DEG_TO_RAD;
    float gz = buf.gz * DEG_TO_RAD;

    #ifdef USE_GY87
        fusion.update(gx, gy, gz, buf.ax, buf.ay, buf.az, buf.mx, buf.my, buf.mz);
    #else
        fusion.updateIMU(gx, gy, gz, buf.ax, buf.ay, buf.az);
    #endif
}

// Consumer: pop all pending items and update the cached `_latest` (called by single consumer Core 1)
void FlightManager::consumeLatest() {
    FlightData tmp;
    while (_ringBuf.pop(tmp)) {
        // mark write in progress (odd)
        _latest_seq++;
        _latest = tmp;
        __dmb();
        // mark write done (even)
        _latest_seq++;
    }
}

// Peek the most recent item without consuming (safe for telemetry on Core 0)
bool FlightManager::peekLatest(FlightData& out) const {
    return _ringBuf.peek(out);
}

// Read a consistent snapshot of `_latest` using sequence counter
FlightData FlightManager::readLatestSnapshot() const {
    FlightData copy;
    while (true) {
        uint32_t s1 = _latest_seq;
        if (s1 & 1) continue; // writer in progress
        copy = _latest;
        uint32_t s2 = _latest_seq;
        if (s1 == s2) break;
    }
    return copy;
}

float    FlightManager::getRoll()     { return readLatestSnapshot().roll; }
float    FlightManager::getPitch()    { return readLatestSnapshot().pitch; }
float    FlightManager::getYaw()      { return readLatestSnapshot().yaw; }
float    FlightManager::getGyroX()    { return readLatestSnapshot().gyroX; }
float    FlightManager::getGyroY()    { return readLatestSnapshot().gyroY; }
float    FlightManager::getGyroZ()    { return readLatestSnapshot().gyroZ; }
uint16_t FlightManager::getAileron()  { return readLatestSnapshot().aileron; }
uint16_t FlightManager::getElevator() { return readLatestSnapshot().elevator; }
uint16_t FlightManager::getThrottle() { return readLatestSnapshot().throttle; }
uint16_t FlightManager::getRudder()   { return readLatestSnapshot().rudder; }

void FlightManager::updateControllers(const FlightData& data) {
    // Orkestrasyon: öncelikle mod/arm güncelle
    _modeController.update(data.throttle, data.rudder);

    // Navigation ve altitude kontrolörlerine güncel kumanda ve sensör verilerini ilet
    _navController.update(data.aileron, data.elevator, data.failsafe);
    // Şu an altimetre yok; altitude controller için yerleşik değer gönderiliyor
    _altController.update(0.0f, data.throttle, data.failsafe);
}
#include "FlightManager.h"

void FlightManager::init(IImuDriver* imu, IRxDriver* rx, IServoOutput* out) {
    _imu = imu; _rx = rx; _out = out;
    _ekf.init();
    _rollPID.reset();
    _pitchPID.reset();
}

void __not_in_flash_func(FlightManager::update)() {
    _imu->update();
    SensorBuffer s = _imu->getLatest();
    
    // EKF Adımları
    _ekf.predict(s.gx * 0.0174f, s.gy * 0.0174f, s.gz * 0.0174f, 0.002f);
    _ekf.updateIMU(s.ax, s.ay, s.az);

    _rx->update();
    _modeCtrl.update(_rx->getChannel(4));

    // Arming (A7 - Disarm logic)
    if (_rx->getChannel(2) < 1100 && _rx->getChannel(3) > 1900) _armed = true;
    if (_rx->getChannel(2) < 1100 && _rx->getChannel(3) < 1100) _armed = false;

    if (_armed && _rx->isValid()) {
        float rollTarget = (_rx->getChannel(0) - 1500) * 0.06f;
        float pitchTarget = (_rx->getChannel(1) - 1500) * 0.04f;
        
        float rollOut = _rollPID.compute(rollTarget, _ekf.roll, 0.002f, false);
        float pitchOut = _pitchPID.compute(pitchTarget, _ekf.pitch, 0.002f, false);
        
        _out->write(_rx->getChannel(2), (uint16_t)(1500 + rollOut), (uint16_t)(1500 + pitchOut), 1500);
    } else {
        _out->write(1000, 1500, 1500, 1500); // Failsafe / Disarmed
    }
}