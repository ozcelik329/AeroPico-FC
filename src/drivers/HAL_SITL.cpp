#include "HAL.h"

#ifdef SITL_MODE

// SITL: Gerçek donanım yok, simüle veriler üretilir

static float _simRoll  = 0.0f;
static float _simPitch = 0.0f;
static float _simYaw   = 0.0f;
static uint32_t _lastUpdate = 0;

void HAL::init() {
    _lastUpdate = micros();
    Serial.println("[HAL] SITL modu aktif — gercek donanim kullanilmiyor.");
    Serial.println("[HAL] Servo cikislari Serial'a yazdirilacak.");
}

HAL_IMUData HAL::readIMU() {
    uint32_t now = micros();
    float dt = (now - _lastUpdate) / 1000000.0f;
    _lastUpdate = now;

    // Basit simülasyon: sabit açısal hızla dönen uçak
    _simRoll  += SITL_ROLL_RATE  * dt;
    _simPitch += SITL_PITCH_RATE * dt;
    _simYaw   += SITL_YAW_RATE   * dt;

    // Açıları sınırla
    if (_simRoll  >  180.0f) _simRoll  -= 360.0f;
    if (_simPitch >   90.0f) _simPitch  =  90.0f;
    if (_simYaw   >  180.0f) _simYaw   -= 360.0f;

    // Basit gürültü (pseudo-random)
    auto noise = [](float amp) -> float {
        return amp * ((float)(micros() % 100 - 50) / 50.0f);
    };

    HAL_IMUData data;
    // İvme: yerçekimi vektörünü roll/pitch'e göre dönüştür
    float rollRad  = _simRoll  * DEG_TO_RAD;
    float pitchRad = _simPitch * DEG_TO_RAD;
    data.ax = -sin(pitchRad)              + noise(SITL_ACCEL_NOISE);
    data.ay =  sin(rollRad) * cos(pitchRad) + noise(SITL_ACCEL_NOISE);
    data.az =  cos(rollRad) * cos(pitchRad) + noise(SITL_ACCEL_NOISE);

    // Jiroskop: simüle açısal hızlar
    data.gx = SITL_ROLL_RATE  + noise(SITL_GYRO_NOISE);
    data.gy = SITL_PITCH_RATE + noise(SITL_GYRO_NOISE);
    data.gz = SITL_YAW_RATE   + noise(SITL_GYRO_NOISE);

    data.tempC = 25.0f;
    data.valid = true;
    return data;
}

HAL_RCData HAL::readRC() {
    // Simülasyonda RC: nötr pozisyon, throttle %50
    HAL_RCData data;
    data.ch[0] = PWM_NEUTRAL;  // roll
    data.ch[1] = PWM_NEUTRAL;  // pitch
    data.ch[2] = 1500;         // throttle %50
    data.ch[3] = PWM_NEUTRAL;  // yaw
    data.valid = true;
    return data;
}

void HAL::writeServos(const HAL_ServoOutput& out) {
    // Gerçek servo yerine Serial'a yaz
    Serial.printf("[SITL] AIL:%u ELE:%u RUD:%u THR:%u\n",
        out.aileron, out.elevator, out.rudder, out.throttle);
}

uint32_t HAL::micros_hal() { return micros(); }
uint32_t HAL::millis_hal() { return millis(); }

#endif  // SITL_MODE