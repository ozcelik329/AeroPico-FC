# AeroPico FC - Estimator / EKF Tasarim Notu

Bu not, GPS/otonom moda gecmeden once estimator altyapisinin nasil tasarlanacagini tarif eder.

## Hedef

Ilk hedef tam EKF degil; test edilebilir, hafif ve sabit kanat icin yeterli bir durum tahmini katmani kurmaktir.

Net kapsam: Bu bir tam-durum EKF degildir, yalnizca irtifa/dikey hiz icin 2-durumlu bir Kalman filtresidir.

## Asama 1 - EstimatedState

```cpp
struct EstimatedState {
    float rollDeg;
    float pitchDeg;
    float yawDeg;
    float altitudeM;
    float verticalSpeedMps;
    SensorHealth health;
    uint32_t timestamp;
    bool valid;
};
```

Durum: `EstimatedState` `src/types.h` icinde eklendi ve native testlerde kullaniliyor.

## Asama 2 - Complementary Estimator

- IMU attitude mevcut `SensorFusion`dan gelir.
- Baro altitude low-pass ile yumusatilabilir.
- Vertical speed baro turevinden uretilir.
- Health, sensor stale/timeout durumundan turetilir.

Durum: `src/estimators/ComplementaryEstimator.*` prototipi eklendi. Mevcut implementasyon attitude'u `FlightData` icinden alir, baro altitude icin low-pass uygular, vertical speed'i filtrelenmis altitude turevinden hesaplar ve sensor/failsafe durumuna gore `valid` bayragi uretir.

## Asama 3 - EKF Hazirligi

EKF'ye gecmeden once gerekli altyapi:

- Matris islemleri icin kucuk ve testli math katmani.
- Sensor input timestamp standardi.
- Measurement gating.
- Innovation logging.
- Reset/recovery davranisi.
- GPS input siniri. `src/drivers/gps/` altindaki NMEA parser yalnizca fix verisi uretir; EKF state guncellemesi yapmaz.

Durum: GPS GGA parser ve UART manager iskeleti eklendi, native testlerle dogrulandi ve `GPS_MODULE_ENABLED=0` ile varsayilan kapali tutuldu. GPS modulu gelince ilk adim parser'i bench'te dogrulamak, sonra `NavigationState`/EKF input katmanina test-first baglamaktir.

Durum tahmini notu:

- `BaroVerticalKalman`, baro olcumu ile altitude'u duzeltir; attitude uzerinden dunya Z eksenine projekte edilen ham ivmeolcer bilgisini surec girdisi olarak kullanir.
- `SensorFusion`, Madgwick/Mahony duzeltme agirligini sabit beta yerine ivme buyuklugu/titresim hatasina gore adaptif azaltir.
- Gyro sicaklik katsayisi boot kalibrasyonunda olculur, kalibrasyon blob'una kaydedilir ve fusion katmaninda kullanilir.
- IMU, manyetometre, barometre ve GPS kabiliyetleri bitmask ile raporlanir; olmayan fonksiyonun algoritmasi devreye alinmaz.

## ESP32-CAM Hazirligi

ESP32-CAM, V1.0 ucus kontrol karari vermemelidir. Kamera linki yalnizca yardimci telemetry/vision hazirligi olarak tutulur.

Durum: `src/drivers/camera/Esp32CamLink.*` link health iskeleti eklendi ve `ESP32_CAM_LINK_ENABLED=0` ile varsayilan kapali tutuldu. Aktif edilince ilk sorumlulugu UART link var/yok bilgisini raporlamak olmalidir; kontrol dongusuna dogrudan baglanmamalidir.

## EKF State Onerisi

Ilk EKF state seti:

- altitude
- vertical speed
- gyro bias z veya basit bias state

Daha sonra:

- position N/E
- velocity N/E
- yaw bias
- wind estimate

## Test Stratejisi

- Sabit altitude testi.
- Lineer tirmanis testi.
- Ani baro spike testi.
- Stale sensor testi.
- Innovation limit testi.

Mevcut native testler:

- Attitude kopyalama testi.
- Baro altitude filtreleme ve vertical speed testi.
- Gecersiz sensor health reddi.
- Eksik baro verisi davranisi.

## Uygulama Kurali

Estimator kodu `src/estimators/` altinda kalacak ve donanim suruculerine dogrudan baglanmayacak. Ucus akisine baglanmadan once native testleri olacak.
