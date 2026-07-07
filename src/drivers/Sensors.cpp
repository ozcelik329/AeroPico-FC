#include "Sensors.h"

// Pico SDK i2c instance — Wire'ın altındaki donanım
// Wire → i2c0 (SDA=4, SCL=5 config.h'da tanımlı)
static i2c_inst_t* _i2c = i2c0;

void SensorManager::_mpu_write_reg(uint8_t reg, uint8_t val) {
    uint8_t buf[2] = {reg, val};
    i2c_write_blocking(_i2c, MPU6050_ADDR, buf, 2, false);
}

bool SensorManager::_readWhoAmI(uint8_t& whoami) {
    uint8_t reg = MPU6050_REG_WHOAMI;
    if (i2c_write_blocking(_i2c, MPU6050_ADDR, &reg, 1, true) != 1) return false;
    if (i2c_read_blocking(_i2c, MPU6050_ADDR, &whoami, 1, false) != 1) return false;
    return true;
}

bool SensorManager::_readRawSample(int16_t& raw_ax, int16_t& raw_ay, int16_t& raw_az,
                                   int16_t& raw_gx, int16_t& raw_gy, int16_t& raw_gz) {
    uint8_t reg = MPU6050_REG_ACCEL;
    uint8_t raw[14];
    if (i2c_write_blocking(_i2c, MPU6050_ADDR, &reg, 1, true) != 1) return false;
    if (i2c_read_blocking(_i2c, MPU6050_ADDR, raw, sizeof(raw), false) != (int)sizeof(raw)) return false;

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

bool SensorManager::isImuAvailable() const { return _imuAvailable; }
bool SensorManager::isDmaOk() const { return _dma_chan >= 0 && _mpu_tx_dma_chan >= 0; }

bool SensorManager::runBootCalibration() {
    if (!_imuAvailable) return false;

    Serial.println("[SENSOR] Boot kalibrasyonu basliyor (uçak sabit olmali)...");
    float sumGx = 0.0f, sumGy = 0.0f, sumGz = 0.0f;
    float sumAx = 0.0f, sumAy = 0.0f, sumAz = 0.0f;
    int validSamples = 0;

    for (int i = 0; i < BOOT_CAL_SAMPLES; i++) {
        int16_t rax, ray, raz, rgx, rgy, rgz;
        if (!_readRawSample(rax, ray, raz, rgx, rgy, rgz)) {
            delay(2);
            continue;
        }
        sumGx += rgx * GYRO_SCALE;
        sumGy += rgy * GYRO_SCALE;
        sumGz += rgz * GYRO_SCALE;
        sumAx += rax * ACCEL_SCALE;
        sumAy += ray * ACCEL_SCALE;
        sumAz += raz * ACCEL_SCALE;
        validSamples++;
        delay(2);
    }

    if (validSamples < BOOT_CAL_SAMPLES / 2) {
        Serial.println("[SENSOR] Boot kalibrasyonu yetersiz ornek!");
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

    Serial.printf("[SENSOR] Gyro bias: %.3f, %.3f, %.3f deg/s\n",
                  _gyroBiasX, _gyroBiasY, _gyroBiasZ);
    Serial.printf("[SENSOR] Accel bias: %.4f, %.4f, %.4f g\n",
                  _accelBiasX, _accelBiasY, _accelBiasZ);
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
}

void SensorManager::beginMagCalibration() {
    _magCalCollecting = true;
    _magMinX = _magMinY = _magMinZ = 1000000.0f;
    _magMaxX = _magMaxY = _magMaxZ = -1000000.0f;
}

bool SensorManager::observeMagCalibrationSample(float mx, float my, float mz) {
    if (!_magCalCollecting) return false;
    _magMinX = min(_magMinX, mx);
    _magMinY = min(_magMinY, my);
    _magMinZ = min(_magMinZ, mz);
    _magMaxX = max(_magMaxX, mx);
    _magMaxY = max(_magMaxY, my);
    _magMaxZ = max(_magMaxZ, mz);
    return true;
}

MagCalibration SensorManager::finishMagCalibration() {
    _magCalCollecting = false;
    _magCalibration.hardIronX = (_magMinX + _magMaxX) * 0.5f;
    _magCalibration.hardIronY = (_magMinY + _magMaxY) * 0.5f;
    _magCalibration.hardIronZ = (_magMinZ + _magMaxZ) * 0.5f;
    _magCalibration.valid = true;
    return _magCalibration;
}

MagCalibration SensorManager::getMagCalibration() const {
    return _magCalibration;
}

void SensorManager::setMagCalibration(const MagCalibration& calibration) {
    if (!calibration.valid) return;
    _magCalibration = calibration;
}

void SensorManager::init() {
    mutex_init(&_mutex);
    _buf[0] = {};
    _buf[1] = {};
    _buf[0].health = SensorHealth::WarmingUp;
    _buf[1].health = SensorHealth::WarmingUp;

    // I2C başlat — Wire yerine doğrudan SDK
    i2c_init(_i2c, 400000);
    gpio_set_function(PIN_SDA, GPIO_FUNC_I2C);
    gpio_set_function(PIN_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(PIN_SDA);
    gpio_pull_up(PIN_SCL);

    uint8_t whoami = 0;
    if (!_readWhoAmI(whoami) || whoami != MPU6050_WHOAMI_VAL) {
        _imuAvailable = false;
        Serial.printf("[SENSOR] MPU6050 WHOAMI hatasi: 0x%02X (beklenen 0x%02X)\n",
                      whoami, MPU6050_WHOAMI_VAL);
    } else {
        _imuAvailable = true;
    }

    if (!_imuAvailable) {
        Serial.println("[SENSOR] MPU6050 devre disi birakildi.");
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

    Serial.println("[SENSOR] MPU6050 (ham I2C+DMA) hazir.");

    // DMA kanalları al: RX için bir, TX komutları için ayrı bir kanal
    _dma_chan = dma_claim_unused_channel(true);
    _mpu_tx_dma_chan = dma_claim_unused_channel(true);

    dma_channel_config rx_cfg = dma_channel_get_default_config(_dma_chan);
    channel_config_set_transfer_data_size(&rx_cfg, DMA_SIZE_8);
    channel_config_set_read_increment(&rx_cfg, false);   // I2C RX FIFO sabit adres
    channel_config_set_write_increment(&rx_cfg, true);   // buffer'a sırayla yaz
    channel_config_set_dreq(&rx_cfg, i2c_get_dreq(_i2c, false)); // I2C RX dreq

    dma_channel_configure(
        _dma_chan,
        &rx_cfg,
        _dma_buf,                          // hedef: RAM buffer
        &i2c_get_hw(_i2c)->data_cmd,       // kaynak: I2C RX FIFO
        MPU6050_RAW_LEN,
        false  // henüz başlatma
    );

    dma_channel_config tx_cfg = dma_channel_get_default_config(_mpu_tx_dma_chan);
    channel_config_set_transfer_data_size(&tx_cfg, DMA_SIZE_16);
    channel_config_set_read_increment(&tx_cfg, true);
    channel_config_set_write_increment(&tx_cfg, false);
    channel_config_set_dreq(&tx_cfg, i2c_get_dreq(_i2c, true)); // I2C TX dreq

    dma_channel_configure(
        _mpu_tx_dma_chan,
        &tx_cfg,
        _mpu_dma_cmd,
        &_i2c->hw->data_cmd,
        1 + MPU6050_RAW_LEN,
        false
    );

    #ifdef USE_GY87
        _i2c_dma_chan = dma_claim_unused_channel(true);
        dma_channel_config aux_cfg = dma_channel_get_default_config(_i2c_dma_chan);
        channel_config_set_transfer_data_size(&aux_cfg, DMA_SIZE_8);
        channel_config_set_read_increment(&aux_cfg, false);
        channel_config_set_write_increment(&aux_cfg, true);
        channel_config_set_dreq(&aux_cfg, i2c_get_dreq(_i2c, false));
        dma_channel_configure(
            _i2c_dma_chan,
            &aux_cfg,
            nullptr,
            &i2c_get_hw(_i2c)->data_cmd,
            0,
            false
        );

        _hasMag = _initMag();
        if (!_hasMag) Serial.println("[SENSOR] HMC5883L bulunamadi!");
        else          Serial.println("[SENSOR] HMC5883L hazir.");

        _hasBaro = _initBaro();
        if (!_hasBaro) Serial.println("[SENSOR] BMP085 bulunamadi!");
        else           Serial.println("[SENSOR] BMP085 hazir.");
    #endif

    // İlk DMA okumayı başlat
    _mpu_start_dma_read();
}

void SensorManager::_mpu_start_dma_read() {
    // Register adresini yaz, sonra okuma isteği gönder; tüm komutları DMA ile veriyoruz
    _mpu_dma_cmd[0] = (uint16_t)(_reg_addr & 0xFF);
    for (int i = 0; i < MPU6050_RAW_LEN; i++) {
        _mpu_dma_cmd[1 + i] = I2C_IC_DATA_CMD_CMD_BITS | (uint16_t)(i == MPU6050_RAW_LEN - 1 ? I2C_IC_DATA_CMD_STOP_BITS : 0);
    }

    dma_channel_set_write_addr(_dma_chan, _dma_buf, false);
    dma_channel_set_trans_count(_dma_chan, MPU6050_RAW_LEN, false);

    dma_channel_set_write_addr(_mpu_tx_dma_chan, &_i2c->hw->data_cmd, false);
    dma_channel_set_trans_count(_mpu_tx_dma_chan, 1 + MPU6050_RAW_LEN, false);

    _i2c->hw->tar = MPU6050_ADDR;
    _i2c->hw->dma_cr = 1 << 1 | 1; // TDMAE + RDMAE

    dma_channel_start(_dma_chan);
    dma_channel_start(_mpu_tx_dma_chan);
}

#ifdef USE_GY87
bool SensorManager::_i2c_write_reg(uint8_t slave_addr, uint8_t reg, uint8_t val) {
    uint8_t buf[2] = {reg, val};
    int written = i2c_write_blocking(_i2c, slave_addr, buf, 2, false);
    return written == 2;
}

bool SensorManager::_i2c_read_regs_dma(uint8_t slave_addr, uint8_t reg, uint8_t* dest, size_t len) {
    if (!_i2c_write_reg(slave_addr, reg, 0)) return false;

    dma_channel_set_write_addr(_i2c_dma_chan, dest, false);
    dma_channel_set_trans_count(_i2c_dma_chan, len, false);
    dma_channel_start(_i2c_dma_chan);

    uint32_t startUs = micros();
    while (dma_channel_is_busy(_i2c_dma_chan)) {
        if ((uint32_t)(micros() - startUs) > I2C_DMA_TIMEOUT_US) {
            dma_channel_abort(_i2c_dma_chan);
            return false;
        }
        tight_loop_contents();
    }

    return true;
}

bool SensorManager::_initMag() {
    bool ok = true;
    ok &= _i2c_write_reg(HMC5883L_ADDR, HMC5883L_REG_CONFIG_A, 0x70);
    ok &= _i2c_write_reg(HMC5883L_ADDR, HMC5883L_REG_CONFIG_B, 0xA0);
    ok &= _i2c_write_reg(HMC5883L_ADDR, HMC5883L_REG_MODE, 0x00);
    return ok;
}

bool SensorManager::_initBaro() {
    // Read BMP085 calibration data
    uint8_t calib[22];
    if (!_i2c_read_regs_dma(BMP085_ADDR, BMP085_REG_CALIB_START, calib, sizeof(calib))) {
        return false;
    }

    auto to_int16 = [](uint8_t hi, uint8_t lo) { return (int16_t)((hi << 8) | lo); };
    auto to_uint16 = [](uint8_t hi, uint8_t lo) { return (uint16_t)((hi << 8) | lo); };

    AC1 = to_int16(calib[0], calib[1]);
    AC2 = to_int16(calib[2], calib[3]);
    AC3 = to_int16(calib[4], calib[5]);
    AC4 = to_uint16(calib[6], calib[7]);
    AC5 = to_uint16(calib[8], calib[9]);
    AC6 = to_uint16(calib[10], calib[11]);
    B1  = to_int16(calib[12], calib[13]);
    B2  = to_int16(calib[14], calib[15]);
    MB  = to_int16(calib[16], calib[17]);
    MC  = to_int16(calib[18], calib[19]);
    MD  = to_int16(calib[20], calib[21]);

    _bmp_state = BMP_IDLE;
    return true;
}

bool SensorManager::_readMagAsync() {
    return _i2c_read_regs_dma(HMC5883L_ADDR, HMC5883L_REG_DATA_X_MSB, _hmc_dma_buf, 6);
}

bool SensorManager::_readBaroAsync(SensorBuffer& buf) {
    if (_bmp_state == BMP_IDLE) {
        if (!_i2c_write_reg(BMP085_ADDR, BMP085_REG_CONTROL, BMP085_CMD_TEMP)) return false;
        _bmp_wait_until = micros() + 5000;
        _bmp_state = BMP_TEMP_PENDING;
        return false;
    }

    if (_bmp_state == BMP_TEMP_PENDING) {
        if ((int32_t)(micros() - _bmp_wait_until) < 0) return false;
        if (!_i2c_read_regs_dma(BMP085_ADDR, BMP085_REG_RESULT, _bmp_dma_buf, 2)) return false;
        _bmp_raw_temp = (int32_t)(_bmp_dma_buf[0] << 8) | _bmp_dma_buf[1];
        _bmp_state = BMP_TEMP_READ;
    }

    if (_bmp_state == BMP_TEMP_READ) {
        if (!_i2c_write_reg(BMP085_ADDR, BMP085_REG_CONTROL, BMP085_CMD_PRESSURE + (3 << 6))) return false;
        _bmp_wait_until = micros() + 26000;
        _bmp_state = BMP_PRESSURE_PENDING;
        return false;
    }

    if (_bmp_state == BMP_PRESSURE_PENDING) {
        if ((int32_t)(micros() - _bmp_wait_until) < 0) return false;
        if (!_i2c_read_regs_dma(BMP085_ADDR, BMP085_REG_RESULT, _bmp_dma_buf, 3)) return false;
        int32_t up = ((int32_t)_bmp_dma_buf[0] << 16 | (int32_t)_bmp_dma_buf[1] << 8 | _bmp_dma_buf[2]) >> (8 - 3);
        int32_t x1 = ((int32_t)_bmp_raw_temp - AC6) * AC5 >> 15;
        int32_t x2 = ((int32_t)MC << 11) / (x1 + MD);
        int32_t b5 = x1 + x2;
        int32_t t = (b5 + 8) >> 4;
        int32_t b6 = b5 - 4000;
        int32_t x1p = (B2 * (b6 * b6 >> 12)) >> 11;
        int32_t x2p = (AC2 * b6) >> 11;
        int32_t x3 = x1p + x2p;
        int32_t b3 = (((int32_t)AC1 * 4 + x3) + 2) >> 2;
        int32_t x1pp = (AC3 * b6) >> 13;
        int32_t x2pp = (B1 * ((b6 * b6) >> 12)) >> 16;
        int32_t x3p = ((x1pp + x2pp) + 2) >> 2;
        int32_t b4 = (AC4 * (uint32_t)(x1pp + x3p + 32768)) >> 15;
        int32_t b7 = ((uint32_t)up - b3) * (50000 >> 3);
        int32_t p = (b7 < 0) ? (b7 * 2) / b4 : (b7 / b4) * 2;
        int32_t x1ppp = (p >> 8) * (p >> 8);
        int32_t x1pppp = (x1ppp * 3038) >> 16;
        int32_t x2pppp = (-7357 * p) >> 16;
        int32_t pressure = p + ((x1pppp + x2pppp + 3791) >> 4);
        buf.pressure = pressure / 100.0f;
        buf.tempC = t / 10.0f;
        _bmp_state = BMP_IDLE;
        return true;
    }

    return false;
}
#endif

bool SensorManager::_mpu_dma_ready() {
    return !dma_channel_is_busy(_dma_chan);
}

void __not_in_flash_func(SensorManager::_mpu_parse)(SensorBuffer& buf) {
    auto to_int16 = [](uint8_t hi, uint8_t lo) -> int16_t {
        return (int16_t)((hi << 8) | lo);
    };

    int16_t raw_ax = to_int16(_dma_buf[0],  _dma_buf[1]);
    int16_t raw_ay = to_int16(_dma_buf[2],  _dma_buf[3]);
    int16_t raw_az = to_int16(_dma_buf[4],  _dma_buf[5]);
    int16_t raw_t  = to_int16(_dma_buf[6],  _dma_buf[7]);  // sıcaklık
    int16_t raw_gx = to_int16(_dma_buf[8],  _dma_buf[9]);
    int16_t raw_gy = to_int16(_dma_buf[10], _dma_buf[11]);
    int16_t raw_gz = to_int16(_dma_buf[12], _dma_buf[13]);

    float gx_raw = raw_gx * GYRO_SCALE - _gyroBiasX;
    float gy_raw = raw_gy * GYRO_SCALE - _gyroBiasY;
    float gz_raw = raw_gz * GYRO_SCALE - _gyroBiasZ;

    // Küçük median penceresi tek örnek sıçramalarını bastırır.
    buf.gx = _gxMedian.update(gx_raw);
    buf.gy = _gyMedian.update(gy_raw);
    buf.gz = _gzMedian.update(gz_raw);

    // IIR Low-Pass Filter: y[n] = alpha * x[n] + (1 - alpha) * y[n-1]
    float ax_raw = raw_ax * ACCEL_SCALE - _accelBiasX;
    float ay_raw = raw_ay * ACCEL_SCALE - _accelBiasY;
    float az_raw = raw_az * ACCEL_SCALE - _accelBiasZ;
    ax_raw = _axMedian.update(ax_raw);
    ay_raw = _ayMedian.update(ay_raw);
    az_raw = _azMedian.update(az_raw);

    _ax_f = IIR_ALPHA * ax_raw + (1.0f - IIR_ALPHA) * _ax_f;
    _ay_f = IIR_ALPHA * ay_raw + (1.0f - IIR_ALPHA) * _ay_f;
    _az_f = IIR_ALPHA * az_raw + (1.0f - IIR_ALPHA) * _az_f;

    buf.ax = _ax_f;
    buf.ay = _ay_f;
    buf.az = _az_f;

    // Sıcaklık: MPU6050 datasheet formülü
    buf.tempC = (float)raw_t / 340.0f + 36.53f;

    buf.timestamp = micros();
    buf.valid = true;
    buf.health = SensorHealth::Ok;

#if SENSOR_DEBUG_LOG_ENABLED
    uint32_t nowMs = millis();
    if (nowMs - _lastSensorDebugLogMs >= SENSOR_DEBUG_LOG_INTERVAL_MS) {
        _lastSensorDebugLogMs = nowMs;
        Serial.printf(
            "[SENSOR_FILTER] acc_raw=%.4f,%.4f,%.4f acc_f=%.4f,%.4f,%.4f gyro_raw=%.3f,%.3f,%.3f gyro_f=%.3f,%.3f,%.3f\n",
            raw_ax * ACCEL_SCALE - _accelBiasX,
            raw_ay * ACCEL_SCALE - _accelBiasY,
            raw_az * ACCEL_SCALE - _accelBiasZ,
            buf.ax,
            buf.ay,
            buf.az,
            gx_raw,
            gy_raw,
            gz_raw,
            buf.gx,
            buf.gy,
            buf.gz
        );
    }
#endif
}

void SensorManager::update() {
    // DMA tamamlandıysa parse et, yeni okuma başlat
    if (!_mpu_dma_ready()) return;  // henüz hazır değil, bekle

    uint8_t writeIdx = 1 - _writeIdx;
    SensorBuffer& buf = _buf[writeIdx];

    _mpu_parse(buf);

    #ifdef USE_GY87
        if (_hasMag) {
            if (!_readMagAsync()) {
                buf.mx = buf.my = buf.mz = 0.0f;
            } else {
                int16_t mx = (int16_t)(_hmc_dma_buf[0] << 8 | _hmc_dma_buf[1]);
                int16_t my = (int16_t)(_hmc_dma_buf[4] << 8 | _hmc_dma_buf[5]);
                int16_t mz = (int16_t)(_hmc_dma_buf[2] << 8 | _hmc_dma_buf[3]);
                float mxScaled = mx * 0.92f;
                float myScaled = my * 0.92f;
                float mzScaled = mz * 0.92f;
                observeMagCalibrationSample(mxScaled, myScaled, mzScaled);
                buf.mx = mxScaled - (_magCalibration.valid ? _magCalibration.hardIronX : 0.0f);
                buf.my = myScaled - (_magCalibration.valid ? _magCalibration.hardIronY : 0.0f);
                buf.mz = mzScaled - (_magCalibration.valid ? _magCalibration.hardIronZ : 0.0f);
            }
        } else {
            buf.mx = buf.my = buf.mz = 0.0f;
        }

        if (_hasBaro) {
            if (!_readBaroAsync(buf)) {
                buf.pressure = 1013.25f;
            }
        } else {
            buf.pressure = 1013.25f;
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

    if (!copy.valid) {
        copy.health = _imuAvailable ? SensorHealth::WarmingUp : SensorHealth::Invalid;
    } else if ((uint32_t)(micros() - copy.timestamp) > SENSOR_STALE_TIMEOUT_US) {
        copy.health = SensorHealth::Stale;
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
