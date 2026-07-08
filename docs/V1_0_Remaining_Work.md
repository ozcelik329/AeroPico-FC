# AeroPico FC V1.0 - Kalan Isler

Bu belge, otonom ucus ozellikleri haric AeroPico FC V1.0 icin kalan
muhendislik islerini izler. Mission, waypoint, RTL ve auto flight bu kapsama
dahil degildir.

Durum isaretleri:

- `[ ]` Bekliyor
- `[~]` Altyapisi var, urun seviyesinde tamamlanmadi
- `[H]` Fiziksel donanim gerektiriyor

## P0 - Determinizm ve Bellek Guvenligi

### [ ] V1-01 FreeRTOS statik gorev tahsisi

`SensorTask`, `FlightTask` ve `TelemetryTask` icin
`xTaskCreateStaticAffinitySet` veya cercevenin esdeger statik API'si
kullanilacak. Stack ve TCB alanlari derleme zamaninda ayrilacak.

Kabul kriterleri:

- Kritik gorevlerin olusturulmasinda heap kullanilmamasi.
- Stack high-water mark degerlerinin health/blackbox tarafindan raporlanmasi.
- Native testler ve Pico firmware derlemesinin gecmesi.

### [ ] V1-02 Data-centric global blackboard

`SensorState`, `VehicleState`, `RcInputState`, `ActuatorState`,
`NavigationState` ve system health verileri sabit boyutlu, typed topic'lerde
tutulacak. Her topic tek yazara ve coklu okuyucuya uygun tutarli snapshot
semantigine sahip olacak.

Kabul kriterleri:

- Core'lar arasi paylasilan state icin tek ve belgeli veri sahipligi.
- Reader tarafinda parcali/torn snapshot olusmamasi.
- Kritik yolda heap, blocking mutex veya veri kopyalama zinciri bulunmamasi.
- Wraparound ve eszamanlilik host testlerinin gecmesi.

### [ ] V1-03 Kritik veri yolunu lock-free yap

Mevcut blocking mutex kullanan ring buffer, kullanim desenine uygun sabit
boyutlu SPSC queue veya sequence-lock snapshot ile degistirilecek.

Kabul kriterleri:

- Producer ve consumer kritik yolunda blocking cagri bulunmamasi.
- Acquire/release bellek siralamasinin acikca tanimlanmasi.
- Overflow politikasinin sayac ve health fault ile gorunur olmasi.
- Stress ve wraparound testlerinin gecmesi.

### [ ] V1-04 Scheduler ve WCET kabul sistemi

Scheduler; release gecikmesi, calisma suresi, deadline miss, jitter, CPU load
ve task overrun verilerini pencere bazli olcecek. Maksimum degerler
sonsuz birikmek yerine rapor doneminde yenilenecek.

Kabul kriterleri:

- Her periyodik is icin budget ve deadline tanimi.
- Ortalama, maksimum, jitter, miss count ve CPU load raporu.
- Budget asiminda blackbox, MAVLink ve preflight health bildirimi.
- Sentetik overrun ve zaman sayaci wraparound testleri.

## P1 - Mimari Sinirlar ve Durum Yonetimi

### [ ] V1-05 Typed, statik Event Bus

Sensor fault, RC loss/recovery, arm denial, failsafe transition, timing
overrun ve battery warning olaylari sabit kapasiteli typed event bus
uzerinden yayinlanacak.

Kabul kriterleri:

- Heap, RTTI ve dinamik subscriber listesi kullanilmamasi.
- Kritik olaylar icin overflow politikasinin belirlenmesi.
- ISR tarafinin yalnizca veri/olay aktarmasi.
- Siralama, overflow ve subscriber testlerinin gecmesi.

### [ ] V1-06 Hierarchical flight state machine

Boot, Standby, PreflightBlocked, ReadyToArm, ArmedManual, Failsafe ve Safe
durumlari tek bir hiyerarsik durum makinesiyle yonetilecek. Arm/disarm ve
failsafe gecisleri farkli siniflara dagitilmayacak.

Kabul kriterleri:

- Her gecisin guard, action ve reason degerinin tanimli olmasi.
- Gecersiz gecislerin reddedilip raporlanmasi.
- Failsafe ve sensor recovery davranisinin deterministik olmasi.
- Tum durum/gecis tablosunun unit test ile kapsanmasi.

### [~] V1-07 FlightManager ve legacy FlightData'yi incelt

Blackboard gecisinden sonra `FlightManager` yalnizca ust seviye
orkestrasyon yapacak. `FlightData`, MAVLink/blackbox uyumluluk DTO'suna
indirgenecek; kontrol dongusu typed state okuyacak.

Kabul kriterleri:

- Control loop'un `FlightManager` getter zincirine bagimli olmamasi.
- Sensor, RC, control, failsafe ve publication sahipliklerinin ayrilmasi.
- Davranis esitligini koruyan entegrasyon testleri.

### [~] V1-08 HAL izolasyonunu tamamla

Core katmanindaki timer/GPIO erisimleri ve RP2350'a bagli output, PIO UART,
DMA sensor bus ve flash storage ayrintilari uygun platform backend'lerine
alinacak. PIO ve DMA gibi cipe ozel hizlandiricilar backend icinde korunacak.

Kabul kriterleri:

- `src/core` altinda Pico SDK veya Arduino donanim API bagimliligi olmamasi.
- Driver API'lerinin platform register tiplerini disariya sizdirmamasi.
- HAL mock'lariyla native davranis testleri.
- Soyutlama nedeniyle sensor/control critical path'inde olculebilir ek
  gecikme olusmamasi.

### [~] V1-09 Degistirilebilir gyro/mag/baro backend'leri

Sensor rolleri cihaz adindan bagimsiz arayuzlerle tanimlanacak. MPU6050,
HMC5883L ve BMP085 ilk RP2350 backend'leri olarak kalacak; yeni sensor
eklemek `SensorPipeline` degisikligi gerektirmeyecek.

Kabul kriterleri:

- Gyro/accelerometer, magnetometer ve barometer capability arayuzleri.
- WHO_AM_I, calibration, timing ve fault bilgisinin ortak sozlesmesi.
- Her rol icin fake backend ve driver contract testleri.

## P1 - Guvenlik ve Kayit

### [ ] V1-10 Binary zero-copy blackbox

Metin bicimlendirme yerine version'li, sabit boyutlu binary record'lar ve
onceden ayrilmis DMA/PIO aktarim kuyrugu kullanilacak.

Kabul kriterleri:

- Ucus kritik yolunda `snprintf`, heap ve blocking UART yazimi olmamasi.
- Record version, timestamp, sequence ve CRC alanlari.
- Queue overflow/drop sayacinin health ve MAVLink'e aktarilmasi.
- Host decoder, golden-vector ve bozuk kayit testleri.

### [~] V1-11 Gelismis safe/degraded mode ve actuator health

Failsafe; RC loss, sensor stale, estimator failure, timing failure ve dusuk
batarya nedenlerini ayri politikalara baglayacak. Geri bildirim donanimi
yokken komut yolu, output backend ve kaynak kullanimi en azindan izlenecek.

Kabul kriterleri:

- Failsafe reason bitmask ve latch/recovery politikasi.
- Safe output'un tum failure yollarinda garanti edilmesi.
- Output write/backend fault ve kaynak claim hatalarinin raporlanmasi.
- Tam sistem RC-loss ve sensor-loss entegrasyon testleri.

### [~] V1-12 Battery/brownout urunlestirmesi

ADC olcegi, divider orani, filtreleme, esik hysteresis'i ve reset nedeni
raporlama tamamlanacak. Fiziksel divider dogrulanana kadar battery arming
gate varsayilan olarak kapali kalacak.

Kabul kriterleri:

- ADC conversion ve divider hesabinin unit testleri.
- Low/critical/brownout durumlarinda hysteresis ve debounce.
- MAVLink, blackbox ve preflight reason entegrasyonu.
- `[H]` Multimetre ile ADC gerilim dogrulamasi.

### [~] V1-13 Ucus sinifi estimator guclendirmesi

EKF-lite; covariance propagation, gyro bias estimation, innovation gating,
sensor confidence ve reset/recovery davranisiyla gelistirilecek. Bu madde
navigation veya otonom ucus eklemeyecek.

Kabul kriterleri:

- Bilinen aci/hareket ve sensor dropout veri setleri.
- NaN/Inf ve covariance sinir kontrolleri.
- Innovation reject ve estimator reset olaylarinin raporlanmasi.
- Complementary estimator'a guvenli fallback.

## P1 - Test ve Urun Dogrulamasi

### [ ] V1-14 Sistem entegrasyon ve fault matrisi

Boot, arm, disarm, RC loss/recovery, sensor timeout, DMA timeout, estimator
failure, scheduler overrun, parameter corruption ve logger saturation
senaryolari tek bir tekrar edilebilir test matrisine baglanacak.

Kabul kriterleri:

- Her senaryoda beklenen state, output, event ve preflight reason tanimi.
- CI'da donanimsiz senaryolarin otomatik kosmasi.
- Fault injection aracinin test sonucunu makine okunur bicimde uretmesi.

### [H] V1-15 Fiziksel HIL ve bench runner

Self-hosted GitHub Actions runner veya kontrollu bench bilgisayari Pico 2'yi
flash edecek, seri portu izleyecek ve temel I/O testlerini calistiracak.

Kabul kriterleri:

- Firmware flash ve boot token kontrolu.
- SBUS UART0/GP1 alma ve RC loss/recovery testi.
- Servo 1000/1500/2000 us pulse olcumu.
- Sensor DMA/health ve blackbox kayit testi.
- Core 1 durduruldugunda watchdog reset testi.
- Battery ADC ve brownout esik testi.

### [H] V1-16 USB/boot soak ve uzun sureli kararlilik

Tekrarli power-cycle, USB reconnect ve en az 8 saatlik bench soak testi
yapilacak.

Kabul kriterleri:

- Boot basari orani ve reset reason kaydi.
- Stack high-water, queue drop, deadline miss ve sensor fault trendleri.
- Beklenmeyen reset, deadlock veya kalici sensor kaybi olmamasi.

## P2 - RP2350 Performans Calismalari

### [ ] V1-17 Olcume dayali bellek ve kod yerlestirme

Critical path fonksiyonlari icin XIP, SRAM ve uygun RP2350 scratch bank
yerlesimi map dosyasi ve profiler sonucuna gore secilecek.

Kabul kriterleri:

- Once/sonra WCET ve flash/RAM raporu.
- Core'lar arasi SRAM bank contention degerlendirmesi.
- Sadece olculebilir kazanc saglayan yerlesimlerin tutulmasi.

### [ ] V1-18 Fixed-point/LUT optimizasyon incelemesi

Sensor scaling, filtreleme ve sik kullanilan matematik islemleri hata butcesi
belirlenerek incelenecek. Float yerine fixed-point veya LUT ancak olculmus
gecikme kazanci ve kabul edilebilir sayisal hata varsa kullanilacak.

Kabul kriterleri:

- Sayisal hata toleransi ve golden-vector testleri.
- Pico 2 uzerinde cycle/WCET karsilastirmasi.
- Okunabilirligi azaltan fakat kazanc saglamayan mikro-optimizasyonlarin
  alinmamasi.

## V1.0 Cikis Kapisi

V1.0 adayi ancak su kosullarda etiketlenebilir:

- P0 maddelerinin tamami tamamlanmis.
- P1 yazilim maddelerinin tamami CI'da geciyor.
- P1 donanim maddelerinin bench kayitlari mevcut.
- Kritik/yuksek statik analiz bulgusu yok.
- Native, integration, smoke, fault-injection ve HIL testleri geciyor.
- Kritik task'larda heap allocation ve blocking I/O yok.
- Tum pre-arm retleri ve failsafe gecisleri kullaniciya acik nedenle
  raporlaniyor.
