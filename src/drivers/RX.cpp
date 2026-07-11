#include "RX.h"
#include "../board/PinValidation.h"

#if !defined(UNIT_TEST)
#include "sbus.h"

#if SBUS_UART_INDEX == 0
static bfs::SbusRx sbus_rx(&Serial1);
#elif SBUS_UART_INDEX == 1
static bfs::SbusRx sbus_rx(&Serial2);
#endif

class HardwareSbusBackend : public ISbusBackend {
  public:
    void begin() override {
#if SBUS_UART_INDEX == 0
        // Serial1 (UART0) -> GP1 (RX), transistör ile invert edilmiş SBUS
        Serial1.setRX(PIN_SBUS_RX);
        Serial1.begin(100000, SERIAL_8E2);
#elif SBUS_UART_INDEX == 1
        Serial2.setRX(PIN_SBUS_RX);
        Serial2.begin(100000, SERIAL_8E2);
#endif
        sbus_rx.Begin();
    }

    bool readFrame(SbusFrameView& frame) override {
        if (!sbus_rx.Read()) {
            return false;
        }
        const bfs::SbusData data = sbus_rx.data();
        frame.failsafe = data.failsafe;
        for (int i = 0; i < 16; ++i) {
            frame.channels[i] = data.ch[i];
        }
        return true;
    }
};

static HardwareSbusBackend defaultSbusBackend;
#endif

void RXManager::setBackend(ISbusBackend* backend) {
    _backend = backend;
}

void RXManager::init() {
#if !defined(UNIT_TEST)
    if (!_backend) {
        _backend = &defaultSbusBackend;
    }
#endif
    if (_backend) {
        _backend->begin();
    }

    valid          = false;
    _failsafe      = false;
    _lastValidTime = millis();

    for (int i = 0; i < 16; i++) {
        channels[i] = PWM_NEUTRAL;
    }
}

void RXManager::update() {
    SbusFrameView frame = {};
    if (_backend && _backend->readFrame(frame)) {

        // SBUS protokolünün kendi failsafe biti
        if (frame.failsafe) {
            _failsafe = true;
            valid     = false;
            return;
        }

        valid          = true;
        _failsafe      = false;
        _lastValidTime = millis();

        SbusMapper::applyFrame(frame, channels, 16);
    } else {
        // Belirli süre sinyal gelmezse failsafe
        if (millis() - _lastValidTime > _failsafeTimeoutMs) {
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

void RXManager::setFailsafeTimeoutMs(uint32_t timeoutMs) {
    _failsafeTimeoutMs = constrain(timeoutMs, 50u, 5000u);
}
