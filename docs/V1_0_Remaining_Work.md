# AeroPico FC V1.0 - Kalan Isler

Bu belge, otonom ucus ozellikleri haric AeroPico FC V1.0 icin kalan
muhendislik islerini izler. Mission, waypoint, RTL ve auto flight bu kapsama
dahil degildir.

Durum isaretleri:

- `[ ]` Bekliyor
- `[~]` Altyapisi var, urun seviyesinde tamamlanmadi
- `[x]` Yazilim ve otomatik testleri tamamlandi
- `[H]` Fiziksel donanim gerektiriyor

## P0 - Determinizm ve Bellek Guvenligi

### [H] PDF-P0-1 Ucus sirasinda flash yazimini guvenli hale getir

Runtime parametre kaydi armed durumda kesin olarak reddedilecek. Disarmed
durumda flash erase/program islemi iki cekirdegin XIP guvenligini saglayan,
timeout ve hata raporu olan platform protokoluyle yapilacak.

Kabul kriterleri:

- Armed durumda `PARAM_SAVE` komutunun flash'a dokunmadan reddedilmesi.
- Core 1 calisirken guvenli flash islemi veya kontrollu core rendezvous.
- Basarisiz kayitta onceki gecerli blob'un korunmasi.
- Unit/fault testleri ve fiziksel heartbeat/GPIO bench dogrulamasi.

### [x] PDF-P0-2 Control loop frekansini tek kaynaga bagla

Kodun mevcut gercegi `FLIGHT_LOOP_PERIOD_US=2000`, yani 500 Hz'dir.
Dokuman, boot raporu, timing budget ve PID zaman adimi ayni sabitten
turetilecek; kalan 400 Hz ifadeleri kaldirilacak.

Kabul kriterleri:

- Kod ve aktif dokumanlarda tek bir control-loop frekansi.
- Periyot wraparound ve `dt` testleri.
- Pico build ve timing budget testlerinin gecmesi.

### [x] V1-01 FreeRTOS statik gorev tahsisi

`SensorTask`, `FlightTask` ve `TelemetryTask` icin
`xTaskCreateStaticAffinitySet` veya cercevenin esdeger statik API'si
kullanilacak. Stack ve TCB alanlari derleme zamaninda ayrilacak.
Stack high-water ve drop sayaclari runtime health blackbox kaydina baglandi.

Kabul kriterleri:

- Kritik gorevlerin olusturulmasinda heap kullanilmamasi.
- Stack high-water mark degerlerinin health/blackbox tarafindan raporlanmasi.
- Native testler ve Pico firmware derlemesinin gecmesi.

### [x] V1-02 Data-centric global blackboard

`SensorState`, `VehicleState`, `RcInputState`, `ActuatorState`,
`NavigationState` ve system health verileri sabit boyutlu, typed topic'lerde
tutulacak. Her topic tek yazara ve coklu okuyucuya uygun tutarli snapshot
semantigine sahip olacak.

Kabul kriterleri:

- Core'lar arasi paylasilan state icin tek ve belgeli veri sahipligi.
- Reader tarafinda parcali/torn snapshot olusmamasi.
- Kritik yolda heap, blocking mutex veya veri kopyalama zinciri bulunmamasi.
- Wraparound ve eszamanlilik host testlerinin gecmesi.

### [x] V1-03 Kritik veri yolunu lock-free yap

Mevcut blocking mutex kullanan ring buffer, kullanim desenine uygun sabit
boyutlu SPSC queue veya sequence-lock snapshot ile degistirilecek.

Kabul kriterleri:

- Producer ve consumer kritik yolunda blocking cagri bulunmamasi.
- Acquire/release bellek siralamasinin acikca tanimlanmasi.
- Overflow politikasinin sayac ve health fault ile gorunur olmasi.
- Stress ve wraparound testlerinin gecmesi.

### [x] V1-04 Scheduler ve WCET kabul sistemi

Scheduler; release gecikmesi, calisma suresi, deadline miss, jitter, CPU load
ve task overrun verilerini pencere bazli olcecek. Maksimum degerler
sonsuz birikmek yerine rapor doneminde yenilenecek.
Core0 scheduler task basina release latency, runtime ve deadline miss tutar;
core1 timing monitor pencere bazli WCET/jitter/load raporlar.

Kabul kriterleri:

- Her periyodik is icin budget ve deadline tanimi.
- Ortalama, maksimum, jitter, miss count ve CPU load raporu.
- Budget asiminda blackbox, MAVLink, event bus ve preflight health bildirimi.
- Sentetik overrun ve zaman sayaci wraparound testleri.

## P1 - Mimari Sinirlar ve Durum Yonetimi

### [x] V1-05 Typed, statik Event Bus

Sensor fault, RC loss/recovery, arm denial, failsafe transition, timing
overrun, battery warning ve blackbox drop olaylari sabit kapasiteli typed event bus
uzerinden yayinlanacak.

Kabul kriterleri:

- Heap, RTTI ve dinamik subscriber listesi kullanilmamasi.
- Kritik olaylar icin overflow politikasinin belirlenmesi.
- ISR tarafinin yalnizca veri/olay aktarmasi.
- Siralama, overflow ve subscriber testlerinin gecmesi.

### [x] V1-06 Hierarchical flight state machine

Boot, Standby, PreflightBlocked, ReadyToArm, ArmedManual, Failsafe ve Safe
durumlari tek bir hiyerarsik durum makinesiyle yonetilecek. Arm/disarm ve
failsafe gecisleri farkli siniflara dagitilmayacak.
Guard/reason gecisleri unit testlerle kapsandi; V1.0 manuel ucus kapsami icin
durum makinesi merkezi karar noktasi olarak kullaniliyor.

Kabul kriterleri:

- Her gecisin guard, action ve reason degerinin tanimli olmasi.
- Gecersiz gecislerin reddedilip raporlanmasi.
- Failsafe ve sensor recovery davranisinin deterministik olmasi.
- Tum durum/gecis tablosunun unit test ile kapsanmasi.

### [x] V1-07 FlightManager ve legacy FlightData'yi incelt

Blackboard gecisinden sonra `FlightManager` yalnizca ust seviye
orkestrasyon yapacak. `FlightData`, MAVLink/blackbox uyumluluk DTO'suna
indirgenecek; kontrol dongusu typed state okuyacak.

Kabul kriterleri:

- Control loop'un `FlightManager` getter zincirine bagimli olmamasi.
- Sensor, RC, control, failsafe ve publication sahipliklerinin ayrilmasi.
- Davranis esitligini koruyan entegrasyon testleri.

### [x] V1-08 HAL izolasyonunu tamamla

Core katmanindaki timer/GPIO erisimleri ve RP2350'a bagli output, PIO UART,
DMA sensor bus ve flash storage ayrintilari uygun platform backend'lerine
alinacak. PIO ve DMA gibi cipe ozel hizlandiricilar backend icinde korunacak.

SensorManager public I2C enjeksiyonu `IHALI2C` seviyesine indirildi. RP2350
backend kullanildiginda DMA hizli yolu korunur; generic HAL backend verilirse
raw I2C fallback calisir.
PIO UART, PWM output, ADC, timer ve debug GPIO erisimleri HAL/platform backend
sinirina tasindi. `RXManager`, donanim SBUS okuyucusunu `ISbusBackend`
sozlesmesi arkasindan kullanir; fake backend ile host testlenir. RP2350 PIO ve
DMA hizli yollari backend icinde korunur.

Kabul kriterleri:

- `src/core` altinda Pico SDK veya Arduino donanim API bagimliligi olmamasi.
- Driver API'lerinin platform register tiplerini disariya sizdirmamasi.
- HAL mock'lariyla native davranis testleri.
- Soyutlama nedeniyle sensor/control critical path'inde olculebilir ek
  gecikme olusmamasi.

### [x] V1-09 Degistirilebilir gyro/mag/baro backend'leri

Sensor rolleri cihaz adindan bagimsiz arayuzlerle tanimlanacak. MPU6050,
HMC5883L ve BMP085 ilk RP2350 backend'leri olarak kalacak; yeni sensor
eklemek `SensorPipeline` degisikligi gerektirmeyecek.

Gyro/mag/baro suruculeri cihaz dosyalarindan ayrildi; sensor bus enjeksiyonu
cihaz bagimsiz I2C sozlesmesine acildi. Gyro, mag ve baro rolleri icin host
contract testleri, hard-iron calibration testi, baro hata testi ve HAL I2C
testleri mevcut. Yeni sensor secimi V1.5 genisletme konusu olarak kalir; V1.0
manuel ucus altyapisi icin rol sinirlari tamamlandi.

Kabul kriterleri:

- Gyro/accelerometer, magnetometer ve barometer capability arayuzleri.
- WHO_AM_I, calibration, timing ve fault bilgisinin ortak sozlesmesi.
- Her rol icin fake backend ve driver contract testleri.

## P1 - Guvenlik ve Kayit

### [x] V1-10 Binary zero-copy blackbox

Metin bicimlendirme yerine version'li, sabit boyutlu binary record'lar ve
onceden ayrilmis PIO aktarim kuyrugu kullanilacak.

Kabul kriterleri:

- Ucus kritik yolunda `snprintf`, heap ve blocking UART yazimi olmamasi.
- Record version, timestamp, sequence ve CRC alanlari.
- Queue overflow/drop sayacinin health ve MAVLink'e aktarilmasi.
- Host decoder, golden-vector ve bozuk kayit testleri.

### [x] V1-11 Gelismis safe/degraded mode ve actuator health

Failsafe; RC loss, sensor stale, estimator failure, timing failure ve dusuk
batarya nedenlerini ayri politikalara baglayacak. Geri bildirim donanimi
yokken komut yolu, output backend ve kaynak kullanimi en azindan izlenecek.

RC, sensor, estimator, timing, brownout ve actuator fault nedenleri merkezi
`FailsafeManager` bitmask'ine baglandi. Fiziksel actuator geri bildirimi ve
tam sistem bench kaniti V1-15'e bagli kalir.
MANUAL/STABILIZE ayrimi `ControlMode` ile acik hale getirildi; MANUAL modda PID
duzeltmeleri bypass edilir, STABILIZE mevcut stabilizasyon yolunu kullanir.
Safe output yolu, actuator readiness ve failsafe reason bitmask native testlerle
kapsandi. Fiziksel servo geri bildirimi olmayan kartta kalan dogrulama V1-15
bench maddesidir.

Kabul kriterleri:

- Failsafe reason bitmask ve latch/recovery politikasi.
- Safe output'un tum failure yollarinda garanti edilmesi.
- Output write/backend fault ve kaynak claim hatalarinin raporlanmasi.
- Tam sistem RC-loss ve sensor-loss entegrasyon testleri.

### [x] V1-12 Battery/brownout urunlestirmesi

ADC olcegi, divider orani, filtreleme, esik hysteresis'i ve reset nedeni
raporlama tamamlanacak. Fiziksel divider dogrulanana kadar battery arming
gate varsayilan olarak kapali kalacak.
Battery warning/brownout olaylari event bus, preflight, failsafe ve blackbox
runtime health akisina baglandi. Filtreleme, low-voltage debounce, recovery
hysteresis ve brownout latch native testlerle kapsandi. Gercek divider/pin
dogrulamasi donanimdadir.

Kabul kriterleri:

- ADC conversion ve divider hesabinin unit testleri.
- Low/critical/brownout durumlarinda hysteresis ve debounce.
- MAVLink, blackbox ve preflight reason entegrasyonu.
- `[H]` Multimetre ile ADC gerilim dogrulamasi.

### [x] V1-13 Ucus sinifi estimator guclendirmesi

`BaroVerticalKalman`; covariance propagation, gyro bias estimation, innovation gating,
sensor confidence ve reset/recovery davranisiyla gelistirilecek. Bu madde
navigation veya otonom ucus eklemeyecek.
`BaroVerticalKalman`; innovation gate, covariance sinirlari, NaN/Inf guard, tekrarli
measurement reject sonrasi stale health, missing-baro prediction ve
complementary fallback ile host testlidir. Bu V1.0 icin altitude/attitude
stabilizasyon altyapisini kapatir; tam navigation EKF V1.5/V2.0 kapsamindadir.

Kabul kriterleri:

- Bilinen aci/hareket ve sensor dropout veri setleri.
- NaN/Inf ve covariance sinir kontrolleri.
- Innovation reject ve estimator reset olaylarinin raporlanmasi.
- Complementary estimator'a guvenli fallback.

## P1 - Test ve Urun Dogrulamasi

### [x] V1-14 Sistem entegrasyon ve fault matrisi

Boot, arm, disarm, RC loss/recovery, sensor timeout, DMA timeout, estimator
failure, scheduler overrun, parameter corruption ve logger saturation
senaryolari tek bir tekrar edilebilir test matrisine baglandi. Fault injection
CI'da makine okunur JSON rapor uretiyor.

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

### [H] V1-17 Olcume dayali bellek ve kod yerlestirme

Critical path fonksiyonlari icin XIP, SRAM ve uygun RP2350 scratch bank
yerlesimi map dosyasi ve profiler sonucuna gore secilecek.

Kritik PID/fusion ve flash-programlama yollarinda RAM yerlestirme basladi.
Donanim WCET/jitter olcumu olmadan daha agresif yerlestirme yapilmayacak.
Yazilim kapilari ve static stack/health raporu tamamlandi; once/sonra cycle ve
SRAM bank contention kaniti Pico 2 uzerinde bench ile alinmalidir.

Kabul kriterleri:

- Once/sonra WCET ve flash/RAM raporu.
- Core'lar arasi SRAM bank contention degerlendirmesi.
- Sadece olculebilir kazanc saglayan yerlesimlerin tutulmasi.

### [x] V1-18 Fixed-point/LUT optimizasyon incelemesi

Sensor scaling, filtreleme ve sik kullanilan matematik islemleri hata butcesi
belirlenerek incelenecek. Float yerine fixed-point veya LUT ancak olculmus
gecikme kazanci ve kabul edilebilir sayisal hata varsa kullanilacak.

PWM stick ve servo range donusumleri bolmesiz, inline ve testli yardimcilara
tasindi. Daha agresif fixed-point/LUT kullanimi Pico uzerinde WCET kaniti
olmadan alinmayacak.
SBUS raw-to-PWM mapping float kullanmadan integer arithmetic ile testli hale
getirildi; sensor fusion double-precision matematikten `sqrtf/asinf/atan2f`
yoluna tasindi. Olculmemis mikro-optimizasyonlar alinmadi.

Kabul kriterleri:

- Sayisal hata toleransi ve golden-vector testleri.
- Pico 2 uzerinde cycle/WCET karsilastirmasi.
- Okunabilirligi azaltan fakat kazanc saglamayan mikro-optimizasyonlarin
  alinmamasi.

## V1.0 Cikis Kapisi

V1.0 adayi ancak su kosullarda etiketlenebilir:

- Harici inceleme takip belgesindeki tum P0/P1 yazilim bulgulari kapali.
- P0 maddelerinin tamami tamamlanmis.
- P1 yazilim maddelerinin tamami CI'da geciyor.
- P1 donanim maddelerinin bench kayitlari mevcut.
- Kritik/yuksek statik analiz bulgusu yok.
- Native, integration, smoke, fault-injection ve HIL testleri geciyor.
- Kritik task'larda heap allocation ve blocking I/O yok.
- Tum pre-arm retleri ve failsafe gecisleri kullaniciya acik nedenle
  raporlaniyor.
