#include "SensorAuxBus.h"

#ifdef USE_GY87

static void setFaultIfNeeded(SensorFaultCode& faultCode, SensorFaultCode code) {
    if (code != SensorFaultCode::None) {
        faultCode = code;
    }
}

bool SensorAuxBus::configure(SensorDmaBus& dmaBus,
                             RP2350I2C& bus,
                             BaroDriver& baroDriver,
                             bool& hasMag,
                             bool& hasBaro,
                             SensorFaultCode& faultCode) {
    _hasMag = initMag(bus, faultCode);
    _hasBaro = initBaro(dmaBus, bus, baroDriver, faultCode);
    hasMag = _hasMag;
    hasBaro = _hasBaro;
    return _hasMag || _hasBaro;
}

void SensorAuxBus::update(SensorDmaBus& dmaBus,
                          RP2350I2C& bus,
                          MagDriver& magDriver,
                          BaroDriver& baroDriver,
                          SensorBuffer& buffer,
                          SensorFaultCode& faultCode) {
    if (_hasMag) {
        readMag(dmaBus, bus, magDriver, buffer, faultCode);
    } else {
        buffer.mx = buffer.my = buffer.mz = 0.0f;
    }

    if (_hasBaro) {
        if (!readBaro(dmaBus, bus, baroDriver, buffer, faultCode)) {
            buffer.baroValid = false;
            buffer.pressureHpa = 0.0f;
        }
    } else {
        buffer.baroValid = false;
        buffer.pressureHpa = 0.0f;
    }
}

bool SensorAuxBus::writeReg(RP2350I2C& bus,
                            uint8_t address,
                            uint8_t reg,
                            uint8_t value,
                            SensorFaultCode& faultCode) {
    if (!bus.writeRegister(address, reg, value)) {
        setFaultIfNeeded(faultCode, SensorFaultCode::AuxI2cWriteFailed);
        return false;
    }
    return true;
}

bool SensorAuxBus::readRegsDma(SensorDmaBus& dmaBus,
                               RP2350I2C& bus,
                               uint8_t address,
                               uint8_t reg,
                               uint8_t* dest,
                               size_t len,
                               SensorFaultCode& faultCode) {
    if (!dmaBus.hasAuxChannel()) {
        setFaultIfNeeded(faultCode, SensorFaultCode::DmaChannelClaimFailed);
        return false;
    }

    if (!dmaBus.readAuxRegistersDma(bus, address, reg, dest, len, I2C_DMA_TIMEOUT_US, micros)) {
        setFaultIfNeeded(faultCode, SensorFaultCode::AuxDmaTransferTimeout);
        return false;
    }

    return true;
}

bool SensorAuxBus::initMag(RP2350I2C& bus, SensorFaultCode& faultCode) {
    bool ok = true;
    ok &= writeReg(bus, HMC5883L_ADDR, HMC5883L_REG_CONFIG_A, 0x70, faultCode);
    ok &= writeReg(bus, HMC5883L_ADDR, HMC5883L_REG_CONFIG_B, 0xA0, faultCode);
    ok &= writeReg(bus, HMC5883L_ADDR, HMC5883L_REG_MODE, 0x00, faultCode);
    if (!ok) {
        setFaultIfNeeded(faultCode, SensorFaultCode::MagReadFailed);
    }
    return ok;
}

bool SensorAuxBus::initBaro(SensorDmaBus& dmaBus,
                            RP2350I2C& bus,
                            BaroDriver& baroDriver,
                            SensorFaultCode& faultCode) {
    uint8_t calib[22];
    if (!readRegsDma(dmaBus, bus, BMP085_ADDR, BMP085_REG_CALIB_START, calib, sizeof(calib), faultCode)) {
        setFaultIfNeeded(faultCode, SensorFaultCode::BaroReadFailed);
        return false;
    }

    baroDriver.loadCalibration(calib);
    _bmpState = BMP_IDLE;
    return true;
}

void SensorAuxBus::readMag(SensorDmaBus& dmaBus,
                           RP2350I2C& bus,
                           MagDriver& magDriver,
                           SensorBuffer& buffer,
                           SensorFaultCode& faultCode) {
    if (!readRegsDma(dmaBus, bus, HMC5883L_ADDR, HMC5883L_REG_DATA_X_MSB, _hmcDmaBuf, sizeof(_hmcDmaBuf), faultCode)) {
        buffer.mx = buffer.my = buffer.mz = 0.0f;
        setFaultIfNeeded(faultCode, SensorFaultCode::MagReadFailed);
        return;
    }

    const int16_t mx = (int16_t)(_hmcDmaBuf[0] << 8 | _hmcDmaBuf[1]);
    const int16_t my = (int16_t)(_hmcDmaBuf[4] << 8 | _hmcDmaBuf[5]);
    const int16_t mz = (int16_t)(_hmcDmaBuf[2] << 8 | _hmcDmaBuf[3]);
    magDriver.applySample(mx, my, mz, buffer);
}

bool SensorAuxBus::readBaro(SensorDmaBus& dmaBus,
                            RP2350I2C& bus,
                            BaroDriver& baroDriver,
                            SensorBuffer& buffer,
                            SensorFaultCode& faultCode) {
    if (_bmpState == BMP_IDLE) {
        if (!writeReg(bus, BMP085_ADDR, BMP085_REG_CONTROL, BMP085_CMD_TEMP, faultCode)) {
            setFaultIfNeeded(faultCode, SensorFaultCode::BaroReadFailed);
            return false;
        }
        _bmpWaitUntilUs = micros() + 5000;
        _bmpState = BMP_TEMP_PENDING;
        return false;
    }

    if (_bmpState == BMP_TEMP_PENDING) {
        if ((int32_t)(micros() - _bmpWaitUntilUs) < 0) return false;
        if (!readRegsDma(dmaBus, bus, BMP085_ADDR, BMP085_REG_RESULT, _bmpDmaBuf, 2, faultCode)) {
            setFaultIfNeeded(faultCode, SensorFaultCode::BaroReadFailed);
            return false;
        }
        baroDriver.setRawTemperature((int32_t)(_bmpDmaBuf[0] << 8) | _bmpDmaBuf[1]);
        _bmpState = BMP_TEMP_READ;
    }

    if (_bmpState == BMP_TEMP_READ) {
        if (!writeReg(bus, BMP085_ADDR, BMP085_REG_CONTROL, BMP085_CMD_PRESSURE + (3 << 6), faultCode)) {
            setFaultIfNeeded(faultCode, SensorFaultCode::BaroReadFailed);
            return false;
        }
        _bmpWaitUntilUs = micros() + 26000;
        _bmpState = BMP_PRESSURE_PENDING;
        return false;
    }

    if (_bmpState == BMP_PRESSURE_PENDING) {
        if ((int32_t)(micros() - _bmpWaitUntilUs) < 0) return false;
        if (!readRegsDma(dmaBus, bus, BMP085_ADDR, BMP085_REG_RESULT, _bmpDmaBuf, 3, faultCode)) {
            setFaultIfNeeded(faultCode, SensorFaultCode::BaroReadFailed);
            return false;
        }
        const int32_t up = ((int32_t)_bmpDmaBuf[0] << 16 | (int32_t)_bmpDmaBuf[1] << 8 | _bmpDmaBuf[2]) >> (8 - 3);
        if (!baroDriver.applyRawPressure(up, buffer)) {
            setFaultIfNeeded(faultCode, SensorFaultCode::BaroReadFailed);
            return false;
        }
        _bmpState = BMP_IDLE;
        return true;
    }

    return false;
}

#endif
