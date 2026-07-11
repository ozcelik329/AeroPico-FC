#include "BaroVerticalKalman.h"

void BaroVerticalKalman::init(const BaroVerticalKalmanConfig& config) {
    _config.altitudeProcessNoise = constrain(config.altitudeProcessNoise, 0.0001f, 100.0f);
    _config.velocityProcessNoise = constrain(config.velocityProcessNoise, 0.0001f, 100.0f);
    _config.baroMeasurementNoise = constrain(config.baroMeasurementNoise, 0.0001f, 1000.0f);
    _config.innovationGateM = constrain(config.innovationGateM, 0.1f, 1000.0f);
    reset();
}

void BaroVerticalKalman::reset() {
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
    _consecutiveRejects = 0;
}

float BaroVerticalKalman::clampDt(uint32_t nowUs) {
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

void BaroVerticalKalman::predict(float dt) {
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
    constrainCovariance();
}

bool BaroVerticalKalman::correct(float baroAltitudeM) {
    if (!isfinite(baroAltitudeM)) {
        _lastMeasurementRejected = true;
        _consecutiveRejects = _consecutiveRejects < 255 ? (uint8_t)(_consecutiveRejects + 1u) : _consecutiveRejects;
        return false;
    }
    _lastInnovationM = baroAltitudeM - _altitudeM;
    _lastMeasurementRejected = fabsf(_lastInnovationM) > _config.innovationGateM;
    if (_lastMeasurementRejected) {
        _consecutiveRejects = _consecutiveRejects < 255 ? (uint8_t)(_consecutiveRejects + 1u) : _consecutiveRejects;
        return false;
    }
    _consecutiveRejects = 0;

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
    constrainCovariance();

    return true;
}

bool BaroVerticalKalman::stateFinite() const {
    return isfinite(_altitudeM) &&
           isfinite(_verticalSpeedMps) &&
           isfinite(_p00) &&
           isfinite(_p01) &&
           isfinite(_p10) &&
           isfinite(_p11);
}

void BaroVerticalKalman::constrainCovariance() {
    constexpr float MIN_COV = 0.0001f;
    constexpr float MAX_COV = 100000.0f;
    if (!stateFinite()) {
        reset();
        _state.health = SensorHealth::Invalid;
        return;
    }
    _p00 = constrain(_p00, MIN_COV, MAX_COV);
    _p11 = constrain(_p11, MIN_COV, MAX_COV);
    _p01 = constrain(_p01, -MAX_COV, MAX_COV);
    _p10 = constrain(_p10, -MAX_COV, MAX_COV);
}

void BaroVerticalKalman::copyAttitude(const EstimatorInput& input) {
    _state.rollDeg = input.rollDeg;
    _state.pitchDeg = input.pitchDeg;
    _state.yawDeg = input.yawDeg;
    _state.timestamp = input.timestampUs;
    _state.health = input.sensorHealth;
    _state.valid = input.sensorHealth == SensorHealth::Ok && !input.failsafe;
}

void BaroVerticalKalman::updateEstimatorHealth(bool baroValid) {
    constexpr uint8_t MAX_CONSECUTIVE_REJECTS = 3;
    constexpr float MAX_OBSERVABILITY_VARIANCE = 2500.0f;
    if (_consecutiveRejects >= MAX_CONSECUTIVE_REJECTS) {
        _state.health = SensorHealth::Stale;
        _state.valid = false;
        return;
    }
    if (baroValid && (_p00 > MAX_OBSERVABILITY_VARIANCE || _p11 > MAX_OBSERVABILITY_VARIANCE)) {
        _state.health = SensorHealth::WarmingUp;
        _state.valid = false;
    }
}

EstimatedState BaroVerticalKalman::update(const EstimatorInput& input, float baroAltitudeM, bool baroValid) {
    copyAttitude(input);
    if (!isfinite(input.rollDeg) || !isfinite(input.pitchDeg) || !isfinite(input.yawDeg)) {
        _state.health = SensorHealth::Invalid;
        _state.valid = false;
        return _state;
    }

    if (!_initialized) {
        if (baroValid && !isfinite(baroAltitudeM)) {
            _state.health = SensorHealth::Invalid;
            _state.valid = false;
            return _state;
        }
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
    if (!stateFinite()) {
        _state.health = SensorHealth::Invalid;
        _state.valid = false;
    }
    updateEstimatorHealth(baroValid);
    return _state;
}

EstimatedState BaroVerticalKalman::getState() const {
    return _state;
}
