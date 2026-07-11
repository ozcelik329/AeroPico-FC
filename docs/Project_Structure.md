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
- Kalibrasyon ve parametre saklama API'leri `storage/` altina alindi. Native testler icin RAM tabanli, RP2350 icin dogrudan flash tabanli implementasyon vardir; parametre kaydi `PARAM_SAVE` komutuyla kontrollu yapilir.
- `FlightManager` icindeki RC, sensor, controller ve state snapshot sorumluluklari ayrildi. RC akisi `RCPipeline`, sensor/fusion akisi `SensorPipeline`, controller orkestrasyonu `ControlPipeline`, flight loop `FlightControlTask`, snapshot uretimi `StatePublisher` icindedir.
- Sensor suruculeri modül adina gore degil role gore adlandirilir. `drivers/sensors/gyro`, `drivers/sensors/mag` ve `drivers/sensors/baro` altindaki siniflar ileride MPU/HMC/BMP yerine farkli moduller gelse bile ayni mimari siniri korur.
- IMU kalibrasyon hesaplamasi `drivers/sensors/SensorCalibration.*` icindedir. `Sensors.cpp` ham ornek okuma ve sonucu uygulama disinda kalibrasyon matematigi tasimamali.
- Sensor DMA orkestrasyonu `drivers/sensors/SensorDmaBus.*` icindedir. MPU ve yardimci sensor okumalari RX+TX DMA komut zinciri kullanir; kanal tahsis hatasinda alinmis kanal geri birakilir. GY87 mag/baro state machine'i `SensorAuxBus.*` icine ayrildi ve `Sensors.cpp` sadece sensor orkestrasyonu sinirinda tutulur.
- HAL arayuzleri `src/hal/` altinda baslatildi; `RP2350Timer`, `RP2350PWM`, `RP2350I2C` ve `RP2350ADC` adaptörleri eklendi. Sensor I2C erisimi HAL sinirina tasindi; RP2350 tarafinda altta yine native Pico SDK I2C, DREQ ve DMA FIFO register yolu kullanilir.
- `Scheduler` ve `PreflightHealth` cekirdek siniflari test-first eklendi. Core0 sensor/RC/state/preflight/watchdog frekanslari scheduler'a baglandi; telemetry ve blackbox 50Hz tasiyici task uzerinde runtime rate gate kullanir. Core1 control loop `FLIGHT_LOOP_PERIOD_US=2000` tek kaynagindan turetilen deterministik 500Hz task olarak kalir.
- Sensor preflight sadece OK/FAIL degil; sensor age ve 0-100 kalite skoru uretir. `SensorPreflightEvaluator` IMU yok, health bozuk, kalite dusuk ve OK nedenlerini heap kullanmadan raporlar.
- `estimators/` altinda runtime fallback `ComplementaryEstimator`, hizli `BaroAltitudeEstimator` ve kapsamini dogru anlatan `BaroVerticalKalman` bulunur. Dikey Kalman filtresi 1D altitude/vertical-speed, 2x2 kovaryans ve baro spike gating kullanir. Estimator katmani buyuk `FlightData` yerine dar `EstimatorInput` tipini tuketir.
- Runtime parametreler PID/mixer/trim/reverse/servo/failsafe yaninda RC kanal esleme, MAVLink stream hizlari, blackbox log hizi ve preflight sensor kalite esigini kapsar.
- `TimingMonitor` max sureye ek olarak sabit-noktali EWMA ortalama, jitter ve deadline-miss sayaci tutar; timing ihlalleri blackbox kaydina eklenir.
- GitHub Actions tum push/PR'larda firmware build, native test, fault injection, software smoke ve high-severity static analysis calistirir. Fiziksel HIL smoke `workflow_dispatch` ile seri port verilerek manuel tetiklenir.
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
