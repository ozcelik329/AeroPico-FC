#include "FlightModeController.h"

void FlightModeController::update(uint16_t sw_val) {
    FlightMode detected;
    if (sw_val < 1300) detected = MANUAL;
    else if (sw_val < 1700) detected = STABILIZE;
    else detected = AUTO;

    if (detected == _pending_mode) {
        _debounce_counter++;
    } else {
        _pending_mode = detected;
        _debounce_counter = 0;
    }

    // En az 5 paket boyunca aynıysa modu değiştir (Hata Raporu Fix)
    if (_debounce_counter >= 5) {
        _current_mode = _pending_mode;
    }
}