#include "BaroAltitudeEstimator.h"

void BaroAltitudeEstimator::init(float metersPerHpa) {
    _metersPerHpa = constrain(metersPerHpa, 1.0f, 20.0f);
    reset();
}

void BaroAltitudeEstimator::reset() {
    _homePressureHpa = 1013.25f;
    _hasHome = false;
}

void BaroAltitudeEstimator::setHomePressure(float pressureHpa) {
    if (pressureHpa > 100.0f && pressureHpa < 1200.0f) {
        _homePressureHpa = pressureHpa;
        _hasHome = true;
    }
}

bool BaroAltitudeEstimator::update(float pressureHpa, bool pressureValid, float& altitudeM) {
    if (!pressureValid || pressureHpa <= 100.0f || pressureHpa >= 1200.0f) {
        altitudeM = 0.0f;
        return false;
    }

    if (!_hasHome) {
        setHomePressure(pressureHpa);
    }

    altitudeM = (_homePressureHpa - pressureHpa) * _metersPerHpa;
    return _hasHome;
}
