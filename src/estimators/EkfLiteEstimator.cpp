#include "EkfLiteEstimator.h"

void EkfLiteEstimator::init(const EkfLiteConfig& config) {
    _config.altitudeProcessNoise = constrain(config.altitudeProcessNoise, 0.0001f, 100.0f);
    _config.velocityProcessNoise = constrain(config.velocityProcessNoise, 0.0001f, 100.0f);
    _config.baroMeasurementNoise = constrain(config.baroMeasurementNoise, 0.0001f, 1000.0f);
    _config.innovationGateM = constrain(config.innovationGateM, 0.1f, 1000.0f);
    reset();
}

void EkfLiteEstimator::reset() {
    _state = {};
    _state.health = SensorHealth::WarmingUp;
    _state.valid = false;
    _initialized = false;
    _lastTimestampUs = 0;
    _altitudeM = 0.0f;
    _verticalSpeedMps = 0.0f;
    _p00 = 4.0f;
    _p01 = 0.0f;
    _p10 = 0.0f;
    _p11 = 4.0f;
    _lastInnovationM = 0.0f;
    _lastMeasurementRejected = false;
}

float EkfLiteEstimator::clampDt(uint32_t nowUs) {
    if (_lastTimestampUs == 0) {
        _lastTimestampUs = nowUs;
        return 0.0f;
    }

    float dt = (float)(nowUs - _lastTimestampUs) * 0.000001f;
    _lastTimestampUs = nowUs;
    if (dt <= 0.0f || dt > 1.0f) {
        return 0.0f;
    }
    return dt;
}

void EkfLiteEstimator::predict(float dt) {
    if (dt <= 0.0f) {
        return;
    }

    _altitudeM += _verticalSpeedMps * dt;

    const float dt2 = dt * dt;
    const float np00 = _p00 + dt * (_p10 + _p01) + dt2 * _p11 + _config.altitudeProcessNoise * dt;
    const float np01 = _p01 + dt * _p11;
    const float np10 = _p10 + dt * _p11;
    const float np11 = _p11 + _config.velocityProcessNoise * dt;

    _p00 = np00;
    _p01 = np01;
    _p10 = np10;
    _p11 = np11;
}

bool EkfLiteEstimator::correct(float baroAltitudeM) {
    _lastInnovationM = baroAltitudeM - _altitudeM;
    _lastMeasurementRejected = fabsf(_lastInnovationM) > _config.innovationGateM;
    if (_lastMeasurementRejected) {
        return false;
    }

    const float s = _p00 + _config.baroMeasurementNoise;
    if (s <= 0.000001f) {
        return false;
    }

    const float invS = 1.0f / s;
    const float k0 = _p00 * invS;
    const float k1 = _p10 * invS;

    _altitudeM += k0 * _lastInnovationM;
    _verticalSpeedMps += k1 * _lastInnovationM;

    const float p00 = _p00;
    const float p01 = _p01;
    _p00 -= k0 * p00;
    _p01 -= k0 * p01;
    _p10 -= k1 * p00;
    _p11 -= k1 * p01;

    return true;
}

void EkfLiteEstimator::copyAttitude(const EstimatorInput& input) {
    _state.rollDeg = input.rollDeg;
    _state.pitchDeg = input.pitchDeg;
    _state.yawDeg = input.yawDeg;
    _state.timestamp = input.timestampUs;
    _state.health = input.sensorHealth;
    _state.valid = input.sensorHealth == SensorHealth::Ok && !input.failsafe;
}

EstimatedState EkfLiteEstimator::update(const EstimatorInput& input, float baroAltitudeM, bool baroValid) {
    copyAttitude(input);

    if (!_initialized) {
        if (baroValid) {
            _altitudeM = baroAltitudeM;
            _verticalSpeedMps = 0.0f;
            _initialized = true;
            _lastTimestampUs = input.timestampUs;
        }
        _state.altitudeM = _altitudeM;
        _state.verticalSpeedMps = _verticalSpeedMps;
        return _state;
    }

    const float dt = clampDt(input.timestampUs);
    predict(dt);
    if (baroValid) {
        correct(baroAltitudeM);
    } else {
        _lastMeasurementRejected = false;
        _lastInnovationM = 0.0f;
    }

    _state.altitudeM = _altitudeM;
    _state.verticalSpeedMps = _verticalSpeedMps;
    return _state;
}

EstimatedState EkfLiteEstimator::getState() const {
    return _state;
}
