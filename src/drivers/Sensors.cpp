#include "Sensors.h"

// Pico SDK i2c instance — Wire'ın altındaki donanım
// Wire → i2c0 (SDA=4, SCL=5 config.h'da tanımlı)
static i2c_inst_t* _i2c = i2c0;

void SensorManager::_mpu_write_reg(uint8_t reg, uint8_t val) {
    uint8_t buf[2] = {reg, val};
    i2c_write_blocking(_i2c, MPU6050_ADDR, buf, 2, false);
}

void SensorManager::init() {
    mutex_init(&_mutex);
    _buf[0] = {};
    _buf[1] = {};

    // I2C başlat — Wire yerine doğrudan SDK
    i2c_init(_i2c, 400000);
    gpio_set_function(PIN_SDA, GPIO_FUNC_I2C);
    gpio_set_function(PIN_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(PIN_SDA);
    gpio_pull_up(PIN_SCL);

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
        // HMC5883L ve BMP085 native SDK I2C üzerinden asenkron okunacak
        _i2c_dma_chan = dma_claim_unused_channel(true);
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

    while (dma_channel_is_busy(_i2c_dma_chan)) {
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

    // Ham ivme
    float ax_raw = raw_ax * ACCEL_SCALE;
    float ay_raw = raw_ay * ACCEL_SCALE;
    float az_raw = raw_az * ACCEL_SCALE;

    // IIR Low-Pass Filter: y[n] = alpha * x[n] + (1 - alpha) * y[n-1]
    _ax_f = IIR_ALPHA * ax_raw + (1.0f - IIR_ALPHA) * _ax_f;
    _ay_f = IIR_ALPHA * ay_raw + (1.0f - IIR_ALPHA) * _ay_f;
    _az_f = IIR_ALPHA * az_raw + (1.0f - IIR_ALPHA) * _az_f;

    buf.ax = _ax_f;
    buf.ay = _ay_f;
    buf.az = _az_f;

    // Jiroskop filtrelenmez — Madgwick zaten entegrasyon yapıyor
    buf.gx = raw_gx * GYRO_SCALE;
    buf.gy = raw_gy * GYRO_SCALE;
    buf.gz = raw_gz * GYRO_SCALE;

    // Sıcaklık: MPU6050 datasheet formülü
    buf.tempC = (float)raw_t / 340.0f + 36.53f;

    buf.timestamp = micros();
    buf.valid = true;
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
                buf.mx = mx * 0.92f;
                buf.my = my * 0.92f;
                buf.mz = mz * 0.92f;
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
    return copy;
}

#ifdef USE_GY87
bool SensorManager::hasMag() const { return _hasMag; }
bool SensorManager::hasBaro() const { return _hasBaro; }
#else
bool SensorManager::hasMag() const { return false; }
bool SensorManager::hasBaro() const { return false; }
#endif