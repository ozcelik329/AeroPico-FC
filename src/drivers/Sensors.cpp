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

    // DMA kanalı al
    _dma_chan = dma_claim_unused_channel(true);

    dma_channel_config cfg = dma_channel_get_default_config(_dma_chan);
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_8);
    channel_config_set_read_increment(&cfg, false);   // I2C RX FIFO sabit adres
    channel_config_set_write_increment(&cfg, true);   // buffer'a sırayla yaz
    channel_config_set_dreq(&cfg, i2c_get_dreq(_i2c, false)); // I2C RX dreq

    dma_channel_configure(
        _dma_chan,
        &cfg,
        _dma_buf,                          // hedef: RAM buffer
        &i2c_get_hw(_i2c)->data_cmd,       // kaynak: I2C RX FIFO
        MPU6050_RAW_LEN,
        false  // henüz başlatma
    );

    #ifdef USE_GY87
        // HMC5883L ve BMP085 Wire üzerinden — DMA değil
        Wire.begin();
        hasMag = mag.begin();
        if (!hasMag) Serial.println("[SENSOR] HMC5883L bulunamadi!");
        else         Serial.println("[SENSOR] HMC5883L hazir.");

        hasBaro = bmp.begin();
        if (!hasBaro) Serial.println("[SENSOR] BMP085 bulunamadi!");
        else          Serial.println("[SENSOR] BMP085 hazir.");
    #endif

    // İlk DMA okumayı başlat
    _mpu_start_dma_read();
}

void SensorManager::_mpu_start_dma_read() {
    // Register adresini yaz, sonra okuma isteği gönder
    i2c_write_blocking(_i2c, MPU6050_ADDR, &_reg_addr, 1, true); // repeated start

    // DMA'yı yeniden tetikle
    dma_channel_set_write_addr(_dma_chan, _dma_buf, false);
    dma_channel_set_trans_count(_dma_chan, MPU6050_RAW_LEN, false);

    // I2C'ye okuma isteği gönder (DMA alacak)
    for (int i = 0; i < MPU6050_RAW_LEN; i++) {
        bool last = (i == MPU6050_RAW_LEN - 1);
        i2c_get_hw(_i2c)->data_cmd = I2C_IC_DATA_CMD_CMD_BITS | (last ? I2C_IC_DATA_CMD_STOP_BITS : 0);
    }

    dma_channel_start(_dma_chan);
}

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
        if (hasMag) {
            sensors_event_t mag_event;
            mag.getEvent(&mag_event);
            buf.mx = mag_event.magnetic.x;
            buf.my = mag_event.magnetic.y;
            buf.mz = mag_event.magnetic.z;
        } else {
            buf.mx = buf.my = buf.mz = 0.0f;
        }

        if (hasBaro) {
            sensors_event_t bmp_event;
            bmp.getEvent(&bmp_event);
            buf.pressure = bmp_event.pressure;
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