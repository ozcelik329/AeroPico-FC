# AeroPico FC - Proje Yapisi

Bu dokuman kaynak kodun hedef mimari sinirlarini tarif eder. Yeni dosya eklerken once bu sinirlara gore yer secilmelidir.

## Ust Seviye Yapi

```text
src/
  board/       Kart, pin ve donanim esleme dogrulamalari
  core/        Ucus mantigi, kontrol, zamanlama, mixer, sensor fusion
  drivers/     Donanim suruculeri ve HAL arayuzleri
  estimators/  Durum tahmini / EKF icin ayrilmis katman
  filters/     Tekrar kullanilabilir filtreleme algoritmalari
  hal/         Platform bagimsiz donanim soyutlama arayuzleri
  storage/     Kalibrasyon ve ileride parametre kaliciligi icin saklama arayuzleri
  telemetry/   MAVLink, parametre ve blackbox akislari
  utils/       Loglama ve boot yardimcilari
  config.h     Kart ve firmware konfigurasyonu
  def.h        Dusuk seviyeli dongu sabitleri
  types.h      Katmanlar arasi ortak veri tipleri
```

## Katman Kurallari

- `board/`: Donanim esleme ve derleme zamani dogrulama. Surucu implementasyonu burada olmaz.
- `drivers/`: I2C, UART, PIO, PWM, SBUS gibi fiziksel donanim detaylari. Ucus karari vermez.
- `estimators/`: EKF/complementary/altitude estimator gibi durum tahmini modulleri. Donanima baglanmaz, sade veri tipleriyle calisir.
- `filters/`: Median, low-pass, notch gibi genel algoritmalar. Arduino/Pico donanim API'lerine bagimli olmamali.
- `hal/`: GPIO, PWM, I2C, SPI, UART ve timer icin platform bagimsiz arayuzler. RP2350 adaptörleri `hal/rp2350/` altinda tutulur.
- `storage/`: Kalibrasyon/parametre gibi kalici veriler icin arayuz ve test edilebilir implementasyonlar. Flash yazma ayrintisi burada soyutlanmali.
- `core/`: FlightManager, PID, mixer, sensor fusion ve flight mode mantigi. Donanima dogrudan erismek yerine `IDrivers.h` arayuzlerini kullanmali.
- `telemetry/`: MAVLink ve log aktarimi. Uzun vadede `FlightManager` globaline dogrudan baglanmak yerine callback/interface kullanmali.
- `utils/`: Loglama, boot banner, genel yardimci kodlar. Kritik ucus hesaplari burada olmaz.
- `test/`: Native testler ilgili modulu davranis seviyesinde korumali.

## Yeni Dosya Eklerken

1. Donanim pini veya kart secimi mi? `src/board/`
2. Fiziksel cihaz surucusu mu? `src/drivers/`
3. Ucus/kontrol karari mi? `src/core/`
4. Genel algoritma mi? `src/filters/`
5. Platform donanim soyutlamasi mi? `src/hal/`
6. Kalici veri/saklama arayuzu mu? `src/storage/`
7. Yer istasyonu veya log akisi mi? `src/telemetry/`
8. Sadece yardimci log/boot araci mi? `src/utils/`

## Mevcut Refactor Kararlari

- `RunningMedian` `core/` altindan `filters/` altina tasindi; sensor gurultusu azaltma bir ucus karari degil, genel filtre algoritmasidir.
- SBUS pin/UART dogrulamasi `board/PinValidation.h` altina alindi; pin esleme hatalari derleme zamaninda yakalanmalidir.
- Kalibrasyon saklama API'si `storage/` altina alindi; ilk implementasyon native testler icin RAM tabanli, donanim sonrasi flash/LittleFS baglantisi eklenecek.
- `FlightManager` icindeki RC, sensor, controller ve state snapshot sorumluluklari ayrildi. RC akisi `RCPipeline`, sensor/fusion akisi `SensorPipeline`, controller orkestrasyonu `ControlPipeline`, flight loop `FlightControlTask`, snapshot uretimi `StatePublisher` icindedir.
- Sensor suruculeri modül adina gore degil role gore adlandirilir. `drivers/sensors/gyro`, `drivers/sensors/mag` ve `drivers/sensors/baro` altindaki siniflar ileride MPU/HMC/BMP yerine farkli moduller gelse bile ayni mimari siniri korur.
- HAL arayuzleri `src/hal/` altinda baslatildi; `RP2350Timer` ve `RP2350PWM` adaptörleri eklendi. I2C/UART adaptörleri mevcut surucu kodu tasinmadan once iskelet durumunda.
- `Scheduler` ve `PreflightHealth` cekirdek siniflari test-first eklendi; ucus akisina kademeli baglanacaklar.
- `config.h` simdilik kokte tutulur. Bir sonraki buyuk refactor'da `src/config/` altina bolunebilir, ancak mevcut include zincirini kiracak toplu tasima acele yapilmamalidir.

## Hedef Nihai Yapi

```text
src/
  app/         setup/task orkestrasyonu (ileride main.cpp buraya inceltilebilir)
  board/       Board tanimlari, pin validasyonlari
  config/      Build-time ve runtime konfigurasyon ayrimi
  core/        Ucus kontrol cekirdegi
  drivers/     HAL ve donanim suruculeri
  estimators/  EKF/complementary/filter fusion katmani (ileride)
  filters/     Genel filtre algoritmalari
  hal/         Platform soyutlama ve RP2350 adaptörleri
  storage/     Kalibrasyon ve parametre kaliciligi
  telemetry/   MAVLink, blackbox, parametre
  utils/       Boot/log/panic yardimcilari
```
