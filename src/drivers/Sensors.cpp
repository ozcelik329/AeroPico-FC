#include "Sensors.h"
#include "../hal/rp2350/RP2350_I2C.h"
#include "../utils/Logger.h"

static RP2350I2C defaultI2cBus(i2c0);

IHALI2C& SensorManager::_bus() {
    return _i2cBus ? *_i2cBus : defaultI2cBus;
}

RP2350I2C* SensorManager::_rpBus() {
    if (_rp2350Bus) {
        return _rp2350Bus;
    }
    return _i2cBus ? nullptr : &defaultI2cBus;
}

void SensorManager::setI2CBus(IHALI2C* bus) {
    _i2cBus = bus;
    _rp2350Bus = nullptr;
    _dmaFastPath = false;
}

void SensorManager::setI2CBus(RP2350I2C* bus) {
    _i2cBus = bus;
    _rp2350Bus = bus;
    _dmaFastPath = false;
}

void SensorManager::_setFault(SensorFaultCode code) {
    if (code != SensorFaultCode::None) {
        _faultCode = code;
    }
}

void SensorManager::_mpu_write_reg(uint8_t reg, uint8_t val) {
    const ImuDeviceProfile& imu = *_imuProfile;
    if (!_bus().writeRegister(imu.address, reg, val)) {
        _setFault(SensorFaultCode::I2cRawWriteFailed);
    }
}

bool SensorManager::_readWhoAmI(uint8_t& whoami) {
    const ImuDeviceProfile& imu = *_imuProfile;
    const uint8_t reg = imu.whoAmIReg;
    if (!_bus().writeRaw(imu.address, &reg, 1, true)) {
        _setFault(SensorFaultCode::I2cWhoamiWriteFailed);
        return false;
    }
    if (!_bus().readRaw(imu.address, &whoami, 1, false)) {
        _setFault(SensorFaultCode::I2cWhoamiReadFailed);
        return false;
    }
    _lastWhoAmI = whoami;
    return true;
}

bool SensorManager::_readRawSample(int16_t& raw_ax, int16_t& raw_ay, int16_t& raw_az,
                                   int16_t& raw_gx, int16_t& raw_gy, int16_t& raw_gz,
                                   int16_t* raw_temp) {
    uint8_t raw[GyroAccelDriver::RAW_LEN];
    if (!_readRawFrame(raw)) {
        return false;
    }

    auto to_int16 = [](uint8_t hi, uint8_t lo) -> int16_t {
        return (int16_t)((hi << 8) | lo);
    };

    raw_ax = to_int16(raw[0], raw[1]);
    raw_ay = to_int16(raw[2], raw[3]);
    raw_az = to_int16(raw[4], raw[5]);
    if (raw_temp) {
        *raw_temp = to_int16(raw[6], raw[7]);
    }
    raw_gx = to_int16(raw[8], raw[9]);
    raw_gy = to_int16(raw[10], raw[11]);
    raw_gz = to_int16(raw[12], raw[13]);
    return true;
}

bool SensorManager::_readRawFrame(uint8_t raw[GyroAccelDriver::RAW_LEN]) {
    const ImuDeviceProfile& imu = *_imuProfile;
    uint8_t reg = imu.accelReg;
    if (!_bus().writeRaw(imu.address, &reg, 1, true)) {
        _setFault(SensorFaultCode::I2cRawWriteFailed);
        return false;
    }
    if (!_bus().readRaw(imu.address, raw, imu.rawSampleLen, false)) {
        _setFault(SensorFaultCode::I2cRawReadFailed);
        return false;
    }
    return true;
}

bool SensorManager::isImuAvailable() const { return _imuAvailable; }
bool SensorManager::isDmaOk() const { return _dmaBus.hasMpuChannels(); }

SensorCapabilityStatus SensorManager::capabilities() const {
    SensorCapabilityStatus status = {};
    status.imuAvailable = _imuAvailable;
    status.magAvailable = hasMag();
    status.baroAvailable = hasBaro();
    status.gpsAvailable = false;
    if (status.imuAvailable) status.functionMask |= SENSOR_CAP_IMU;
    if (status.magAvailable) status.functionMask |= SENSOR_CAP_MAG;
    if (status.baroAvailable) status.functionMask |= SENSOR_CAP_BARO;
    return status;
}

SensorFaultCode SensorManager::getFaultCode() const {
    return _faultCode;
}

const char* SensorManager::getFaultText() const {
    switch (_faultCode) {
        case SensorFaultCode::None: return "None";
        case SensorFaultCode::I2cWhoamiWriteFailed: return "I2C WHOAMI write failed";
        case SensorFaultCode::I2cWhoamiReadFailed: return "I2C WHOAMI read failed";
        case SensorFaultCode::WhoamiMismatch: return "MPU6050 WHOAMI mismatch";
        case SensorFaultCode::I2cRawWriteFailed: return "I2C raw write failed";
        case SensorFaultCode::I2cRawReadFailed: return "I2C raw read failed";
        case SensorFaultCode::DmaChannelClaimFailed: return "DMA channel claim failed";
        case SensorFaultCode::DmaTransferTimeout: return "DMA transfer timeout";
        case SensorFaultCode::AuxI2cWriteFailed: return "Aux I2C write failed";
        case SensorFaultCode::AuxDmaTransferTimeout: return "Aux DMA transfer timeout";
        case SensorFaultCode::AuxPollingFallbackFailed: return "Aux polling fallback failed";
        case SensorFaultCode::MagReadFailed: return "Mag read failed";
        case SensorFaultCode::BaroReadFailed: return "Baro read failed";
        default: return "Unknown sensor fault";
    }
}

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

void SensorManager::init() {
    mutex_init(&_mutex);
    _buf[0] = {};
    _buf[1] = {};
    _buf[0].health = SensorHealth::WarmingUp;
    _buf[1].health = SensorHealth::WarmingUp;
    _gyroAccelDriver.resetFilters();

    _bus().init(PIN_SDA, PIN_SCL, 400000);

    uint8_t whoami = 0;
    const ImuDeviceProfile& imu = *_imuProfile;
    if (!_readWhoAmI(whoami) || whoami != imu.whoAmIValue) {
        _imuAvailable = false;
        if (whoami != imu.whoAmIValue) {
            _setFault(SensorFaultCode::WhoamiMismatch);
        }
        char line[80];
        snprintf(line, sizeof(line), "[SENSOR] MPU6050 WHOAMI hatasi: 0x%02X (beklenen 0x%02X)",
                 whoami, imu.whoAmIValue);
        Logger::log(line);
    } else {
        _imuAvailable = true;
    }

    if (!_imuAvailable) {
        Logger::log("[SENSOR] MPU6050 devre disi birakildi.");
        _buf[0].valid = false;
        _buf[0].health = SensorHealth::Invalid;
        _buf[1].valid = false;
        _buf[1].health = SensorHealth::Invalid;
        return;
    }

    // MPU6050 uyandır
    _mpu_write_reg(imu.powerReg, imu.powerWakeValue);
    delay(10);

    // ±8g ivme
    _mpu_write_reg(imu.accelConfigReg, imu.accelConfigValue);
    // ±500°/s jiroskop
    _mpu_write_reg(imu.gyroConfigReg, imu.gyroConfigValue);
    // DLPF: 21Hz bant genişliği
    _mpu_write_reg(imu.dlpfReg, imu.dlpfValue);

    Logger::log("[SENSOR] MPU6050 (ham I2C+DMA) hazir.");

    RP2350I2C* rpBus = _rpBus();
    _dmaFastPath = rpBus && _dmaBus.configureMpu(*rpBus);
    if (!_dmaFastPath) {
        _setFault(SensorFaultCode::DmaChannelClaimFailed);
        Logger::log("[SENSOR] MPU6050 DMA kullanilamiyor, HAL I2C fallback aktif.");
    }

    #ifdef USE_GY87
        if (!_dmaFastPath || !_dmaBus.configureAuxRx(*rpBus)) {
            _setFault(SensorFaultCode::DmaChannelClaimFailed);
            Logger::log("[SENSOR] Yardimci I2C DMA kanali alinamadi.");
            _hasMag = false;
            _hasBaro = false;
        } else {
            _auxBus.configure(_dmaBus, *rpBus, _baroDriver, _hasMag, _hasBaro, _faultCode);
            if (!_hasMag) Logger::log("[SENSOR] HMC5883L bulunamadi!");
            else          Logger::log("[SENSOR] HMC5883L hazir.");

            if (!_hasBaro) Logger::log("[SENSOR] BMP085 bulunamadi!");
            else           Logger::log("[SENSOR] BMP085 hazir.");
        }
    #endif

    // İlk DMA okumayı başlat
    if (_dmaFastPath) {
        _mpu_start_dma_read();
    }
}

void SensorManager::_mpu_start_dma_read() {
    RP2350I2C* rpBus = _rpBus();
    if (!_dmaFastPath || !rpBus || !_dmaBus.hasMpuChannels()) {
        _setFault(SensorFaultCode::DmaChannelClaimFailed);
        return;
    }

    const ImuDeviceProfile& imu = *_imuProfile;
    _dmaBus.startMpuRead(*rpBus, imu.address, _reg_addr, micros());
}

void SensorManager::_observeCalibrationRawFrame(const uint8_t raw[GyroAccelDriver::RAW_LEN]) {
    if (_imuCalibrationState != ImuCalibrationState::Collecting) {
        return;
    }
    auto raw16 = [](uint8_t hi, uint8_t lo) -> int16_t {
        return (int16_t)((hi << 8) | lo);
    };
    _calibration.observeMpuRaw({
        raw16(raw[0], raw[1]),
        raw16(raw[2], raw[3]),
        raw16(raw[4], raw[5]),
        raw16(raw[8], raw[9]),
        raw16(raw[10], raw[11]),
        raw16(raw[12], raw[13]),
        raw16(raw[6], raw[7])
    });
    if (++_imuCalibrationSamples >= BOOT_CAL_SAMPLES) {
        _finishAsyncImuCalibration();
    }
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

bool SensorManager::_mpu_dma_ready() {
    return _dmaFastPath && _dmaBus.isMpuReady();
}

void SensorManager::update() {
    if (!_imuAvailable) {
        return;
    }

    if (!_dmaFastPath) {
        uint8_t raw[GyroAccelDriver::RAW_LEN];
        if (!_readRawFrame(raw)) {
            return;
        }
        uint8_t writeIdx = 1 - _writeIdx;
        SensorBuffer& buf = _buf[writeIdx];
        _gyroAccelDriver.parseRawSample(raw, _imuCalibration, buf, micros());
        _observeCalibrationRawFrame(raw);
        buf.baroValid = false;
        buf.pressureHpa = 0.0f;
        mutex_enter_blocking(&_mutex);
        _writeIdx = writeIdx;
        mutex_exit(&_mutex);
        return;
    }

    // DMA tamamlandıysa parse et, yeni okuma başlat
    if (!_mpu_dma_ready()) {
        const uint32_t nowUs = micros();
        if (_dmaBus.mpuTimedOut(nowUs, I2C_DMA_TIMEOUT_US)) {
            _dmaBus.abortMpu();
            _setFault(SensorFaultCode::DmaTransferTimeout);

            uint8_t writeIdx = 1 - _writeIdx;
            _buf[writeIdx].valid = false;
            _buf[writeIdx].health = SensorHealth::Timeout;
            _buf[writeIdx].timestamp = nowUs;
            mutex_enter_blocking(&_mutex);
            _writeIdx = writeIdx;
            mutex_exit(&_mutex);

            _mpu_start_dma_read();
        }
        return;
    }

    uint8_t writeIdx = 1 - _writeIdx;
    SensorBuffer& buf = _buf[writeIdx];

    const uint8_t* raw = _dmaBus.mpuBuffer();
    _gyroAccelDriver.parseRawSample(raw, _imuCalibration, buf, micros());
    _observeCalibrationRawFrame(raw);
    buf.baroValid = false;
    buf.pressureHpa = 0.0f;

    #ifdef USE_GY87
        if (RP2350I2C* rpBus = _rpBus()) {
            _auxBus.update(_dmaBus, *rpBus, _magDriver, _baroDriver, buf, _faultCode);
        }
    #endif

    mutex_enter_blocking(&_mutex);
    _writeIdx = writeIdx;
    mutex_exit(&_mutex);

    // Sonraki okumayı hemen başlat — CPU beklemez
    _mpu_start_dma_read();
}

SensorBuffer SensorManager::getLatest() {
    mutex_enter_blocking(&_mutex);
    SensorBuffer copy = _buf[_writeIdx];
    mutex_exit(&_mutex);

    SensorQuality quality = _healthMonitor.evaluateQuality(
        _imuAvailable,
        copy.valid,
        copy.timestamp,
        micros(),
        SENSOR_STALE_TIMEOUT_US
    );
    copy.health = quality.health;
    copy.qualityScore = quality.score;
    copy.sampleAgeUs = quality.ageUs;
    if (copy.health != SensorHealth::Ok) {
        copy.valid = false;
    }

    return copy;
}

#ifdef USE_GY87
bool SensorManager::hasMag() const { return _hasMag; }
bool SensorManager::hasBaro() const { return _hasBaro; }
#else
bool SensorManager::hasMag() const { return false; }
bool SensorManager::hasBaro() const { return false; }
#endif
