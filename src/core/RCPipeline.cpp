#include "RCPipeline.h"

static uint16_t clampPwm(uint16_t value) {
    if (value < PWM_MIN) return PWM_MIN;
    if (value > PWM_MAX) return PWM_MAX;
    return value;
}

static uint8_t clampChannel(uint8_t value) {
    return value > 7 ? 7 : value;
}

void RCPipeline::init(IRxDriver* rxDriver) {
    _rx = rxDriver;
    if (_rx) {
        _rx->init();
    }
    clearOverride();
    _state = failsafeState(millis());
}

RcInputState RCPipeline::failsafeState(uint32_t nowMs) const {
    RcInputState state;
    state.aileron = PWM_NEUTRAL;
    state.elevator = PWM_NEUTRAL;
    state.throttle = PWM_MIN;
    state.rudder = PWM_NEUTRAL;
    state.failsafe = true;
    state.overrideActive = false;
    state.timestampMs = nowMs;
    return state;
}

RcInputState RCPipeline::update() {
    uint32_t nowMs = millis();

    if (_rx) {
        _rx->update();
    }

    if (_overrideActive && (nowMs - _overrideLastMs > MAVLINK_RC_OVERRIDE_TIMEOUT_MS)) {
        clearOverride();
    }

    if (!_rx || !_rx->isValid() || _rx->isFailsafe()) {
        _state = failsafeState(nowMs);
        return _state;
    }

    if (_overrideActive) {
        _state.aileron = _overrideAileron;
        _state.elevator = _overrideElevator;
        _state.throttle = _overrideThrottle;
        _state.rudder = _overrideRudder;
        _state.failsafe = false;
        _state.overrideActive = true;
        _state.timestampMs = nowMs;
        return _state;
    }

    _state.aileron = _rx->getChannel(_mapping.rollChannel);
    _state.elevator = _rx->getChannel(_mapping.pitchChannel);
    _state.throttle = _rx->getChannel(_mapping.throttleChannel);
    _state.rudder = _rx->getChannel(_mapping.yawChannel);
    _state.failsafe = false;
    _state.overrideActive = false;
    _state.timestampMs = nowMs;
    return _state;
}

RcInputState RCPipeline::getState() const {
    return _state;
}

void RCPipeline::applyMapping(const RcMapping& mapping) {
    _mapping.rollChannel = clampChannel(mapping.rollChannel);
    _mapping.pitchChannel = clampChannel(mapping.pitchChannel);
    _mapping.throttleChannel = clampChannel(mapping.throttleChannel);
    _mapping.yawChannel = clampChannel(mapping.yawChannel);
}

void RCPipeline::setOverride(uint16_t aileron, uint16_t elevator, uint16_t throttle, uint16_t rudder) {
    _overrideActive = true;
    _overrideLastMs = millis();
    _overrideAileron = clampPwm(aileron);
    _overrideElevator = clampPwm(elevator);
    _overrideThrottle = clampPwm(throttle);
    _overrideRudder = clampPwm(rudder);
}

void RCPipeline::clearOverride() {
    _overrideActive = false;
}
