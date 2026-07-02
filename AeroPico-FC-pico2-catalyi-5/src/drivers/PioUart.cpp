#include "PioUart.h"

PioUart espUart;

void PioUart::init(uint32_t baud) {
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

    Serial.println("[PIO UART] ESP32-CAM baglantisi hazir.");
}

void PioUart::write(const uint8_t* buf, size_t len) {
    for (size_t i = 0; i < len; i++) {
        pio_sm_put_blocking(_pio, _sm_tx, buf[i]);
    }
}

bool PioUart::available() {
    return !pio_sm_is_rx_fifo_empty(_pio, _sm_rx);
}

uint8_t PioUart::read() {
    return (uint8_t)pio_sm_get_blocking(_pio, _sm_rx);
}