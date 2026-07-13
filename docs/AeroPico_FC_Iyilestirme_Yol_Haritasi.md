# AeroPico FC - Kapsamli Muhendislik Iyilestirme Yol Haritasi

Tarih: 2026-07-07

## Hedef

AeroPico FC kod tabanini ucus guvenligi, deterministik zamanlama, test edilebilirlik, sensor sagligi ve MAVLink/GCS entegrasyonu acisindan ticari sinifa yaklastirmak.

Bu dosya, kapsamli statik inceleme raporundaki bulgular bizim guncel kod tabanimizla yeniden karsilastirilarak uyarlanmistir. Raporun bazi bulgulari bu dalda artik cozulmus durumdadir; bu nedenle asagidaki liste "guncel uygulanabilir plan" olarak izlenmelidir.

## Guncel Durum Ozeti

Tamamlanan veya guncel kodda cozulmus gorunenler:

- SBUS GP1 artik `Serial1` ile UART0 RX uzerinden okunuyor.
- BootLogger `setup()` akisina bagli.
- FreeRTOS stack overflow ve malloc failed hook'lari mevcut.
- `SensorFusion::computeAngles()` icinde `asin()` argumani clamp ediliyor.
- 6-DOF `updateIMU()` ivme duzeltmesi mevcut.
- `RC_CHANNELS_OVERRIDE` artik `FlightManager::setRCOverride()` akisini tetikliyor.
- MPU6050 gyro/accel icin `RunningMedian` filtresi eklendi.
- `SensorHealth` ile stale/invalid sensor verisi fusion'a sokulmuyor.
- Native unit test altyapisi calisiyor.
- `filters/` ve `board/` katmanlari baslatildi; `RunningMedian` filtre katmanina tasindi.
- SBUS pin/UART uyumu `board/PinValidation.h` ile derleme zamaninda kontrol ediliyor.

Hala kritik veya yuksek oncelikli kalanlar:

1. Donanimda SBUS, heartbeat, boot health ve sensor health davranisi dogrulanmali.
2. CI ortaminda yeni test setinin gectigi dogrulanmali.
3. Kalibrasyon storage API'si flash implementasyonuna baglanmali.

## Safha 1 - Ucus Kritik Duzeltmeler

Hedef: Boot, RC girisi, kontrol dongusu ve sensor verisi icin once guvenli temel davranisi garanti etmek.

| ID | Durum | Gorev | Dosyalar |
|---|---|---|---|
| F1-01 | Tamamlandi | FreeRTOS SMP / `multicore_launch_core1()` karisimini tek modele indir | `main.cpp`, `SystemTimer.cpp/h` |
| F1-02 | Tamamlandi | SBUS GP1 icin UART0/`Serial1` kullan | `RX.cpp`, `config.h` |
| F1-03 | Tamamlandi | Boot saglik raporunu `setup()` akisina bagla | `main.cpp`, `BootLogger.*` |
| F1-04 | Tamamlandi | FreeRTOS stack/malloc hook'larini ekle | `main.cpp`, `platformio.ini` |
| F1-05 | Tamamlandi | `asin()` clamp ile NaN riskini azalt | `SensorFusion.cpp` |
| F1-06 | Tamamlandi | 6-DOF ivme duzeltmesini koru | `SensorFusion.cpp` |
| F1-07 | Tamamlandi | Median filtre + sensor stale korumasi ekle | `RunningMedian.h`, `Sensors.*`, `types.h` |
| F1-08 | Tamamlandi | PID cikis limitleri ve conditional anti-windup ekle | `PID.*`, `SystemTimer.cpp`, `test/test_pid/` |
| F1-09 | Tamamlandi | I2C/DMA timeout ekle | `Sensors.cpp` |
| F1-10 | Tamamlandi | `-Ofast` yerine `-O2 -fno-fast-math` kullan | `platformio.ini` |
| F1-11 | Tamamlandi | Filtreleri `filters/` katmanina ayir | `filters/RunningMedian.h`, `Sensors.h` |
| F1-12 | Tamamlandi | SBUS pin/UART derleme zamani dogrulamasi ekle | `board/PinValidation.h`, `RX.cpp`, `config.h` |
| F1-13 | Tamamlandi | Watchdog beslemesini ucus dongusu sagligina bagla | `WatchdogGate.*`, `main.cpp`, `test/test_watchdog_gate/` |

Cikis kriteri:

- Pico firmware derlenir.
- Native testler gecer.
- Core 1 heartbeat FreeRTOS gorevinden guncellenir.
- Stale/invalid sensor verisi attitude hesabina girmez.
- RC sinyali kesilince failsafe degerleri uygulanir.

## Safha 2 - Kontrol Kalitesi ve Test

Hedef: Saturasyon, ruzgar sapmasi ve sert firlatma gibi durumlarda kontrolcuyu ongorulebilir hale getirmek.

| ID | Durum | Gorev | Dosyalar |
|---|---|---|---|
| F2-01 | Tamamlandi | PID'e `outputMin/outputMax` ve `integralLimit` ekle | `PID.h/.cpp` |
| F2-02 | Tamamlandi | Conditional integration anti-windup uygula | `PID.cpp` |
| F2-03 | Tamamlandi | `dt <= 0` ve anormal buyuk `dt` korumasi ekle | `PID.cpp` |
| F2-04 | Tamamlandi | PID unit testleri ekle | `test/test_pid/` |
| F2-05 | Tamamlandi | FixedWingMixer limit testleri ekle | `FixedWingMixer.*`, `test/test_mixer/` |
| F2-06 | Tamamlandi | Failsafe durumunu arm/disarm mantigina dogrudan bagla | `FlightModeController.*`, `FlightManager.cpp`, `test/test_controllers/` |
| F2-07 | Tamamlandi | `ThreadSafeRingBuffer` const cast kullanimini `mutable` ile temizle | `ThreadSafeRingBuffer.h` |
| F2-08 | Tamamlandi | SensorFusion bilinen aci testleri ekle | `SensorFusion.*`, `test/test_sensor_fusion/` |
| F2-09 | Tamamlandi | FlightManager'dan RC ve sensor pipeline sorumluluklarini ayir | `FlightManager.*`, `RCPipeline.*`, `SensorPipeline.*` |
| F2-10 | Tamamlandi | RC override/failsafe pipeline testlerini ekle | `test/test_rc_pipeline/` |

## Safha 3 - MAVLink, Parametre ve Gevsek Baglilik

Hedef: GCS ile guvenilir iki yonlu kontrol ve parametre akisi kurmak.

| ID | Durum | Gorev | Dosyalar |
|---|---|---|---|
| F3-01 | Tamamlandi | `MavlinkHandler` icindeki global `flightManager` bagimliligini callback arayuzune tasima | `MavlinkHandler.*`, `main.cpp` |
| F3-02 | Tamamlandi | RC override timeout ekle | `FlightManager.*`, `config.h` |
| F3-03 | Tamamlandi | ParamManager'i MAVLink yonlendiricisine tam bagla | `ParamManager.*`, `MavlinkHandler.cpp` |
| F3-04 | Tamamlandi | PID gain parametrelerini runtime callback ile uygulama | `ParamManager.*`, `SystemTimer.cpp`, `main.cpp` |
| F3-05 | Tamamlandi | MAVLink override unit test / host test altyapisi kur | `test/test_mavlink_override/`, `MavlinkHandler.*` |
| F3-06 | Tamamlandi | ParamManager host testleri ekle | `test/test_param_manager/`, `ParamManager.*` |

## Safha 4 - Sensor Sagligi ve Kalibrasyon

Hedef: Sensor ofsetleri, timeout ve gecersiz veri durumlarindan kaynaklanan ucus hatalarini azaltmak.

| ID | Durum | Gorev | Dosyalar |
|---|---|---|---|
| F4-01 | Tamamlandi | I2C/DMA timeout ve transfer abort ekle | `Sensors.cpp` |
| F4-02 | Tamamlandi | Gyro/accel boot kalibrasyonunu test edilebilir hale getir | `Sensors.*`, `types.h` |
| F4-03 | Kismen tamamlandi | Barometre ve manyetometre icin health durumu uret | `Sensors.*`, `types.h` |
| F4-04 | Tamamlandi | Sensor health bilgisini MAVLink SYS_STATUS ve blackbox'a tasima | `MavlinkHandler.*`, `Blackbox.*`, `main.cpp` |
| F4-05 | Tamamlandi | Manyetometre hard-iron kalibrasyon iskeleti | `Sensors.*`, `types.h` |
| F4-06 | Tamamlandi | Kalibrasyon parametrelerini kalici saklama arastirmasi | `docs/Calibration_Persistence_Research.md` |
| F4-07 | Tamamlandi | Kalibrasyon storage API ve native testlerini ekle | `storage/CalibrationStorage.*`, `test/test_calibration_storage/` |

## Safha 5 - Zamanlama, CI ve Bakim Disiplini

Hedef: Her degisiklikten sonra ayni hatalarin geri gelmesini engellemek.

| ID | Durum | Gorev | Dosyalar |
|---|---|---|---|
| F5-01 | Tamamlandi | Timing budget ihlallerini MAVLink/blackbox'a raporla | `SystemTimer.*`, `Blackbox.*`, `MavlinkHandler.*` |
| F5-02 | Tamamlandi | `LOOP_TIME_MS` / `LOOP_TIME` isimlerini netlestir | `config.h`, `def.h`, `SystemTimer.h` |
| F5-03 | Tamamlandi | Kullanilmayan `RingBuffer.h` ve `SerialCom.*` icin karar ver | `core/`, `telemetry/` |
| F5-04 | Tamamlandi | GitHub Actions veya yerel CI: `pio test -e native` + `pio run -e pico` | `.github/workflows/ci.yml` |
| F5-05 | Tamamlandi | Bench test ve ilk ucus checklist dokumanlari ekle | `docs/Bench_Test_Checklist.md`, `docs/First_Flight_Checklist.md` |
| F5-06 | Tamamlandi | Proje yapisi dokumanini guncel tut | `docs/Project_Structure.md` |
| F5-07 | Tamamlandi | FreeRTOS panic hook'larinda watchdog beslemeyi kaldir | `main.cpp` |
| F5-08 | Tamamlandi | Pico derleme uyarilarini azalt: time-critical anotasyonlarini tek yerde tut | `PID.h`, `SensorFusion.h` |

## Safha 6 - Ucus Modlari ve Uzun Vadeli Ozellikler

Hedef: Temel stabilizasyon oturduktan sonra sabit kanatli IHA icin kullanisli modlar eklemek.

| ID | Gorev |
|---|---|
| F6-01 | MANUAL / STABILIZE / FBWA mod ayrimini gercek cikislara bagla |
| F6-02 | Baro tabanli altitude hold |
| F6-03 | GPS parser ve home konumu |
| F6-04 | Return-to-home altyapisi |
| F6-05 | Batarya voltaj izleme ve brownout safe mode |
| F6-06 | Binary blackbox veya daha verimli log formati |
| F6-07 | GCS UI icin sensor health, arming state, flight mode ekranlari |

## Safha 8 - Urun Seviyesi Mimari Sertlestirme

Hedef: Mevcut calisan cekirdegi, daha profesyonel FC mimarisine yaklastirmak.

| ID | Durum | Gorev | Dosyalar |
|---|---|---|---|
| F8-01 | Tamamlandi | FlightManager'i pipeline tabanli orkestratore indir | `FlightManager.*`, `RCPipeline.*`, `SensorPipeline.*`, `StatePublisher.*` |
| F8-02 | Tamamlandi | `ControlPipeline` ile controller/mixer hazirliklarini ayir | `core/control/ControlPipeline.*`, `core/control/FlightControlTask.*`, `core/control/ControlLoopExecutor.*`, `core/TimingMonitor.*` |
| F8-03 | Tamamlandi | `FailsafeManager` ile failsafe kararlarini merkezilestir | `core/FailsafeManager.*`, `test/test_failsafe_manager/` |
| F8-04 | Tamamlandi | `FlightData` yerine `RcInputState`, `VehicleState`, `ActuatorState`, `NavigationState` ayrimini genislet | `types.h`, `core/StatePublisher.*`, `test/test_state_publisher/` |
| F8-05 | Basladi | Gercek HAL katmani tasarla: GPIO/PWM/I2C/UART/ADC/TIMER | `hal/`, `hal/rp2350/` |
| F8-06 | Tamamlandi | Multi-rate scheduler tasarimi: 400/200/100/50/20/10/5/1Hz | `core/Scheduler.*`, `test/test_scheduler/`, `main.cpp` |
| F8-07 | Tamamlandi | Runtime parametre kapsamlarini genislet: servo reverse, trim, mixer, failsafe, RC | `telemetry/ParamManager.*`, `test/test_param_manager/` |
| F8-08 | Basladi | Boot sequence, driver registration, dependency graph, health check ve self-test akisini kod seviyesinde ayir | `core/PreflightHealth.*`, `core/BatteryMonitor.*`, `test/test_preflight/`, `test/test_battery_monitor/` |
| F8-09 | Tamamlandi | Scheduler'i telemetry/log/health akisina bagla | `main.cpp`, `core/Scheduler.*` |
| F8-10 | Tamamlandi | Preflight sonucunu arm kapisina bagla | `main.cpp`, `FlightModeController.*`, `FlightManager.*` |
| F8-11 | Kismen tamamlandi | Sensor davranislarini role gore ayir: gyro, mag, baro | `drivers/sensors/gyro/`, `drivers/sensors/mag/`, `drivers/sensors/baro/`, `test/test_sensor_drivers/` |

## Safha 7 - Inovasyon ve Estimator Hazirligi

Hedef: GPS/otonom moda hemen girmeden, ileride EKF ve urun farklilastirmasi icin temiz zemin hazirlamak.

| ID | Durum | Gorev | Dosyalar |
|---|---|---|---|
| F7-01 | Tamamlandi | Inovasyon backlog dokumani ekle | `docs/Innovation_Backlog.md` |
| F7-02 | Tamamlandi | Estimator klasorunu ayir ve kurallari yaz | `src/estimators/README.md` |
| F7-03 | Tamamlandi | `EstimatedState` veri tipini tasarla | `types.h` |
| F7-04 | Tamamlandi | Complementary altitude/attitude estimator prototipi | `src/estimators/ComplementaryEstimator.*`, `test/test_complementary_estimator/` |
| F7-05 | Tamamlandi | EKF tasarim notu ve test stratejisi | `docs/EKF_Design_Note.md` |
| F7-06 | Backlog | Ayirt edici inovasyon hedeflerini izle: adaptif Madgwick, sensor confidence, preflight explainability, EKF-lite altitude, gorsel kalibrasyon, fault injection, blackbox analiz, modul otomatik algilama, timing profiler, RP2350 PIO/DMA/FPU hizli mimari | `docs/Innovation_Backlog.md` |

## Guncel Dogrulama

- 2026-07-07 mimari ayrim sonrasi dogrulama: `pio test -e native` sonucu 65/65 test basarili.
- 2026-07-07 mimari ayrim sonrasi dogrulama: `pio run -e pico` basarili.
- 2026-07-08 sensor I2C HAL gecisi sonrasi dogrulama: `pio test -e native` sonucu 73/73 test basarili; `pio run -e pico` basarili.
- 2026-07-08 SensorDmaBus ayrimi ve core0 multi-rate scheduler sonrasi dogrulama: `pio test -e native` sonucu 73/73 test basarili; `pio run -e pico` basarili.
- 2026-07-08 battery/brownout ADC altyapisi sonrasi dogrulama: `pio test -e native` sonucu 74/74 test basarili; `pio run -e pico` basarili.
- Donanim elde olmadigi icin heartbeat, SBUS UART0/GP1, sensor health ve blackbox alanlari sahada beklemede.
- GitHub Actions dosyasi mevcut; uzak CI sonucu repo GitHub'a baglandiktan sonra dogrulanmali.

## Oncelikli Sonraki Isler

1. Donanimda FreeRTOS FlightTask heartbeat ve boot davranisini dogrula.
2. Donanimda SBUS UART0/GP1 okumasini dogrula.
3. Donanimda sensor health ve blackbox alanlarini dogrula.
4. CI ortaminda yeni test setinin gectigini dogrula.
5. Battery ADC pinini ve voltaj bolucu oranini donanim uzerinde dogrula; `BATTERY_ADC_ENABLED` sonra acilacak.
6. Kalibrasyon storage API'sini RP flash veya LittleFS implementasyonuna bagla.
7. ParamManager kalici saklama entegrasyonunu storage katmani uzerinden tasarla.
8. HIL/fault-injection CI akisina fiziksel smoke test bagla.
9. Sensor quality skoru ve acik pre-arm nedenlerini genislet.
10. `BaroVerticalKalman` ve altitude estimator prototipini gelistir.
11. Inovasyon backlog'unu estimator ve timing sonuclarina gore onceliklendir.
