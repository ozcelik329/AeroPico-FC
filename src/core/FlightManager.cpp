#include "FlightManager.h"

void FlightManager::init() {
    sensors.init();
    fusion.init(0.08f);
    rx.init();
}

void FlightManager::update() {
    sensors.update();
    performSensorFusion();
    rx.update();

    SensorBuffer buf = sensors.getLatest();

    FlightData data;
    data.gyroX = buf.gx;
    data.gyroY = buf.gy;
    data.gyroZ = buf.gz;
    data.roll  = fusion.getRoll();
    data.pitch = fusion.getPitch();
    data.yaw   = fusion.getYaw();
    data.timestamp = micros();

    if (!rx.isValid()) {
        data.aileron  = PWM_NEUTRAL;
        data.elevator = PWM_NEUTRAL;
        data.throttle = PWM_MIN;
        data.rudder   = PWM_NEUTRAL;
        data.failsafe = true;
    } else {
        data.aileron  = rx.getChannel(RC_ROLL_CHANNEL);
        data.elevator = rx.getChannel(RC_PITCH_CHANNEL);
        data.throttle = rx.getChannel(RC_THROTTLE_CHANNEL);
        data.rudder   = rx.getChannel(RC_YAW_CHANNEL);
        data.failsafe = false;
    }

    updateArmDisarm(data.throttle, data.rudder);
    _ringBuf.push(data);  // Lock-free yazma
}

void FlightManager::performSensorFusion() {
    SensorBuffer buf = sensors.getLatest();

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

// Core 1 tarafında çağrılır — lock-free okuma
static FlightData _getLatest(RingBuffer<FlightData, 4>& buf, FlightData& cache) {
    FlightData tmp;
    while (buf.pop(tmp)) {
        cache = tmp;  // En güncel veriyi al
    }
    return cache;
}

float    FlightManager::getRoll()     { return _getLatest(_ringBuf, _latest).roll; }
float    FlightManager::getPitch()    { return _getLatest(_ringBuf, _latest).pitch; }
float    FlightManager::getYaw()      { return _getLatest(_ringBuf, _latest).yaw; }
float    FlightManager::getGyroX()    { return _getLatest(_ringBuf, _latest).gyroX; }
float    FlightManager::getGyroY()    { return _getLatest(_ringBuf, _latest).gyroY; }
float    FlightManager::getGyroZ()    { return _getLatest(_ringBuf, _latest).gyroZ; }
uint16_t FlightManager::getAileron()  { return _getLatest(_ringBuf, _latest).aileron; }
uint16_t FlightManager::getElevator() { return _getLatest(_ringBuf, _latest).elevator; }
uint16_t FlightManager::getThrottle() { return _getLatest(_ringBuf, _latest).throttle; }
uint16_t FlightManager::getRudder()   { return _getLatest(_ringBuf, _latest).rudder; }

void FlightManager::updateArmDisarm(uint16_t throttle, uint16_t rudder) {
    uint32_t now = millis();

    if (!_armed) {
        if (throttle < ARM_THROTTLE_MAX && rudder > ARM_RUDDER_MIN) {
            if (_armHoldStart == 0) _armHoldStart = now;
            if (now - _armHoldStart >= ARM_HOLD_MS) {
                _armed = true;
                _armHoldStart = 0;
                Serial.println("[ARM] Sistem arm edildi!");
            }
        } else {
            _armHoldStart = 0;
        }
    } else {
        if (throttle < ARM_THROTTLE_MAX && rudder < DISARM_RUDDER_MAX) {
            if (_disarmHoldStart == 0) _disarmHoldStart = now;
            if (now - _disarmHoldStart >= ARM_HOLD_MS) {
                _armed = false;
                _disarmHoldStart = 0;
                Serial.println("[ARM] Sistem disarm edildi!");
            }
        } else {
            _disarmHoldStart = 0;
        }
    }
}