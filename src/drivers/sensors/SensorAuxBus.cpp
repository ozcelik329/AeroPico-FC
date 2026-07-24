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
    _auxDmaEnabled = true;
    _auxDmaTimeoutStreak = 0;
    _auxDmaTimeouts = 0;
    _auxPollingRecoveries = 0;
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

bool SensorAuxBus::readRegsNoFault(RP2350I2C& bus,
                                   uint8_t address,
                                   uint8_t reg,
                                   uint8_t* dest,
                                   size_t len) {
    return dest && len > 0 && bus.readRegisters(address, reg, dest, len);
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
    if (!_auxDmaEnabled) {
        (void)dmaBus;
        (void)bus;
        (void)address;
        (void)reg;
        (void)dest;
        (void)len;
        (void)kind;
        (void)faultCode;
        return false;
    }
    if (!dmaBus.hasAuxChannel()) {
        (void)faultCode;
        return false;
    }
    if (!dmaBus.startAuxRead(bus, address, reg, dest, len, micros())) {
        _auxDmaEnabled = false;
        return false;
    }
    _auxReadKind = kind;
    return true;
}

bool SensorAuxBus::finishReadFromBuffer(AuxReadKind kind,
                                        MagDriver& magDriver,
                                        BaroDriver& baroDriver,
                                        SensorBuffer& buffer,
                                        SensorFaultCode& faultCode) {
    if (kind == AUX_MAG) {
        return applyMagSampleFromBuffer(magDriver, buffer, faultCode);
    }

    if (kind == AUX_BARO_TEMP) {
        baroDriver.setRawTemperature((int32_t)(_bmpDmaBuf[0] << 8) | _bmpDmaBuf[1]);
        _bmpState = BMP_TEMP_READ;
        return true;
    }

    if (kind == AUX_BARO_PRESSURE) {
        const int32_t up = ((int32_t)_bmpDmaBuf[0] << 16 |
                            (int32_t)_bmpDmaBuf[1] << 8 |
                            _bmpDmaBuf[2]) >> (8 - 3);
        if (!baroDriver.applyRawPressure(up, buffer)) {
            setFaultIfNeeded(faultCode, SensorFaultCode::BaroReadFailed);
            return false;
        }
        _bmpState = BMP_IDLE;
        return true;
    }

    return true;
}

bool SensorAuxBus::applyMagSampleFromBuffer(MagDriver& magDriver,
                                            SensorBuffer& buffer,
                                            SensorFaultCode& faultCode) {
    if (!_magProfile) {
        setFaultIfNeeded(faultCode, SensorFaultCode::MagReadFailed);
        return false;
    }

    auto be16 = [](uint8_t hi, uint8_t lo) -> int16_t {
        return (int16_t)((hi << 8) | lo);
    };
    auto le16 = [](uint8_t lo, uint8_t hi) -> int16_t {
        return (int16_t)((hi << 8) | lo);
    };

    int16_t mx = 0;
    int16_t my = 0;
    int16_t mz = 0;
    if (_magProfile->layout == MagSampleLayout::QmcXyzLittleEndian) {
        mx = le16(_hmcDmaBuf[0], _hmcDmaBuf[1]);
        my = le16(_hmcDmaBuf[2], _hmcDmaBuf[3]);
        mz = le16(_hmcDmaBuf[4], _hmcDmaBuf[5]);
    } else {
        mx = be16(_hmcDmaBuf[0], _hmcDmaBuf[1]);
        mz = be16(_hmcDmaBuf[2], _hmcDmaBuf[3]);
        my = be16(_hmcDmaBuf[4], _hmcDmaBuf[5]);
    }

    magDriver.applySample(mx, my, mz, buffer, _magProfile->scaleMilliGaussPerCount);
    return true;
}

bool SensorAuxBus::recoverTimedOutRead(RP2350I2C& bus,
                                       AuxReadKind kind,
                                       MagDriver& magDriver,
                                       BaroDriver& baroDriver,
                                       SensorBuffer& buffer,
                                       SensorFaultCode& faultCode) {
    bool recovered = false;
    if (kind == AUX_MAG && _magProfile) {
        recovered = readRegsPolling(bus, _magProfile->address, _magProfile->dataReg,
                                    _hmcDmaBuf, _magProfile->sampleLen, faultCode) &&
                    finishReadFromBuffer(kind, magDriver, baroDriver, buffer, faultCode);
        if (!recovered) {
            buffer.mx = buffer.my = buffer.mz = 0.0f;
            setFaultIfNeeded(faultCode, SensorFaultCode::MagReadFailed);
        }
    } else if (_baroProfile && (kind == AUX_BARO_TEMP || kind == AUX_BARO_PRESSURE)) {
        const size_t len = kind == AUX_BARO_TEMP ? 2 : 3;
        recovered = readRegsPolling(bus, _baroProfile->address, _baroProfile->resultReg,
                                    _bmpDmaBuf, len, faultCode) &&
                    finishReadFromBuffer(kind, magDriver, baroDriver, buffer, faultCode);
        if (!recovered) {
            buffer.baroValid = false;
            buffer.pressureHpa = 0.0f;
            setFaultIfNeeded(faultCode, SensorFaultCode::BaroReadFailed);
        }
    }

    if (recovered) {
        _auxPollingRecoveries++;
        _auxDmaTimeoutStreak++;
        if (_auxDmaTimeoutStreak >= 1) {
            _auxDmaEnabled = false;
        }
    } else {
        _auxDmaTimeoutStreak++;
        setFaultIfNeeded(faultCode, SensorFaultCode::AuxDmaTransferTimeout);
    }
    return recovered;
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
        const AuxReadKind timedOutKind = _auxReadKind;
        dmaBus.abortAux();
        _auxDmaTimeouts++;
        recoverTimedOutRead(bus, timedOutKind, magDriver, baroDriver, buffer, faultCode);
        _auxReadKind = AUX_NONE;
        return true;
    }

    if (!dmaBus.isAuxReady()) {
        return false;
    }

    finishReadFromBuffer(_auxReadKind, magDriver, baroDriver, buffer, faultCode);
    _auxDmaTimeoutStreak = 0;

    dmaBus.finishAux();
    _auxReadKind = AUX_NONE;
    return true;
}

bool SensorAuxBus::initMag(RP2350I2C& bus, SensorFaultCode& faultCode) {
    _unsupportedMagDetected = false;
    _unsupportedMagAddress = 0;

    uint8_t probe[3] = {};
    _magProfile = &SensorBackendRegistry::hmc5883l();
    const MagDeviceProfile& profile = *_magProfile;
    if (!readRegsNoFault(bus, profile.address, profile.dataReg, probe, sizeof(probe))) {
        const MagDeviceProfile& qmc = SensorBackendRegistry::qmc5883pCompat();
        if (readRegsNoFault(bus, qmc.address, qmc.dataReg, probe, sizeof(probe))) {
            _magProfile = &qmc;
            bool qmcOk = true;
            qmcOk &= writeReg(bus, qmc.address, qmc.configAReg, qmc.configAValue, faultCode);
            qmcOk &= writeReg(bus, qmc.address, qmc.configBReg, qmc.configBValue, faultCode);
            qmcOk &= writeReg(bus, qmc.address, qmc.modeReg, qmc.modeValue, faultCode);
            if (qmcOk) {
                _unsupportedMagDetected = false;
                _unsupportedMagAddress = 0;
                return true;
            }
            _unsupportedMagDetected = true;
            _unsupportedMagAddress = qmc.address;
        }

        _magProfile = nullptr;
        setFaultIfNeeded(faultCode, _unsupportedMagDetected
                                      ? SensorFaultCode::MagUnsupported
                                      : SensorFaultCode::MagReadFailed);
        return false;
    }

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
            applyMagSampleFromBuffer(magDriver, buffer, faultCode);
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
