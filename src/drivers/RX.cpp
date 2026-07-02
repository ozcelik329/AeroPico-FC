#include "RX.h"
#include "sbus.h"

bfs::SbusRx sbus_rx(&Serial2);
bfs::SbusData sbus_data;

void RXManager::init() {
    // Serial2 → GP1 (RX), transistör ile invert edilmiş SBUS
    Serial2.setRX(PIN_SBUS_RX);
    Serial2.begin(100000, SERIAL_8E2);
    sbus_rx.Begin();

    valid          = false;
    _failsafe      = false;
    _lastValidTime = millis();

    for (int i = 0; i < 16; i++) {
        channels[i] = PWM_NEUTRAL;
    }
}

void RXManager::update() {
    if (sbus_rx.Read()) {
        sbus_data = sbus_rx.data();

        // SBUS protokolünün kendi failsafe biti
        if (sbus_data.failsafe) {
            _failsafe = true;
            valid     = false;
            return;
        }

        valid          = true;
        _failsafe      = false;
        _lastValidTime = millis();

        for (int i = 0; i < 16; i++) {
            channels[i] = constrain(
                map(sbus_data.ch[i], 172, 1811, 1000, 2000),
                1000, 2000
            );
        }
    } else {
        // Belirli süre sinyal gelmezse failsafe
        if (millis() - _lastValidTime > FAILSAFE_TIMEOUT_MS) {
            _failsafe = true;
            valid     = false;
        }
    }
}

bool RXManager::isValid() const {
    return valid && !_failsafe;
}

bool RXManager::isFailsafe() const {
    return _failsafe;
}

uint32_t RXManager::lastValidMs() const {
    return _lastValidTime;
}

uint16_t RXManager::getChannel(int ch) const {
    if (_failsafe) {
        // Failsafe değerleri döndür
        if (ch == RC_THROTTLE_CHANNEL) return FAILSAFE_THROTTLE;
        if (ch == RC_ROLL_CHANNEL)     return FAILSAFE_AILERON;
        if (ch == RC_PITCH_CHANNEL)    return FAILSAFE_ELEVATOR;
        if (ch == RC_YAW_CHANNEL)      return FAILSAFE_RUDDER;
        return PWM_NEUTRAL;
    }
    if (ch >= 0 && ch < 16) return channels[ch];
    return PWM_NEUTRAL;
}
if (millis() - _last_ms > 500) _valid = false;


void RXManager::update() {
    if (SBUS_SERIAL.available() >= 25) {
        uint8_t buf[25];
        SBUS_SERIAL.readBytes(buf, 25);
        if (buf[0] == 0x0F && buf[24] == 0x00) {
            // SBUS Parsing
            _channels[0] = ((buf[1] | buf[2] << 8) & 0x07FF);
            _channels[1] = ((buf[2] >> 3 | buf[3] << 5) & 0x07FF);
            _channels[2] = ((buf[3] >> 6 | buf[4] << 2 | buf[5] << 10) & 0x07FF);
            _channels[3] = ((buf[5] >> 1 | buf[6] << 7) & 0x07FF);
            _channels[4] = ((buf[6] >> 4 | buf[7] << 4) & 0x07FF); // Mod Switch
            
            _valid = !(buf[23] & 0x08); // SBUS Failsafe biti
            _last_ms = millis();
        }
    }
    // Sinyal zaman aşımı (Risk 4)
    if (millis() - _last_ms > 500) _valid = false;
}