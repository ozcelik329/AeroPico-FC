#include "SensorAuxBus.h"
#include "SensorBackendRegistry.h"

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
    if (!processPendingRead(dmaBus, bus, magDriver, baroDriver, buffer, faultCode)) {
        return;
    }

    const bool baroInProgress = _bmpState != BMP_IDLE;
    if (_hasMag && _magTurn && !baroInProgress) {
        readMag(dmaBus, bus, magDriver, buffer, faultCode);
        _magTurn = false;
        return;
    }

    if (_hasBaro) {
        if (!readBaro(dmaBus, bus, baroDriver, buffer, faultCode)) {
            buffer.baroValid = false;
            buffer.pressureHpa = 0.0f;
        }
        _magTurn = true;
    } else {
        buffer.baroValid = false;
        buffer.pressureHpa = 0.0f;
        if (_hasMag) {
            _magTurn = true;
        } else {
            buffer.mx = buffer.my = buffer.mz = 0.0f;
        }
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
        return readRegsPolling(bus, address, reg, dest, len, faultCode);
    }

    if (!dmaBus.readAuxRegistersDma(bus, address, reg, dest, len, I2C_DMA_TIMEOUT_US, micros)) {
        return readRegsPolling(bus, address, reg, dest, len, faultCode);
    }

    return true;
}

bool SensorAuxBus::readRegsPolling(RP2350I2C& bus,
                                   uint8_t address,
                                   uint8_t reg,
                                   uint8_t* dest,
                                   size_t len,
                                   SensorFaultCode& faultCode) {
    if (!dest || len == 0 || !bus.readRegisters(address, reg, dest, len)) {
        setFaultIfNeeded(faultCode, SensorFaultCode::AuxPollingFallbackFailed);
        return false;
    }
    return true;
}

bool SensorAuxBus::startRegsDma(SensorDmaBus& dmaBus,
                                RP2350I2C& bus,
                                uint8_t address,
                                uint8_t reg,
                                uint8_t* dest,
                                size_t len,
                                AuxReadKind kind,
                                SensorFaultCode& faultCode) {
    if (!dmaBus.hasAuxChannel()) {
        setFaultIfNeeded(faultCode, SensorFaultCode::DmaChannelClaimFailed);
        return false;
    }
    if (!dmaBus.startAuxRead(bus, address, reg, dest, len, micros())) {
        setFaultIfNeeded(faultCode, SensorFaultCode::AuxDmaTransferTimeout);
        return false;
    }
    _auxReadKind = kind;
    return true;
}

bool SensorAuxBus::processPendingRead(SensorDmaBus& dmaBus,
                                      RP2350I2C& bus,
                                      MagDriver& magDriver,
                                      BaroDriver& baroDriver,
                                      SensorBuffer& buffer,
                                      SensorFaultCode& faultCode) {
    if (_auxReadKind == AUX_NONE) {
        return true;
    }

    const uint32_t nowUs = micros();
    if (dmaBus.auxTimedOut(nowUs, I2C_DMA_TIMEOUT_US)) {
        dmaBus.abortAux();
        setFaultIfNeeded(faultCode, SensorFaultCode::AuxDmaTransferTimeout);
        if (_auxReadKind == AUX_MAG) {
            if (_magProfile &&
                readRegsPolling(bus, _magProfile->address, _magProfile->dataReg,
                                _hmcDmaBuf, _magProfile->sampleLen, faultCode)) {
                const int16_t mx = (int16_t)(_hmcDmaBuf[0] << 8 | _hmcDmaBuf[1]);
                const int16_t my = (int16_t)(_hmcDmaBuf[4] << 8 | _hmcDmaBuf[5]);
                const int16_t mz = (int16_t)(_hmcDmaBuf[2] << 8 | _hmcDmaBuf[3]);
                magDriver.applySample(mx, my, mz, buffer);
            } else {
                buffer.mx = buffer.my = buffer.mz = 0.0f;
                setFaultIfNeeded(faultCode, SensorFaultCode::MagReadFailed);
            }
        } else {
            if (_baroProfile &&
                readRegsPolling(bus, _baroProfile->address, _baroProfile->resultReg,
                                _bmpDmaBuf, _auxReadKind == AUX_BARO_TEMP ? 2 : 3, faultCode)) {
                if (_auxReadKind == AUX_BARO_TEMP) {
                    baroDriver.setRawTemperature((int32_t)(_bmpDmaBuf[0] << 8) | _bmpDmaBuf[1]);
                    _bmpState = BMP_TEMP_READ;
                } else {
                    const int32_t up = ((int32_t)_bmpDmaBuf[0] << 16 | (int32_t)_bmpDmaBuf[1] << 8 | _bmpDmaBuf[2]) >> (8 - 3);
                    if (!baroDriver.applyRawPressure(up, buffer)) {
                        setFaultIfNeeded(faultCode, SensorFaultCode::BaroReadFailed);
                    }
                    _bmpState = BMP_IDLE;
                }
            } else {
                buffer.baroValid = false;
                buffer.pressureHpa = 0.0f;
                setFaultIfNeeded(faultCode, SensorFaultCode::BaroReadFailed);
            }
        }
        _auxReadKind = AUX_NONE;
        return true;
    }

    if (!dmaBus.isAuxReady()) {
        return false;
    }

    if (_auxReadKind == AUX_MAG) {
        const int16_t mx = (int16_t)(_hmcDmaBuf[0] << 8 | _hmcDmaBuf[1]);
        const int16_t my = (int16_t)(_hmcDmaBuf[4] << 8 | _hmcDmaBuf[5]);
        const int16_t mz = (int16_t)(_hmcDmaBuf[2] << 8 | _hmcDmaBuf[3]);
        magDriver.applySample(mx, my, mz, buffer);
    } else if (_auxReadKind == AUX_BARO_TEMP) {
        baroDriver.setRawTemperature((int32_t)(_bmpDmaBuf[0] << 8) | _bmpDmaBuf[1]);
        _bmpState = BMP_TEMP_READ;
    } else if (_auxReadKind == AUX_BARO_PRESSURE) {
        const int32_t up = ((int32_t)_bmpDmaBuf[0] << 16 | (int32_t)_bmpDmaBuf[1] << 8 | _bmpDmaBuf[2]) >> (8 - 3);
        if (!baroDriver.applyRawPressure(up, buffer)) {
            setFaultIfNeeded(faultCode, SensorFaultCode::BaroReadFailed);
        }
        _bmpState = BMP_IDLE;
    }

    dmaBus.finishAux();
    _auxReadKind = AUX_NONE;
    return true;
}

bool SensorAuxBus::initMag(RP2350I2C& bus, SensorFaultCode& faultCode) {
    _magProfile = &SensorBackendRegistry::hmc5883l();
    const MagDeviceProfile& profile = *_magProfile;
    bool ok = true;
    ok &= writeReg(bus, profile.address, profile.configAReg, profile.configAValue, faultCode);
    ok &= writeReg(bus, profile.address, profile.configBReg, profile.configBValue, faultCode);
    ok &= writeReg(bus, profile.address, profile.modeReg, profile.modeValue, faultCode);
    if (!ok) {
        _magProfile = nullptr;
        setFaultIfNeeded(faultCode, SensorFaultCode::MagReadFailed);
    }
    return ok;
}

bool SensorAuxBus::initBaro(SensorDmaBus& dmaBus,
                            RP2350I2C& bus,
                            BaroDriver& baroDriver,
                            SensorFaultCode& faultCode) {
    _baroProfile = &SensorBackendRegistry::bmp085();
    const BaroDeviceProfile& profile = *_baroProfile;
    uint8_t calib[22];
    if (!readRegsDma(dmaBus, bus, profile.address, profile.calibrationReg, calib, profile.calibrationLen, faultCode)) {
        _baroProfile = nullptr;
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
    (void)magDriver;
    if (!_magProfile ||
        !startRegsDma(dmaBus, bus, _magProfile->address, _magProfile->dataReg,
                      _hmcDmaBuf, _magProfile->sampleLen, AUX_MAG, faultCode)) {
        if (_magProfile &&
            readRegsPolling(bus, _magProfile->address, _magProfile->dataReg,
                            _hmcDmaBuf, _magProfile->sampleLen, faultCode)) {
            const int16_t mx = (int16_t)(_hmcDmaBuf[0] << 8 | _hmcDmaBuf[1]);
            const int16_t my = (int16_t)(_hmcDmaBuf[4] << 8 | _hmcDmaBuf[5]);
            const int16_t mz = (int16_t)(_hmcDmaBuf[2] << 8 | _hmcDmaBuf[3]);
            magDriver.applySample(mx, my, mz, buffer);
        } else {
            buffer.mx = buffer.my = buffer.mz = 0.0f;
            setFaultIfNeeded(faultCode, SensorFaultCode::MagReadFailed);
        }
    }
}

bool SensorAuxBus::readBaro(SensorDmaBus& dmaBus,
                            RP2350I2C& bus,
                            BaroDriver& baroDriver,
                            SensorBuffer& buffer,
                            SensorFaultCode& faultCode) {
    (void)baroDriver;
    if (!_baroProfile) {
        setFaultIfNeeded(faultCode, SensorFaultCode::BaroReadFailed);
        return false;
    }
    const BaroDeviceProfile& profile = *_baroProfile;
    if (_bmpState == BMP_IDLE) {
        if (!writeReg(bus, profile.address, profile.controlReg, profile.temperatureCommand, faultCode)) {
            setFaultIfNeeded(faultCode, SensorFaultCode::BaroReadFailed);
            return false;
        }
        _bmpWaitUntilUs = micros() + profile.temperatureWaitUs;
        _bmpState = BMP_TEMP_PENDING;
        return false;
    }

    if (_bmpState == BMP_TEMP_PENDING) {
        if ((int32_t)(micros() - _bmpWaitUntilUs) < 0) return false;
        if (!startRegsDma(dmaBus, bus, profile.address, profile.resultReg,
                          _bmpDmaBuf, 2, AUX_BARO_TEMP, faultCode)) {
            if (!readRegsPolling(bus, profile.address, profile.resultReg, _bmpDmaBuf, 2, faultCode)) {
                setFaultIfNeeded(faultCode, SensorFaultCode::BaroReadFailed);
                return false;
            }
            baroDriver.setRawTemperature((int32_t)(_bmpDmaBuf[0] << 8) | _bmpDmaBuf[1]);
            _bmpState = BMP_TEMP_READ;
            return false;
        }
        return false;
    }

    if (_bmpState == BMP_TEMP_READ) {
        if (!writeReg(bus, profile.address, profile.controlReg,
                      profile.pressureCommand + (profile.pressureOversampling << 6), faultCode)) {
            setFaultIfNeeded(faultCode, SensorFaultCode::BaroReadFailed);
            return false;
        }
        _bmpWaitUntilUs = micros() + profile.pressureWaitUs;
        _bmpState = BMP_PRESSURE_PENDING;
        return false;
    }

    if (_bmpState == BMP_PRESSURE_PENDING) {
        if ((int32_t)(micros() - _bmpWaitUntilUs) < 0) return false;
        if (!startRegsDma(dmaBus, bus, profile.address, profile.resultReg,
                          _bmpDmaBuf, 3, AUX_BARO_PRESSURE, faultCode)) {
            if (!readRegsPolling(bus, profile.address, profile.resultReg, _bmpDmaBuf, 3, faultCode)) {
                setFaultIfNeeded(faultCode, SensorFaultCode::BaroReadFailed);
                return false;
            }
            const int32_t up = ((int32_t)_bmpDmaBuf[0] << 16 | (int32_t)_bmpDmaBuf[1] << 8 | _bmpDmaBuf[2]) >> (8 - 3);
            _bmpState = BMP_IDLE;
            return baroDriver.applyRawPressure(up, buffer);
        }
        return false;
    }

    return false;
}

#endif
