#include <Arduino.h>
#include "config.h"
#include "drivers/Sensors.h"
#include "drivers/RX.h"
#include "drivers/IMU.h"        // Yeni eklenen IMU modülü
#include "core/SystemTimer.h"

// Modül nesnelerimizi tanımlıyoruz
SensorManager sensors;
RXManager rx;
IMUManager imu;                 // Füzyon ve Açı hesabı için

void setup() {
  Serial.begin(115200);
  
  // 1. Sistem ve Core 1 başlatma
  SystemManager::init();
  
  // 2. Sürücüleri ve IMU'yu başlat
  sensors.init();
  rx.init();
  imu.init();
  
  Serial.println("AeroPico FC Baslatildi - Sistem Hazir!");
}

void loop() {
  // --- Core 0: Veri Okuma ve İşleme (Data Pipeline) ---
  
  // 1. Sensörlerden ham verileri çek
  sensors.update();
  
  // 2. Ham verileri IMU'ya (Füzyon/Açı hesabı) gönder
  #ifdef USE_GY87
    imu.update(sensors.ax, sensors.ay, sensors.az, 
               sensors.gx, sensors.gy, sensors.gz, 
               sensors.mx, sensors.my, sensors.mz, 
               sensors.pressure);
  #else
    // GY-87 yoksa sadece ivme ve jiroskopla basit hesaplama
    imu.update(sensors.ax, sensors.ay, sensors.az, 
               sensors.gx, sensors.gy, sensors.gz, 
               0, 0, 0, 0);
  #endif
  
  // 3. Kumanda sinyallerini güncelle
  rx.update();
  
  // Not: PID ve Output (Motor) kontrolleri SystemTimer.cpp 
  // (Core 1) içerisinde senkronize bir şekilde yürütülmektedir.
}