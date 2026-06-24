#include "HAL.h"

#ifndef SITL_MODE

#include "Sensors.h"
#include "RX.h"
#include "Output.h"

extern SensorManager sensors;
extern RXManager rx;

void HAL::init() {
    // Gerçek donanım zaten FlightManager::init() içinde başlatılıyor
    Serial.println("[HAL] Gercek donanim modu.");
}

HAL_IMUData HAL::readIMU() {
    SensorBuffer buf = sensors.getLatest();
    HAL_IMUData data;
    data.ax    = buf.ax;
    data.ay    = buf.ay;
    data.az    = buf.az;
    data.gx    = buf.gx;
    data.gy    = buf.gy;
    data.gz    = buf.gz;
    data.tempC = buf.tempC;
    data.valid = buf.valid;
    return data;
}

HAL_RCData HAL::readRC() {
    HAL_RCData data;
    data.ch[0] = rx.getChannel(RC_ROLL_CHANNEL);
    data.ch[1] = rx.getChannel(RC_PITCH_CHANNEL);
    data.ch[2] = rx.getChannel(RC_THROTTLE_CHANNEL);
    data.ch[3] = rx.getChannel(RC_YAW_CHANNEL);
    data.valid = rx.isValid();
    return data;
}

void HAL::writeServos(const HAL_ServoOutput& out) {
    writeMotors(out.throttle, out.aileron, out.elevator, out.rudder);
}

uint32_t HAL::micros_hal() { return micros(); }
uint32_t HAL::millis_hal() { return millis(); }

#endif  // !SITL_MODE