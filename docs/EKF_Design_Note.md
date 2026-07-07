# AeroPico FC - Estimator / EKF Tasarim Notu

Bu not, GPS/otonom moda gecmeden once estimator altyapisinin nasil tasarlanacagini tarif eder.

## Hedef

Ilk hedef tam EKF degil; test edilebilir, hafif ve sabit kanat icin yeterli bir durum tahmini katmani kurmaktir.

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
