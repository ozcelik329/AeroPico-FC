# AeroPico FC - Harici Inceleme Bulgulari Takibi

Kaynak: `41097f74-7943-4643-977e-95fad9184307.pdf`

Inceleme tarihi: 2026-07-09

Bu belge, harici mimari inceleme raporundaki hata bulgularini mevcut
`pico2-catalyi` koduna karsi dogrular ve kapanislarini izler. Kaynak rapordaki
bir madde, kod ve test ile dogrulanmadan tamamlanmis sayilmaz.

Durumlar:

- `ACIK`: Bulgu mevcut ve duzeltme bekliyor.
- `KISMEN`: Altyapi veya duzeltmenin bir bolumu mevcut.
- `DONANIM`: Kapanis icin fiziksel bench/HIL gerekiyor.
- `KAPALI`: Kod ve test kaniti mevcut.
- `DONANIM`: Yazilim kapanmis, fiziksel bench kaniti bekliyor.

## P0 - Ucus Guvenligi

| ID | Durum | Dogrulama | Yapilacak | Kapanis kaniti |
|---|---|---|---|---|
| PDF-P0-1 | DONANIM | Armed save kapisi ve SDK `flash_safe_execute` iki-core protokolu uygulandi; native ret testi ve Pico build gecti. | Fiziksel heartbeat/GPIO bench kaniti. | `test_param_manager`, Pico build ve bench kaydi. |
| PDF-P0-2 | KAPALI | Kod, boot ve aktif muhendislik dokumanlari `FLIGHT_LOOP_PERIOD_US=2000` / 500 Hz ile uyumlu. CI mimari kapisi eski 400 Hz ifadesini engelliyor. | Yok. | Architecture policy, native testler ve Pico build. |

## P1 - Mimari ve Gercek Zaman

| ID | Durum | Dogrulama | Yapilacak | V1.0 baglantisi |
|---|---|---|---|---|
| PDF-P1-1 | KAPALI / DONANIM | `RP2350UART` gercek `IHALUART` adapter'i olarak eklendi; PioUart da IRQ destekli `IHALUART` buffer write sozlesmesini sagliyor. `RXManager`, SBUS kaynagini `ISbusBackend` sozlesmesi arkasindan okuyor ve fake backend ile host testleniyor. | SBUS UART0/GP1 fiziksel bench testini V1-15'te kos. | `test_rx_manager`, `test_sbus_mapper`, SBUS bench |
| PDF-P1-2 | KAPALI | `ComplementaryEstimator`, `SensorPipeline` icinde dikey Kalman gecersiz oldugunda runtime fallback olarak kullaniliyor. | Yok. | `test_sensor_pipeline`, estimator testleri. |
| PDF-P1-3 | KAPALI | `SensorManager` public I2C bagimliligi `IHALI2C` sozlesmesine indirildi; RP2350 backend kullanildiginda DMA fast path korunuyor, generic HAL backend icin raw I2C fallback var. | Yeni sensor secim katmani V1.5 kapsamina alinabilir. | `V1-08`, `V1-09`, native HAL testleri |
| PDF-P1-4 | KAPALI | Aux mag/baro DMA okuma busy-wait yerine baslat/poll/tamamla state akisi kullaniyor; `tight_loop_contents()` sensör yolundan kalkti. | Donanimda mag/baro timing ve fault davranisini dogrula. | `V1-04`, `V1-09`, Pico build |
| PDF-P1-5 | KAPALI | Health raporu sonrasi reset istegi Core 1'e atomik aktariliyor; pencereyi yalnizca sahibi olan control core sifirliyor. | Yok. | Pico build ve timing/watchdog testleri. |
| PDF-P1-6 | KAPALI | Core/control timing yolunda dogrudan `Serial` kalmadi; `SensorManager` loglari Logger uzerinden geciyor. Kritik dongude blocking formatlama yok. | Telemetry/utility log backend'i ileride event sink'e tasinabilir. | Architecture scan, native testler, Pico build |
| PDF-P1-7 | KAPALI | Kullanilmayan Adafruit bagimliliklari kaldirildi; temiz Pico dependency graph ve build gecti. | Yok. | Pico build. |
| PDF-P1-8 | KAPALI | Sinif ve dosyalar kapsamini dogru anlatan `BaroVerticalKalman` adina tasindi. | Yok. | Estimator ve pipeline testleri. |

## P2 - Hijyen ve Dayaniklilik

| ID | Durum | Dogrulama | Yapilacak | Kapanis kaniti |
|---|---|---|---|---|
| PDF-P2-1 | KAPALI | Legacy PID tanimlari kaldirildi. | Yok. | PID/control testleri ve Pico build. |
| PDF-P2-2 | KAPALI | Kullanilmayan GPIO/SPI gelecek-vaadi arayuzleri kaldirildi. | Yok. | Pico build. |
| PDF-P2-3 | KAPALI | Parametre listesi 20 ms aralikli, adimlanan sabit durum makinesiyle gonderiliyor; telemetry task bloklanmiyor. | Yok. | `test_param_manager`. |
| PDF-P2-4 | KAPALI | Sicaklik ve basinc paydalari kontrol ediliyor; sifir-payda testi eklendi. | Yok. | `test_sensor_drivers`. |
| PDF-P2-5 | KAPALI | Reset tek `0..N` dongusune indirildi. | Yok. | `test_scheduler`. |
| PDF-P2-6 | KAPALI | Native testlere ek olarak `native_link` environment eklendi; secili urun modulleri ayri translation unit olarak linkleniyor ve CI'da kosuyor. | Kapsami moduller arttikca genislet. | `pio test -e native_link --without-uploading` |
| PDF-P2-7 | KAPALI | `PARAM_SET` ve `PARAM_REQUEST_LIST` system/component hedeflerini dogruluyor; broadcast destekleniyor. | Yok. | Yanlis hedef unit testi ve Pico build. |

## Rapordaki Diger Gelistirme Onerileri

| Konu | Durum | Takip |
|---|---|---|
| Statik FreeRTOS task/stack tahsisi | KAPALI | `xTaskCreateStaticAffinitySet` ile Sensor/Flight/Telemetry task stackleri statik; high-water ve drop sayaclari runtime-health blackbox kaydinda. |
| Blocking ring buffer yerine lock-free veri yolu | KAPALI | Aktif veri yolu typed blackboard/seqlock topic; legacy ring buffer aktif akista kullanilmiyor. |
| Binary/non-blocking blackbox ve ortak UART bant genisligi yonetimi | KAPALI | Binary blackbox record + CRC var; PioUart TX queue `pio_sm_put_blocking` kullanmiyor. |
| Battery ADC, filtre/hysteresis ve brownout dogrulamasi | KAPALI / DONANIM | `V1-12` yazilim kapali; fiziksel divider/pin kaniti `V1-15` |
| Otomatik fiziksel HIL runner | DONANIM | `V1-15` |
| Uzun sureli USB/boot/stack soak testi | DONANIM | `V1-16` |
| XIP/SRAM yerlestirmesini olcumle dogrulama | DONANIM | Kritik PID/fusion ve flash programlama rutinleri RAM'e alindi; yazilim kapilari tamam. Jitter/cycle ve SRAM bank contention kaniti bench'e kaldi. |
| Navigation/Altitude iskeletlerinin V1.0 disi oldugunu netlestirme | KAPALI | V1.0 dokumanlari manuel/SAS kapsamini, otonominin V1.5/V2.0 oldugunu belirtiyor. |
| MAVLink/blackbox PIO UART blocking yazimini kaldirma | KAPALI | `pio_sm_put_blocking` kaldirildi; TX queue servis modeli kullaniyor. |
| MAVLink RC override yetkilendirme | KAPALI | RC override varsayilan kapali; hedef system/component ve explicit enable kapisi native testle dogrulaniyor. |

## 2026-07-11 Hizli Bulgu Kapanislari

Kaynak: `0d8bc8a9-b6e1-4557-bbac-6472c75c4e14/pasted-text.txt`

| ID | Durum | Duzeltme | Kanit |
|---|---|---|---|
| FAST-01 PioUart RX FIFO overflow | KAPALI | PIO RX FIFO polling yerine PIO IRQ ile RAM ring buffer'a bosaltiliyor; RX drop sayaci eklendi. | Architecture policy, `pio test -e native --without-uploading`, `pio check -e pico --skip-packages --fail-on-defect=high --silent` |
| FAST-02 PioUart TX starvation | KAPALI | TX ring buffer PIO TX-not-full IRQ ile arka planda besleniyor; task beklemesi ve blocking PIO yazimi yok. | Architecture policy `pio_sm_put_blocking` kapisi, native testler, static check |
| FAST-03 Aux bus DMA collision | KAPALI | Mag/baro paylasimli aux DMA yolu sirali state machine'e cevrildi; baro transferi mag transferi aktifken baslamiyor. | Architecture busy-wait kapisi, sensor driver/native testler |
| FAST-04 Seqlock barrier race | KAPALI | Publish/read yollarina seq-cst fence ve release/acquire siralama eklendi; cross-core snapshot tutarliligi guclendirildi. | `test_blackboard`, native testler |
| FAST-05 Double precision math penalty | KAPALI | Sensor fusion `sqrtf/asinf/atan2f` kullaniyor; RP2350 tek duyarlikli FPU yolu korunuyor. | Architecture double-math kapisi, `test_sensor_fusion` |
| FAST-06 Servo PIO FIFO flooding/jitter | KAPALI | Servo output 20 ms frame hizina indirildi; yazma yolunda FIFO clear yok, safe frame aninda geciyor. | Architecture servo-write kapisi, `test_pwm_pio`, static check |
| FAST-07 RC update latency | KAPALI | RC scheduler hizi 50 Hz'den 150 Hz'e cikarildi. | Architecture RC-rate kapisi, native scheduler/RC testleri |
| FAST-08 Battery disabled default | KAPALI / DONANIM | Battery ADC varsayilan acildi; dusuk voltaj/brownout karar yolu testli. | `test_battery_monitor`; fiziksel divider/pin dogrulamasi bench checklist'te |

## Uygulama Sirasi

1. `PDF-P0-1`: armed flash-save kapisi ve iki-core flash guvenligi.
2. `PDF-P0-2`: 500 Hz tek kaynak duzeltmesi.
3. `PDF-P1-5`: timing window/latch semantigi.
4. `PDF-P2-4` ve `PDF-P2-7`: kucuk ama guvenlik etkili savunma
   duzeltmeleri.
5. `PDF-P2-3`: non-blocking parametre aktarimi.
6. `PDF-P1-4`: non-blocking aux sensor bus.
7. `PDF-P1-3`, `PDF-P1-1`, `PDF-P1-6`: HAL ve logging sinirlari.
8. `PDF-P1-2`, `PDF-P1-8`: estimator isim/fallback karari.
9. Kalan P2 hijyen ve normal translation-unit integration hedefi.
10. Fiziksel bench/HIL maddeleri.

## Kapanis Kurali

Bir bulgu ancak:

- kod degisikligi tamamlandiginda,
- ilgili unit/integration/fault testi gectiginde,
- Pico firmware derlendiginde,
- kritik/yuksek statik analiz bulgusu kalmadiginda,
- donanim bagimliysa bench kaniti kaydedildiginde

`KAPALI` durumuna getirilebilir.
