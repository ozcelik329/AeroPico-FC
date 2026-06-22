#include "FlightManager.h"

void FlightManager::init() {
    mutex_init(&_mutex);

    sensors.init();
    fusion.init(0.08f);
    rx.init();
}

void FlightManager::update() {
    // Sensörleri oku (Core 0)
    sensors.update();
    performSensorFusion();
    rx.update();

    // Güvenli yazma
    mutex_enter_blocking(&_mutex);

    _gyroX = sensors.gx;
    _gyroY = sensors.gy;
    _gyroZ = sensors.gz;

    if (!rx.isValid()) {
        _aileron  = PWM_NEUTRAL;
        _elevator = PWM_NEUTRAL;
        _throttle = PWM_MIN;
        _rudder   = PWM_NEUTRAL;
    } else {
        _aileron  = rx.getChannel(RC_ROLL_CHANNEL);
        _elevator = rx.getChannel(RC_PITCH_CHANNEL);
        _throttle = rx.getChannel(RC_THROTTLE_CHANNEL);
        _rudder   = rx.getChannel(RC_YAW_CHANNEL);
    }

    mutex_exit(&_mutex);
}

void FlightManager::performSensorFusion() {
    float ax = sensors.ax;
    float ay = sensors.ay;
    float az = sensors.az;
    float gx = sensors.gx * DEG_TO_RAD;
    float gy = sensors.gy * DEG_TO_RAD;
    float gz = sensors.gz * DEG_TO_RAD;

    #ifdef USE_GY87
        float mx = sensors.mx;
        float my = sensors.my;
        float mz = sensors.mz;
        fusion.update(gx, gy, gz, ax, ay, az, mx, my, mz);
    #else
        fusion.updateIMU(gx, gy, gz, ax, ay, az);
    #endif

    // Fusion sonuçları da mutex altında yaz
    mutex_enter_blocking(&_mutex);
    _roll  = fusion.getRoll();
    _pitch = fusion.getPitch();
    _yaw   = fusion.getYaw();
    mutex_exit(&_mutex);
}

// --- Güvenli getter'lar (Core 1 tarafından çağrılır) ---

float FlightManager::getRoll() {
    mutex_enter_blocking(&_mutex);
    float v = _roll;
    mutex_exit(&_mutex);
    return v;
}

float FlightManager::getPitch() {
    mutex_enter_blocking(&_mutex);
    float v = _pitch;
    mutex_exit(&_mutex);
    return v;
}

float FlightManager::getYaw() {
    mutex_enter_blocking(&_mutex);
    float v = _yaw;
    mutex_exit(&_mutex);
    return v;
}

float FlightManager::getGyroX() {
    mutex_enter_blocking(&_mutex);
    float v = _gyroX;
    mutex_exit(&_mutex);
    return v;
}

float FlightManager::getGyroY() {
    mutex_enter_blocking(&_mutex);
    float v = _gyroY;
    mutex_exit(&_mutex);
    return v;
}

float FlightManager::getGyroZ() {
    mutex_enter_blocking(&_mutex);
    float v = _gyroZ;
    mutex_exit(&_mutex);
    return v;
}

uint16_t FlightManager::getAileron() {
    mutex_enter_blocking(&_mutex);
    uint16_t v = _aileron;
    mutex_exit(&_mutex);
    return v;
}

uint16_t FlightManager::getElevator() {
    mutex_enter_blocking(&_mutex);
    uint16_t v = _elevator;
    mutex_exit(&_mutex);
    return v;
}

uint16_t FlightManager::getThrottle() {
    mutex_enter_blocking(&_mutex);
    uint16_t v = _throttle;
    mutex_exit(&_mutex);
    return v;
}

uint16_t FlightManager::getRudder() {
    mutex_enter_blocking(&_mutex);
    uint16_t v = _rudder;
    mutex_exit(&_mutex);
    return v;
}
