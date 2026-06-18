
#include <Arduino.h>

// Kendi dosyalarımızı dahil ediyoruz
#include "config.h"
#include "Sensors.h"
#include "RX.h"
#include "IMU.h"
#include "Output.h"

void setup() {
  // Seri portu başlat (GCS için hazırlık)
  Serial.begin(115200);
  
  // Sensörleri ve diğer modülleri başlat
  initSensors();
  
  Serial.println("AeroPico FC Baslatildi!");
}

void loop() {
  // Sensörlerden verileri oku
  updateSensors();
  
  // Uçuş hesaplamalarını yap
  // updateIMU();
  
  // Motorlara komut gönder
  // updateOutput();
}