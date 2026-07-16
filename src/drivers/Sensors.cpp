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

SensorBusProbeSnapshot SensorManager::busProbeSnapshot() const {
    return _busProbe;
}

static const char* sensorBusRoleText(SensorBusDeviceRole role) {
    switch (role) {
        case SensorBusDeviceRole::Imu: return "IMU";
        case SensorBusDeviceRole::Baro: return "BARO";
        case SensorBusDeviceRole::Mag: return "MAG";
        case SensorBusDeviceRole::UnsupportedMag: return "MAG?";
        default: return "UNK";
    }
}

static void appendProbeList(char* dest,
                            size_t len,
                            const char* prefix,
                            const SensorBusDevice* devices,
                            uint8_t count,
                            bool overflow) {
    if (!dest || len == 0) {
        return;
    }
    auto boundedLen = [](const char* text, size_t maxLen) -> size_t {
        size_t n = 0;
        while (n < maxLen && text[n] != '\0') {
            ++n;
        }
        return n;
    };

    size_t used = boundedLen(dest, len);
    if (used >= len) {
        return;
    }
    int written = snprintf(dest + used, len - used, "%s", prefix);
    if (written < 0) {
        return;
    }
    used = boundedLen(dest, len);
    for (uint8_t i = 0; i < count && used < len; ++i) {
        written = snprintf(dest + used, len - used, "0x%02X/%s%s",
                           devices[i].address,
                           sensorBusRoleText(devices[i].role),
                           i + 1 == count ? "" : ",");
        if (written < 0) {
            return;
        }
        used = boundedLen(dest, len);
    }
    if (overflow && used < len) {
        snprintf(dest + used, len - used, "+");
    }
}

void SensorManager::formatBusProbeSummary(char* dest, size_t len) const {
    if (!dest || len == 0) {
        return;
    }
    dest[0] = '\0';
    appendProbeList(dest, len, "I2C:", _busProbe.mainDevices, _busProbe.mainCount, _busProbe.mainOverflow);
    if (_busProbe.bypassEnabled) {
        appendProbeList(dest, len, " AUX:", _busProbe.auxDevices, _busProbe.auxCount, _busProbe.auxOverflow);
    }
}

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
        case SensorFaultCode::MagUnsupported: return "Unsupported magnetometer detected";
        case SensorFaultCode::BaroReadFailed: return "Baro read failed";
        default: return "Unknown sensor fault";
    }
}

void SensorManager::init() {
    mutex_init(&_mutex);
    _buf[0] = {};
    _buf[1] = {};
    _buf[0].health = SensorHealth::WarmingUp;
    _buf[1].health = SensorHealth::WarmingUp;
    _gyroAccelDriver.resetFilters();

    _bus().init(PIN_SDA, PIN_SCL, 400000);
    _busProbe = SensorBusProbe::scanMain(_bus());

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
    // GY-87 manyetometresi MPU6050 aux I2C hattindadir; bypass acik olmadan
    // HMC/QMC adaylari ana I2C bus'ta gorunmez.
    _mpu_write_reg(0x6A, 0x00);
    _mpu_write_reg(0x37, 0x02);
    SensorBusProbe::scanAux(_bus(), _busProbe);

    char probeLine[160];
    formatBusProbeSummary(probeLine, sizeof(probeLine));
    if (probeLine[0] != '\0') {
        char logLine[180];
        snprintf(logLine, sizeof(logLine), "[SENSOR] %s", probeLine);
        Logger::log(logLine);
    }

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
            if (_hasMag) {
                Logger::log("[SENSOR] HMC5883L hazir.");
            } else if (_auxBus.hasUnsupportedMag()) {
                char line[96];
                snprintf(line, sizeof(line),
                         "[SENSOR] Desteklenmeyen manyetometre algilandi: 0x%02X (6DOF fallback)",
                         _auxBus.unsupportedMagAddress());
                Logger::log(line);
            } else {
                Logger::log("[SENSOR] HMC5883L bulunamadi! 6DOF fallback aktif.");
            }

            if (!_hasBaro) Logger::log("[SENSOR] BMP085 bulunamadi!");
            else           Logger::log("[SENSOR] BMP085 hazir.");
        }
    #endif

    // İlk DMA okumayı başlat
    if (_dmaFastPath) {
        _mpu_start_dma_read();
    }
}

void SensorManager::initBenchDisabled() {
    mutex_init(&_mutex);
    _imuAvailable = false;
    _hasMag = false;
    _hasBaro = false;
    _dmaFastPath = false;
    _faultCode = SensorFaultCode::None;
    _lastWhoAmI = 0;
    _busProbe = {};
    _buf[0] = {};
    _buf[1] = {};
    _buf[0].valid = false;
    _buf[1].valid = false;
    _buf[0].health = SensorHealth::Invalid;
    _buf[1].health = SensorHealth::Invalid;
    _buf[0].timestamp = micros();
    _buf[1].timestamp = _buf[0].timestamp;
    _writeIdx = 0;
    Logger::log("[SENSOR] Bench mode: sensor init skipped.");
}

void SensorManager::_mpu_start_dma_read() {
    RP2350I2C* rpBus = _rpBus();
    if (!_dmaFastPath || !rpBus || !_dmaBus.hasMpuChannels()) {
        _setFault(SensorFaultCode::DmaChannelClaimFailed);
        return;
    }

    const ImuDeviceProfile& imu = *_imuProfile;
    if (!_dmaBus.startMpuRead(*rpBus, imu.address, _reg_addr, micros())) {
        _setFault(SensorFaultCode::DmaTransferTimeout);
    }
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
    _dmaBus.finishMpu();
    _gyroAccelDriver.parseRawSample(raw, _imuCalibration, buf, micros());
    _observeCalibrationRawFrame(raw);
    buf.baroValid = false;
    buf.pressureHpa = 0.0f;

    #ifdef USE_GY87
        if (RP2350I2C* rpBus = _rpBus()) {
            const SensorFaultCode previousFault = _faultCode;
            _auxBus.update(_dmaBus, *rpBus, _magDriver, _baroDriver, buf, _faultCode);
            if (_faultCode == SensorFaultCode::AuxDmaTransferTimeout && buf.baroValid) {
                _faultCode = previousFault == SensorFaultCode::AuxDmaTransferTimeout
                    ? SensorFaultCode::None
                    : previousFault;
            }
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
bool SensorManager::hasUnsupportedMag() const { return _auxBus.hasUnsupportedMag(); }
uint8_t SensorManager::unsupportedMagAddress() const { return _auxBus.unsupportedMagAddress(); }
#else
bool SensorManager::hasMag() const { return false; }
bool SensorManager::hasBaro() const { return false; }
bool SensorManager::hasUnsupportedMag() const { return false; }
uint8_t SensorManager::unsupportedMagAddress() const { return 0; }
#endif
