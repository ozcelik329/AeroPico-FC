# AeroPico FC: Raspberry Pi Pico Uçuş Kontrolcüsü

Bu proje, Raspberry Pi Pico W kullanarak geliştirilen açık kaynaklı bir uçuş kontrolcüsü yazılımıdır. 

## Dosya Yapısı ve İşlevleri

- `src/main.cpp`: Sistemin kalbi. `setup()` ve `loop()` döngülerini yönetir.
- `src/config.h`: Donanım pinleri, PID değerleri ve uçuş konfigürasyonlarının tutulduğu merkez.
- `src/types.h`: Uçuş verileri (IMU verileri, RC kanalları) için ortak veri yapıları (struct).
- `src/def.h`: Genel tanımlamalar ve sistem zamanlama sabitleri.
- `src/RX.cpp / .h`: Kumanda alıcısı (SBUS) ile haberleşme ve veri işleme.
- `src/Sensors.cpp / .h`: MPU6050 sensöründen veri okuma ve kalibrasyon.
- `src/IMU.cpp / .h`: Uçuş açılarının hesaplanması ve filtreleme algoritmaları.
- `src/Output.cpp / .h`: Motor ESC kontrolü ve PWM sinyal çıkışları.

## Nasıl Derlenir?
1. PlatformIO ile projeyi açın.
2. `platformio.ini` dosyasında gerekli kütüphanelerin tanımlı olduğundan emin olun.
3. VS Code üzerindeki "Build" butonuna basın.
4. Oluşan `firmware.uf2` dosyasını `BOOTSEL` modundaki Pico'ya sürükleyin.

## Donanım Gereksinimleri
- Raspberry Pi Pico / Pico W
- MPU6050 (I2C)
- SBUS destekli alıcı
- ESC ve Fırçasız Motorlar
