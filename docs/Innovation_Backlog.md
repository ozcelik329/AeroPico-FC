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
- Adaptif Madgwick beta: ivme buyuklugu, gyro varyansi ve titresim seviyesine gore beta katsayisini ucus sirasinda ayarla.
- Titresim/kalite tabanli sensor confidence: IMU, mag, baro, battery ve GPS icin guven skoru, stale yas ve noise seviyesi uret.
- Akilli preflight explainability: sadece ARM olabilir/olamaz degil, kullaniciya net ve onceliklendirilmis sebep listesi ver.
- Hafif EKF-lite / altitude estimator: tam-durum EKF'ye gecmeden irtifa/dikey hiz icin ivme girdili test edilebilir estimator gelistir.
- Configurator ile gorsel kalibrasyon: IMU, manyetometre, RC range ve servo yon testlerini adim adim gorsellestir.
- Fault-injection odakli test altyapisi: sensor timeout, stale data, RC loss, brownout ve timing overrun senaryolarini CI/native testlerde uret.
- Blackbox analiz araci: ucus/bench loglarini timing, failsafe, sensor quality ve actuator output acisindan otomatik ozetle.
- Modul otomatik algilama: gyro/mag/baro/GPS/battery/RC var-yok ve alternatif sensor secimini boot senaryosuna bagla.
- Dusuk gecikmeli scheduler/timing profiler: release latency, WCET, jitter ve task overrun degerlerini olc ve raporla.
- RP2350 PIO/DMA/FPU hizli mimari: PIO/DMA/FPU avantajlarini olculu kullan, her optimizasyonu profiler ve testle dogrula.
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
| 9 | Adaptif Madgwick beta | Accel norm/vibration metrigine gore beta secen politika ve native test ekle. |
| 10 | Modul otomatik algilama | Boot'ta gyro/mag/baro/GPS/battery var-yok matrisi ve fallback senaryolari yaz. |
| 11 | Configurator gorsel kalibrasyon | Kalibrasyon komutlarini MAVLink'e baglamadan once UI akislarini netlestir. |
| 12 | Fault-injection test seti | Sensor stale, RC loss, brownout ve timing overrun test fixture'lari ekle. |
| 13 | Blackbox analiz araci | Log dosyasindan kalite/timing/failsafe ozet raporu ureten CLI tasarla. |
| 14 | RP2350 performans profili | PIO/DMA/FPU kullanimini profiler ciktilariyla olc ve dokumante et. |

## AeroPico Ayirt Edici Inovasyon Hedefleri

Bu liste V1.0 manuel ucus altyapisi urun seviyesine yaklastiktan sonra sirayla ele alinacak izlenebilir hedeflerdir:

1. Adaptif Madgwick beta.
2. Titresim/kalite tabanli sensor confidence.
3. Akilli preflight explainability.
4. Hafif EKF-lite / altitude estimator.
5. Configurator ile gorsel kalibrasyon.
6. Fault-injection odakli test altyapisi.
7. Blackbox analiz araci.
8. Modul otomatik algilama.
9. Dusuk gecikmeli scheduler/timing profiler.
10. RP2350 PIO/DMA/FPU avantajlarini olculu kullanan hizli mimari.

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
