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
    if (!_bus().writeRegister(MPU6050_ADDR, reg, val)) {
        _setFault(SensorFaultCode::I2cRawWriteFailed);
    }
}

bool SensorManager::_readWhoAmI(uint8_t& whoami) {
    const uint8_t reg = MPU6050_REG_WHOAMI;
    if (!_bus().writeRaw(MPU6050_ADDR, &reg, 1, true)) {
        _setFault(SensorFaultCode::I2cWhoamiWriteFailed);
        return false;
    }
    if (!_bus().readRaw(MPU6050_ADDR, &whoami, 1, false)) {
        _setFault(SensorFaultCode::I2cWhoamiReadFailed);
        return false;
    }
    return true;
}

bool SensorManager::_readRawSample(int16_t& raw_ax, int16_t& raw_ay, int16_t& raw_az,
                                   int16_t& raw_gx, int16_t& raw_gy, int16_t& raw_gz) {
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
    raw_gx = to_int16(raw[8], raw[9]);
    raw_gy = to_int16(raw[10], raw[11]);
    raw_gz = to_int16(raw[12], raw[13]);
    return true;
}

bool SensorManager::_readRawFrame(uint8_t raw[GyroAccelDriver::RAW_LEN]) {
    uint8_t reg = MPU6050_REG_ACCEL;
    if (!_bus().writeRaw(MPU6050_ADDR, &reg, 1, true)) {
        _setFault(SensorFaultCode::I2cRawWriteFailed);
        return false;
    }
    if (!_bus().readRaw(MPU6050_ADDR, raw, GyroAccelDriver::RAW_LEN, false)) {
        _setFault(SensorFaultCode::I2cRawReadFailed);
        return false;
    }
    return true;
}

bool SensorManager::isImuAvailable() const { return _imuAvailable; }
bool SensorManager::isDmaOk() const { return _dmaBus.hasMpuChannels(); }

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
        case SensorFaultCode::MagReadFailed: return "Mag read failed";
        case SensorFaultCode::BaroReadFailed: return "Baro read failed";
        default: return "Unknown sensor fault";
    }
}

bool SensorManager::runBootCalibration() {
    if (!_imuAvailable) return false;

    Logger::log("[SENSOR] Boot kalibrasyonu basliyor (ucak sabit olmali)...");
    float sumGx = 0.0f, sumGy = 0.0f, sumGz = 0.0f;
    float sumAx = 0.0f, sumAy = 0.0f, sumAz = 0.0f;
    int validSamples = 0;

    for (int i = 0; i < BOOT_CAL_SAMPLES; i++) {
        int16_t rax, ray, raz, rgx, rgy, rgz;
        if (!_readRawSample(rax, ray, raz, rgx, rgy, rgz)) {
            delay(2);
            continue;
        }
        sumGx += rgx * GyroAccelDriver::GYRO_SCALE;
        sumGy += rgy * GyroAccelDriver::GYRO_SCALE;
        sumGz += rgz * GyroAccelDriver::GYRO_SCALE;
        sumAx += rax * GyroAccelDriver::ACCEL_SCALE;
        sumAy += ray * GyroAccelDriver::ACCEL_SCALE;
        sumAz += raz * GyroAccelDriver::ACCEL_SCALE;
        validSamples++;
        delay(2);
    }

    if (validSamples < BOOT_CAL_SAMPLES / 2) {
        Logger::log("[SENSOR] Boot kalibrasyonu yetersiz ornek!");
        return false;
    }

    _gyroBiasX = sumGx / validSamples;
    _gyroBiasY = sumGy / validSamples;
    _gyroBiasZ = sumGz / validSamples;
    _accelBiasX = sumAx / validSamples;
    _accelBiasY = sumAy / validSamples;
    // Z ekseninde ~1g yerçekimi kalır; düz uçak varsayımı
    _accelBiasZ = (sumAz / validSamples) - 1.0f;
    _imuCalibration = {
        _gyroBiasX, _gyroBiasY, _gyroBiasZ,
        _accelBiasX, _accelBiasY, _accelBiasZ,
        true
    };

    char line[96];
    snprintf(line, sizeof(line), "[SENSOR] Gyro bias: %.3f, %.3f, %.3f deg/s",
             _gyroBiasX, _gyroBiasY, _gyroBiasZ);
    Logger::log(line);
    snprintf(line, sizeof(line), "[SENSOR] Accel bias: %.4f, %.4f, %.4f g",
             _accelBiasX, _accelBiasY, _accelBiasZ);
    Logger::log(line);
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
    if (!_readWhoAmI(whoami) || whoami != MPU6050_WHOAMI_VAL) {
        _imuAvailable = false;
        if (whoami != MPU6050_WHOAMI_VAL) {
            _setFault(SensorFaultCode::WhoamiMismatch);
        }
        char line[80];
        snprintf(line, sizeof(line), "[SENSOR] MPU6050 WHOAMI hatasi: 0x%02X (beklenen 0x%02X)",
                 whoami, MPU6050_WHOAMI_VAL);
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
    _mpu_write_reg(MPU6050_REG_PWR, 0x00);
    delay(10);

    // ±8g ivme
    _mpu_write_reg(MPU6050_REG_ACCEL_CFG, 0x10);
    // ±500°/s jiroskop
    _mpu_write_reg(MPU6050_REG_GYRO_CFG, 0x08);
    // DLPF: 21Hz bant genişliği
    _mpu_write_reg(MPU6050_REG_DLPF, 0x04);

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

    _dmaBus.startMpuRead(*rpBus, MPU6050_ADDR, _reg_addr, micros());
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

    _gyroAccelDriver.parseRawSample(_dmaBus.mpuBuffer(), _imuCalibration, buf, micros());
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
