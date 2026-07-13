#include "ComplementaryEstimator.h"

void ComplementaryEstimator::init(float altitudeAlpha) {
    _altitudeAlpha = constrain(altitudeAlpha, 0.0f, 1.0f);
    reset();
}

void ComplementaryEstimator::reset() {
    _state = {};
    _state.health = SensorHealth::WarmingUp;
    _state.valid = false;
    _hasAltitude = false;
    _lastAltitudeM = 0.0f;
    _lastAltitudeTimestamp = 0;
}

EstimatedState ComplementaryEstimator::update(const EstimatorInput& input, float baroAltitudeM, bool baroValid) {
    _state.rollDeg = input.rollDeg;
    _state.pitchDeg = input.pitchDeg;
    _state.yawDeg = input.yawDeg;
    _state.timestamp = input.timestampUs;
    _state.health = input.sensorHealth;
    _state.valid = input.sensorHealth == SensorHealth::Ok && !input.failsafe;

    if (!baroValid) {
        if (!_hasAltitude) {
            _state.altitudeM = 0.0f;
            _state.verticalSpeedMps = 0.0f;
        }
        return _state;
    }

    if (!_hasAltitude) {
        _state.altitudeM = baroAltitudeM;
        _state.verticalSpeedMps = 0.0f;
        _hasAltitude = true;
    } else {
        float filteredAltitude = _altitudeAlpha * baroAltitudeM + (1.0f - _altitudeAlpha) * _state.altitudeM;
        float dt = (float)(input.timestampUs - _lastAltitudeTimestamp) / 1000000.0f;
        if (dt <= 0.0f || dt > 2.0f) {
            _state.verticalSpeedMps = 0.0f;
        } else {
            _state.verticalSpeedMps = (filteredAltitude - _lastAltitudeM) / dt;
        }
        _state.altitudeM = filteredAltitude;
    }

    _lastAltitudeM = _state.altitudeM;
    _lastAltitudeTimestamp = input.timestampUs;
    return _state;
}

EstimatedState ComplementaryEstimator::getState() const {
    return _state;
}
