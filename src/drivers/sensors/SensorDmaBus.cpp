#include "SensorDmaBus.h"

bool SensorDmaBus::configureMpu(RP2350I2C& bus) {
    _mpuRxChan = dma_claim_unused_channel(false);
    _mpuTxChan = dma_claim_unused_channel(false);
    if (!hasMpuChannels()) {
        if (_mpuRxChan >= 0) dma_channel_unclaim(_mpuRxChan);
        if (_mpuTxChan >= 0) dma_channel_unclaim(_mpuTxChan);
        _mpuRxChan = -1;
        _mpuTxChan = -1;
        return false;
    }

    dma_channel_config rxCfg = dma_channel_get_default_config(_mpuRxChan);
    channel_config_set_transfer_data_size(&rxCfg, DMA_SIZE_8);
    channel_config_set_read_increment(&rxCfg, false);
    channel_config_set_write_increment(&rxCfg, true);
    channel_config_set_dreq(&rxCfg, bus.dmaDreq(false));

    dma_channel_configure(
        _mpuRxChan,
        &rxCfg,
        _mpuRx,
        bus.dataCommandRegister(),
        MPU_RAW_LEN,
        false
    );

    dma_channel_config txCfg = dma_channel_get_default_config(_mpuTxChan);
    channel_config_set_transfer_data_size(&txCfg, DMA_SIZE_16);
    channel_config_set_read_increment(&txCfg, true);
    channel_config_set_write_increment(&txCfg, false);
    channel_config_set_dreq(&txCfg, bus.dmaDreq(true));

    dma_channel_configure(
        _mpuTxChan,
        &txCfg,
        bus.dataCommandRegister(),
        _mpuCmd,
        1 + MPU_RAW_LEN,
        false
    );

    return true;
}

bool SensorDmaBus::configureAuxRx(RP2350I2C& bus) {
    _auxRxChan = dma_claim_unused_channel(false);
    _auxTxChan = dma_claim_unused_channel(false);
    if (!hasAuxChannel()) {
        if (_auxRxChan >= 0) dma_channel_unclaim(_auxRxChan);
        if (_auxTxChan >= 0) dma_channel_unclaim(_auxTxChan);
        _auxRxChan = -1;
        _auxTxChan = -1;
        return false;
    }

    dma_channel_config auxCfg = dma_channel_get_default_config(_auxRxChan);
    channel_config_set_transfer_data_size(&auxCfg, DMA_SIZE_8);
    channel_config_set_read_increment(&auxCfg, false);
    channel_config_set_write_increment(&auxCfg, true);
    channel_config_set_dreq(&auxCfg, bus.dmaDreq(false));
    dma_channel_configure(
        _auxRxChan,
        &auxCfg,
        nullptr,
        bus.dataCommandRegister(),
        0,
        false
    );

    dma_channel_config txCfg = dma_channel_get_default_config(_auxTxChan);
    channel_config_set_transfer_data_size(&txCfg, DMA_SIZE_16);
    channel_config_set_read_increment(&txCfg, true);
    channel_config_set_write_increment(&txCfg, false);
    channel_config_set_dreq(&txCfg, bus.dmaDreq(true));
    dma_channel_configure(
        _auxTxChan,
        &txCfg,
        bus.dataCommandRegister(),
        _auxCmd,
        0,
        false
    );

    return true;
}

void SensorDmaBus::prepareMpuCommands(uint8_t reg) {
    _mpuCmd[0] = reg;
    for (size_t i = 0; i < MPU_RAW_LEN; i++) {
        _mpuCmd[1 + i] = READ_CMD | (i == MPU_RAW_LEN - 1 ? STOP_CMD : 0);
    }
}

bool SensorDmaBus::prepareAuxCommands(size_t len) {
    if (len == 0 || len > AUX_MAX_LEN) {
        return false;
    }

    for (size_t i = 0; i < len; i++) {
        _auxCmd[i] = READ_CMD | (i == len - 1 ? STOP_CMD : 0);
    }
    return true;
}

bool SensorDmaBus::startMpuRead(RP2350I2C& bus, uint8_t address, uint8_t reg, uint32_t nowUs) {
    if (!hasMpuChannels() || _mpuActive) {
        return false;
    }

    prepareMpuCommands(reg);

    dma_channel_set_write_addr(_mpuRxChan, _mpuRx, false);
    dma_channel_set_trans_count(_mpuRxChan, MPU_RAW_LEN, false);
    dma_channel_set_read_addr(_mpuTxChan, _mpuCmd, false);
    dma_channel_set_write_addr(_mpuTxChan, bus.dataCommandRegister(), false);
    dma_channel_set_trans_count(_mpuTxChan, 1 + MPU_RAW_LEN, false);

    bus.setTarget(address);
    bus.setDmaEnabled(true, true);

    dma_channel_start(_mpuRxChan);
    dma_channel_start(_mpuTxChan);
    _mpuStartUs = nowUs;
    _mpuActive = true;
    return true;
}

bool SensorDmaBus::isMpuReady() const {
    return _mpuActive && hasMpuChannels() && !dma_channel_is_busy(_mpuRxChan);
}

bool SensorDmaBus::mpuTimedOut(uint32_t nowUs, uint32_t timeoutUs) const {
    return _mpuActive && hasMpuChannels() && (uint32_t)(nowUs - _mpuStartUs) > timeoutUs;
}

void SensorDmaBus::finishMpu() {
    _mpuActive = false;
}

void SensorDmaBus::abortMpu() {
    if (_mpuRxChan >= 0) dma_channel_abort(_mpuRxChan);
    if (_mpuTxChan >= 0) dma_channel_abort(_mpuTxChan);
    _mpuActive = false;
}

bool SensorDmaBus::readAuxRegistersDma(RP2350I2C& bus,
                                       uint8_t address,
                                       uint8_t reg,
                                       uint8_t* dest,
                                       size_t len,
                                       uint32_t timeoutUs,
                                       uint32_t (*nowUs)()) {
    if (!startAuxRead(bus, address, reg, dest, len, nowUs())) {
        return false;
    }
    const uint32_t startUs = nowUs();
    while (!isAuxReady()) {
        if ((uint32_t)(nowUs() - startUs) > timeoutUs) {
            abortAux();
            return false;
        }
        yield();
    }

    return true;
}

bool SensorDmaBus::startAuxRead(RP2350I2C& bus,
                                uint8_t address,
                                uint8_t reg,
                                uint8_t* dest,
                                size_t len,
                                uint32_t nowUs) {
    if (!hasAuxChannel() || _auxActive) {
        return false;
    }

    if (!dest || !prepareAuxCommands(len)) {
        return false;
    }

    if (!bus.writeRaw(address, &reg, 1, true)) {
        return false;
    }

    dma_channel_set_write_addr(_auxRxChan, dest, false);
    dma_channel_set_trans_count(_auxRxChan, len, false);
    dma_channel_set_read_addr(_auxTxChan, _auxCmd, false);
    dma_channel_set_write_addr(_auxTxChan, bus.dataCommandRegister(), false);
    dma_channel_set_trans_count(_auxTxChan, len, false);

    bus.setTarget(address);
    bus.setDmaEnabled(true, true);

    dma_channel_start(_auxRxChan);
    dma_channel_start(_auxTxChan);
    _auxStartUs = nowUs;
    _auxActive = true;
    return true;
}

bool SensorDmaBus::isAuxReady() const {
    if (!_auxActive || !hasAuxChannel()) {
        return false;
    }
    return !dma_channel_is_busy(_auxRxChan);
}

bool SensorDmaBus::auxTimedOut(uint32_t nowUs, uint32_t timeoutUs) const {
    return _auxActive && hasAuxChannel() && (uint32_t)(nowUs - _auxStartUs) > timeoutUs;
}

void SensorDmaBus::finishAux() {
    _auxActive = false;
}

void SensorDmaBus::abortAux() {
    if (_auxRxChan >= 0) dma_channel_abort(_auxRxChan);
    if (_auxTxChan >= 0) dma_channel_abort(_auxTxChan);
    _auxActive = false;
}
