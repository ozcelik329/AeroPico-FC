# AeroPico-FC v1.0.0-rc1 RCI Notlari

Bu surum, manuel sabit kanat ucus altyapisini Mission Planner, QGroundControl,
AeroPico Configurator ve ilerideki AeroPico GCS ile ayni MAVLink Common temeli
uzerinden calisacak sekilde hazirlar.

## Kapsam

- USB Serial ve PIO UART uzerinden ortak MAVLink transport.
- Mission Planner / QGC icin generic autopilot uyumlulugu:
  - `HEARTBEAT` armed biti
  - `COMMAND_LONG` arm/disarm
  - `COMMAND_ACK`
  - `SYS_STATUS`
  - `VFR_HUD`
  - GPS yokken `GPS_RAW_INT` / `NO_GPS`
  - `MISSION_REQUEST_LIST` icin `MISSION_COUNT = 0`
- Arm/disarm komutlari preflight, failsafe, throttle ve sistem fault kapilarindan gecer.
- Parametre cevaplari artik ortak MAVLink transport yolunu kullanir.
- AeroPico Configurator servis komutlari firmware'e baglandi:
  - IMU kalibrasyon
  - iki asamali mag hard-iron kalibrasyon
  - sensor/preflight/RC durum sorgusu
  - disarmed/safe servo test kapisi
- Mutating MAVLink/Configurator komutlari task-safe `ServiceCommandMailbox`
  ve `ServiceCommandProcessor` yoluna tasindi; telemetry task artik sensor veya
  control nesnelerini dogrudan degistirmez.
- IMU kalibrasyonu sensor task icinde ornek geldikce ilerleyen non-blocking
  state machine oldu; heartbeat/telemetry akisi kalibrasyon sirasinda durmaz.
- Sensor rol/backend ayrimi baslatildi:
  - `BaroDriver` rol katmani
  - `Bmp085Backend` cihaz kompanzasyonu
  - `GyroAccelDriver` + `Mpu6050Backend`
  - `MagDriver` + `Hmc5883lBackend`
  - `SensorBackendRegistry` ve `SensorDeviceProfile`
  - `SensorAuxBus` cihaz register profilinden calisir.
- Mag/baro aux DMA yolunda kanal/timeout hatasinda bounded polling fallback var.
- Servo output latest-value 50 Hz modeliyle FIFO drop/stale/latency sayaçlari
  tutar ve safe-frame onceligi uygular.
- Parametre ve kalibrasyon storage iki slotlu generation + checksum journal
  modeline tasindi.
- `TimingMonitor` azalan sure orneklerinde unsigned EWMA underflow uretmez.
- `BuildInfo` firmware adi, hedef kart, MCU ve surum bilgisini tek yerde tutar.
- ESP32 WiFi MAVLink bridge iskeleti `Esp32Cam/` altinda hazirdir.

## Bilerek Aktif Edilmeyenler

- RTL, waypoint mission, loiter ve auto landing v1.0 RCI kapsaminda aktif degildir.
- Mission protokolu simdilik `MISSION_COUNT = 0` ile time-out engelleyen stub seviyesindedir.
- ESP32 WiFi bridge opsiyoneldir; ilk bench dogrulama Pico USB Serial ile yapilmalidir.

## Ucus / Ticari Hazirlik Notu

Bu paket yazilim RCI seviyesidir. Fiziksel ucus veya ticari kullanim icin su
bench kanitlari tamamlanmadan final kabul verilmemelidir:

- Mission Planner USB baglanti kaydi
- QGroundControl USB baglanti kaydi
- Servo PWM logic analyzer capture
- SBUS GP1 alici testi
- Battery ADC multimetre dogrulamasi
- ESP32/WiFi UDP 14550 MAVLink bridge testi

## Yazilim Kabul Kaniti

v1.0.0-rc1 yazilim kapatma kriterleri:

- `pio test -e native`
- `pio run -e native_link`
- `python3 tools/ci/check_architecture.py`
- `python3 tools/fault_injection/fault_injection.py`
- `pio run -e pico`
- `cd tools/aeropico-configurator && npm run check`

Bu kontroller gecmeden release tag'i yayinlanmamalidir.
