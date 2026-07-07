#include "RX.h"
#include "../board/PinValidation.h"
#include "sbus.h"

#if SBUS_UART_INDEX == 0
bfs::SbusRx sbus_rx(&Serial1);
#elif SBUS_UART_INDEX == 1
bfs::SbusRx sbus_rx(&Serial2);
#endif

bfs::SbusData sbus_data;

void RXManager::init() {
#if SBUS_UART_INDEX == 0
    // Serial1 (UART0) -> GP1 (RX), transistör ile invert edilmiş SBUS
    Serial1.setRX(PIN_SBUS_RX);
    Serial1.begin(100000, SERIAL_8E2);
#elif SBUS_UART_INDEX == 1
    Serial2.setRX(PIN_SBUS_RX);
    Serial2.begin(100000, SERIAL_8E2);
#endif
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
