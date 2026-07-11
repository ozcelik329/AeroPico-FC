#include "PioUart.h"

PioUart espUart;

#ifndef UNIT_TEST
static PioUart* activePioUart = nullptr;

static void pioUartIrqHandler() {
    if (activePioUart) {
        activePioUart->handleIrq();
    }
}

void PioUart::init(uint32_t baud) {
    begin(baud);
}

void PioUart::begin(uint32_t baud) {
    // TX
    _offset_tx = pio_add_program(_pio, &pio_uart_tx_program);
    _sm_tx     = pio_claim_unused_sm(_pio, true);

    pio_sm_config tx_cfg = pio_uart_tx_program_get_default_config(_offset_tx);
    sm_config_set_sideset_pins(&tx_cfg, PIN_ESP_TX);
    sm_config_set_out_pins(&tx_cfg, PIN_ESP_TX, 1);
    sm_config_set_out_shift(&tx_cfg, true, false, 8);  // LSB first
    float div = (float)clock_get_hz(clk_sys) / (8.0f * baud);
    sm_config_set_clkdiv(&tx_cfg, div);

    pio_gpio_init(_pio, PIN_ESP_TX);
    pio_sm_set_consecutive_pindirs(_pio, _sm_tx, PIN_ESP_TX, 1, true);
    pio_sm_init(_pio, _sm_tx, _offset_tx, &tx_cfg);
    pio_sm_set_enabled(_pio, _sm_tx, true);

    // RX
    _offset_rx = pio_add_program(_pio, &pio_uart_rx_program);
    _sm_rx     = pio_claim_unused_sm(_pio, true);

    pio_sm_config rx_cfg = pio_uart_rx_program_get_default_config(_offset_rx);
    sm_config_set_in_pins(&rx_cfg, PIN_ESP_RX);
    sm_config_set_in_shift(&rx_cfg, true, false, 8);   // LSB first
    sm_config_set_clkdiv(&rx_cfg, div);

    pio_gpio_init(_pio, PIN_ESP_RX);
    pio_sm_set_consecutive_pindirs(_pio, _sm_rx, PIN_ESP_RX, 1, false);
    pio_sm_init(_pio, _sm_rx, _offset_rx, &rx_cfg);
    pio_sm_set_enabled(_pio, _sm_rx, true);

    activePioUart = this;
    irq_set_exclusive_handler(PIO1_IRQ_0, pioUartIrqHandler);
    pio_set_irq0_source_enabled(_pio, pio_get_rx_fifo_not_empty_interrupt_source(_sm_rx), true);
    enableTxIrq(false);
    irq_set_enabled(PIO1_IRQ_0, true);

    Serial.println("[PIO UART] ESP32-CAM baglantisi hazir.");
}

size_t PioUart::write(const uint8_t* buf, size_t len) {
    size_t accepted = 0;
    for (size_t i = 0; i < len; i++) {
        const uint16_t next = (uint16_t)((_txHead + 1u) % TX_QUEUE_CAPACITY);
        if (next == _txTail) {
            _droppedBytes += (uint32_t)(len - i);
            break;
        }
        _txQueue[_txHead] = buf[i];
        _txHead = next;
        accepted++;
    }
    enableTxIrq(true);
    return accepted;
}

void PioUart::serviceTx() {
    while (_txTail != _txHead && !pio_sm_is_tx_fifo_full(_pio, _sm_tx)) {
        pio_sm_put(_pio, _sm_tx, _txQueue[_txTail]);
        _txTail = (uint16_t)((_txTail + 1u) % TX_QUEUE_CAPACITY);
    }
    if (_txTail == _txHead) {
        enableTxIrq(false);
    }
}

void PioUart::serviceRx() {
    while (!pio_sm_is_rx_fifo_empty(_pio, _sm_rx)) {
        const uint8_t value = (uint8_t)pio_sm_get(_pio, _sm_rx);
        const uint16_t next = (uint16_t)((_rxHead + 1u) % RX_QUEUE_CAPACITY);
        if (next == _rxTail) {
            _rxDroppedBytes++;
            continue;
        }
        _rxQueue[_rxHead] = value;
        _rxHead = next;
    }
}

void PioUart::handleIrq() {
    serviceRx();
    serviceTx();
}

void PioUart::enableTxIrq(bool enabled) {
    pio_set_irq0_source_enabled(_pio, pio_get_tx_fifo_not_full_interrupt_source(_sm_tx), enabled);
}

int PioUart::available() {
    return _rxHead == _rxTail ? 0 : 1;
}

int PioUart::read() {
    if (_rxHead == _rxTail) {
        return -1;
    }
    const uint8_t value = _rxQueue[_rxTail];
    _rxTail = (uint16_t)((_rxTail + 1u) % RX_QUEUE_CAPACITY);
    return (int)value;
}
#endif
