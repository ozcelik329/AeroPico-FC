#include "Logger.h"
#include "UsbCdcTx.h"

void Logger::init() {
    // Serial.begin main.cpp'de zaten açılıyor; burada tekrar çağırmıyoruz.
    UsbCdcTx::enqueueLine("[Logger] Baslatildi.");
}

void Logger::log(const char* message) {
    UsbCdcTx::enqueueLine(message);
}

void Logger::logError(const char* error) {
    UsbCdcTx::enqueueFormat("[HATA] %s\n", error ? error : "(null)");
}
