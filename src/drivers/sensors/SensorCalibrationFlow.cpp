#include "../Sensors.h"
#include "../../utils/Logger.h"

bool SensorManager::runBootCalibration() {
    if (!_imuAvailable) return false;

    Logger::log("[SENSOR] Boot kalibrasyonu basliyor (ucak sabit olmali)...");
    _calibration.reset();

    for (int i = 0; i < BOOT_CAL_SAMPLES; i++) {
        int16_t rax, ray, raz, rgx, rgy, rgz, rtemp;
        if (!_readRawSample(rax, ray, raz, rgx, rgy, rgz, &rtemp)) {
            delay(2);
            continue;
        }
        _calibration.observeMpuRaw({rax, ray, raz, rgx, rgy, rgz, rtemp});
        delay(2);
    }

    const SensorCalibrationResult result = _calibration.finish(BOOT_CAL_SAMPLES / 2);
    if (!result.valid) {
        Logger::log("[SENSOR] Boot kalibrasyonu yetersiz ornek!");
        return false;
    }

    _imuCalibration = result.imu;
    _gyroBiasX = _imuCalibration.gyroBiasX;
    _gyroBiasY = _imuCalibration.gyroBiasY;
    _gyroBiasZ = _imuCalibration.gyroBiasZ;
    _accelBiasX = _imuCalibration.accelBiasX;
    _accelBiasY = _imuCalibration.accelBiasY;
    _accelBiasZ = _imuCalibration.accelBiasZ;

    char line[96];
    snprintf(line, sizeof(line), "[SENSOR] Gyro bias: %.3f, %.3f, %.3f deg/s",
             _gyroBiasX, _gyroBiasY, _gyroBiasZ);
    Logger::log(line);
    snprintf(line, sizeof(line), "[SENSOR] Accel bias: %.4f, %.4f, %.4f g",
             _accelBiasX, _accelBiasY, _accelBiasZ);
    Logger::log(line);
    snprintf(line, sizeof(line), "[SENSOR] Gyro temp coeff: %.5f deg/s/C (span %.2f C)",
             _imuCalibration.gyroTempCoeff, result.tempSpanC);
    Logger::log(line);
    return true;
}

bool SensorManager::beginImuCalibration() {
    if (!_imuAvailable || _imuCalibrationState == ImuCalibrationState::Collecting) {
        return false;
    }
    _calibration.reset();
    _imuCalibrationSamples = 0;
    _lastAsyncImuCalibration = {};
    _imuCalibrationState = ImuCalibrationState::Collecting;
    return true;
}

bool SensorManager::isImuCalibrationActive() const {
    return _imuCalibrationState == ImuCalibrationState::Collecting;
}

bool SensorManager::takeImuCalibrationResult(ImuCalibration& calibration, bool& success) {
    if (_imuCalibrationState != ImuCalibrationState::Complete &&
        _imuCalibrationState != ImuCalibrationState::Failed) {
        return false;
    }
    success = _imuCalibrationState == ImuCalibrationState::Complete;
    calibration = _lastAsyncImuCalibration;
    _imuCalibrationState = ImuCalibrationState::Idle;
    return true;
}

ImuCalibration SensorManager::getImuCalibration() const {
    return _imuCalibration;
}

void SensorManager::setImuCalibration(const ImuCalibration& calibration) {
    if (!calibration.valid) return;
    _imuCalibration = calibration;
    _gyroBiasX = calibration.gyroBiasX;
    _gyroBiasY = calibration.gyroBiasY;
    _gyroBiasZ = calibration.gyroBiasZ;
    _accelBiasX = calibration.accelBiasX;
    _accelBiasY = calibration.accelBiasY;
    _accelBiasZ = calibration.accelBiasZ;
    _gyroAccelDriver.resetFilters();
}

void SensorManager::beginMagCalibration() {
    _magDriver.beginCalibration();
}

bool SensorManager::observeMagCalibrationSample(float mx, float my, float mz) {
    return _magDriver.observeCalibrationSample(mx, my, mz);
}

MagCalibration SensorManager::finishMagCalibration() {
    return _magDriver.finishCalibration();
}

MagCalibration SensorManager::getMagCalibration() const {
    return _magDriver.getCalibration();
}

void SensorManager::setMagCalibration(const MagCalibration& calibration) {
    _magDriver.setCalibration(calibration);
}

void SensorManager::_finishAsyncImuCalibration() {
    const SensorCalibrationResult result = _calibration.finish(BOOT_CAL_SAMPLES / 2);
    if (!result.valid) {
        _lastAsyncImuCalibration = {};
        _imuCalibrationState = ImuCalibrationState::Failed;
        return;
    }
    setImuCalibration(result.imu);
    _lastAsyncImuCalibration = result.imu;
    _imuCalibrationState = ImuCalibrationState::Complete;
}
