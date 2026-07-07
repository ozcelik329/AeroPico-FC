# AeroPico FC - Inovasyon ve Ileri Gelistirme Backlog

Bu belge GPS/otonom moda hemen girmeden, ticari urun kalitesi oturduktan sonra AeroPico'yu PX4/ArduPilot benzeri kontrolculerden farklilastirabilecek fikirleri toplar.

## Once Temel Bitmeli

Asagidaki fikirler, Safha 2-5 tamamlanip bench testler tutarli hale gelmeden uygulanmamalidir:

- Kontrol kalitesi testleri
- MAVLink callback mimarisi
- Sensor health / blackbox gorunurlugu
- Timing budget raporlama
- CI test/build gecisi
- Donanim bench checklist

## EKF / Estimator Altyapisi

Hedef: GPS, baro, IMU ve manyetometre verilerini ileride tek bir durum tahmini katmaninda birlestirmek.

Baslangic onerisi:

1. `src/estimators/` katmani ac.
2. Ilk implementasyon tam EKF degil, test edilebilir bir complementary/state estimator olsun.
3. Estimator girdisi donanima bagli olmasin; `SensorBuffer` ve ileride `GpsSample` gibi sade veri tipleri alsin.
4. Cikti `EstimatedState` olsun: roll, pitch, yaw, altitude, vertical speed, health, timestamp.
5. EKF'ye gecis icin matris islemleri ayri ve testli bir mini math katmaninda tutulmali.

## Ticari Urun Farki Yaratabilecek Fikirler

- Sensor health skoru: sadece OK/FAIL degil, guven skoru ve stale yasini raporla.
- Otomatik bench test modu: servo sweep, RC channel check, sensor motion check tek komutla calissin.
- Adaptive filtering: ucus modu ve titreşim seviyesine gore median/IIR parametrelerini degistir.
- Timing-aware control: loop jitter artarsa PID cikisini yumuşat veya degraded mode'a gec.
- Blackbox kalite skoru: her log dosyasina sensor health, timing ihlali ve failsafe sayaci ekle.
- Parametre profilleri: "maiden flight", "stable", "agile" gibi kaydedilebilir ayar setleri.
- GCS pre-arm checklist entegrasyonu: eksik sensor/RC/failsafe durumlarini GCS'e acik mesajla bildir.
- Ucus sonrasi otomatik ozet: max roll/pitch, failsafe sayisi, sensor stale sayisi, timing max degerleri.

## En Yuksek Getiri Veren Mini Faz

| Oncelik | Fikir | Ilk Adim |
|---|---|---|
| 1 | Sensor guven skoru | `SensorHealth` yanina kalite skoru ve stale yas alanlari ekle. |
| 2 | Bench auto-test modu | MAVLink komutu veya param ile servo/RC/sensor test modunu tetikle. |
| 3 | Adaptive filtering | IMU varyansina gore median/IIR parametrelerini secen politika yaz. |
| 4 | Timing-aware control | `TimingBudgetStatus` ile PID cikisini yumusat veya degraded mode'a gec. |
| 5 | EKF-lite altitude estimator | ComplementaryEstimator'a acceleration-Z opsiyonel girdisi ekle. |
| 6 | Pre-arm explainability | `PreArmStatus` veri tipi ve MAVLink STATUSTEXT mesajlari ekle. |
| 7 | Blackbox flight score | Failsafe/stale/timing/attitude ozet accumulator yaz. |
| 8 | Parametre profilleri | Maiden/stable/agile profil setleri icin storage tasarla. |

## PX4/ArduPilot'tan Farklilasma Yonleri

- Daha hafif ve sabit kanat odakli cekirdek.
- Pico/RP2350 hedefinde daha dusuk maliyetli ama gorunurlugu yuksek sistem.
- Bench-first gelistirme: her kritik ozellik once masa testiyle dogrulanir.
- Kodun kucuk, okunabilir ve egitim/gelistirme dostu kalmasi.

## Simdilik Yapilmayacaklar

- Tam waypoint navigation
- Return-to-home
- TECS/L1 tam otonom kontrol
- SITL/HIL buyuk sim altyapisi
- Çok platformlu multirotor mixer

Bu ozellikler temel ucus guvenligi ve test kapsami oturduktan sonra tekrar ele alinacak.
