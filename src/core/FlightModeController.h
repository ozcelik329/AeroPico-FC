#ifndef FLIGHT_MODE_CONTROLLER_H
#define FLIGHT_MODE_CONTROLLER_H

#include <Arduino.h>

enum FlightMode { MANUAL = 0, STABILIZE = 1, AUTO = 2 };

class FlightModeController {
public:
    void update(uint16_t sw_val);
    FlightMode getMode() { return _current_mode; }

private:
    FlightMode _current_mode = MANUAL;
    FlightMode _pending_mode = MANUAL;
    uint8_t _debounce_counter = 0; // Hata Fix: Switch Debounce
};

#endif