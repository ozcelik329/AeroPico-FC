#include "Logger.h"
#include <Arduino.h>

void Logger::init() {
    // Serial.begin main.cpp'de zaten açılıyor; burada tekrar çağırmıyoruz.
    Serial.println("[Logger] Baslatildi.");
}

void Logger::log(const char* message) {
    Serial.println(message);
}

void Logger::logError(const char* error) {
    Serial.print("[HATA] ");
    Serial.println(error);
}
