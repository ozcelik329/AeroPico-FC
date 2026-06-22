#include <Arduino.h>
#include "core/FlightManager.h"
#include "core/SystemTimer.h"
#include "utils/Logger.h"

FlightManager flightManager;

void setup() {
    Serial.begin(115200);
    Logger::init();

    flightManager.init();
    SystemTimer::init();

    Logger::log("AeroPico FC Baslatildi!");
}

void loop() {
    flightManager.update();
}
