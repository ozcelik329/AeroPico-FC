# AeroPico FC Kullanma Kilavuzu

Bu kilavuz, AeroPico FC projesini ilk kez eline alan birinin karti kurmasi, firmware yuklemesi, kalibrasyon yapmasi, masa testlerini kosmasi ve ilk ucusa hazirlik yapmasi icin yazildi.

Onemli: Bu proje henuz sertifikali bir ucus kontrolcusu degildir. Pervane takili test yapma. Ilk testleri mutlaka motor/ESC gucu ayrikken ve model sabitlenmis halde yap.

## 1. Gerekli Parcalar

- Raspberry Pi Pico 2 / RP2350 kart
- GY-87 sensor karti
- SBUS alici
- SBUS icin transistör inverter devresi
- ESC ve 5V BEC
- Servo veya servo tester ile denenmis saglam servo seti
- USB kablo
- 30x10 breadboard ve jumper kablolar
- 10k / 1k direncler
- Bilgisayar: macOS, Linux veya Windows

Istege bagli ama onerilir:

- Logic analyzer
- Multimetre
- Harici 5V BEC
- Pervanesiz motor test standi

## 2. Guvenlik Kurallari

1. Pervane takiliyken yazilim yukleme, kalibrasyon veya bench test yapma.
2. Pico, GY-87 ve alicinin tum GND hatlari ortak olmali.
3. I2C hatlari 5V pull-up ile kullanilmamali. GY-87 uzerindeki pull-up genelde 3.3V ise uygundur.
4. Pico GPIO pinlerine 5V sinyal verme.
5. Servo/ESC 5V hatti Pico GPIO pinlerine baglanmaz.
6. ESC sinyal GND'si Pico GND ile ortaklanir.
7. Ilk hareket testinde servo kollarini modelden ayir.

## 3. Temel Baglanti Ozeti

### GY-87

| GY-87 | Pico 2 |
|---|---|
| VCC | 3V3 |
| GND | GND |
| SDA | GP4 |
| SCL | GP5 |

Not: GY-87 kartinda 5V pull-up varsa kullanma. Pull-up direncleri 3.3V'a bagli olmali.

### SBUS Alici

| Alici / Inverter | Pico 2 |
|---|---|
| SBUS cikisi | Transistör inverter girisi |
| Inverter cikisi | GP1 / UART0 RX |
| GND | GND |
| Alici besleme | BEC 5V |

Varsayilan SBUS ayari `100000` baud ve `SERIAL_8E2` formatidir. Bu degerler `src/config.h` icindeki `SBUS_UART_BAUD` ve `SBUS_UART_CONFIG` makrolarindan degistirilir.

Varsayilan RC kanal eslemesi:

```text
CH1: Roll / aileron
CH2: Pitch / elevator
CH3: Throttle
CH4: Yaw / rudder
CH5: Flight mode
```

Flight mode kanali:

- `CH5 < 1500 us`: MANUAL
- `CH5 >= 1500 us`: STABILIZE

Istersen bu kanal daha sonra MAVLink parametresi `RC_MODE_CH` ile 6. kanal veya baska bir kanala alinabilir.

### Servo / ESC Sinyalleri

Servo ve ESC sinyal kablolari proje pin haritasina gore baglanir. Besleme icin ESC/BEC 5V kullanilir; sinyal GND mutlaka Pico GND ile ortak olur.

Genel kural:

- Servo kahverengi/siyah: GND
- Servo kirmizi: BEC 5V
- Servo sari/beyaz/turuncu: Pico PWM sinyal pini

ESC kirmizi hatti:

- ESC uzerinde BEC varsa alici/servo 5V rayini besleyebilir.
- Pico'yu VSYS uzerinden besleyeceksen 5V BEC cikisi VSYS'e verilebilir, ama USB ve BEC ayni anda bagliyken geri besleme riskine dikkat et.
- Ilk testlerde Pico'yu USB'den beslemek, servo/ESC hattini ayri 5V BEC ile beslemek daha kontrolludur. GND ortak kalir.

### Batarya Voltaj Bolucu / Battery Monitor

Pico ADC pini en fazla 3.3V gorebilir. Ucus bataryasini ADC pinine direkt baglama; Pico zarar gorebilir. Batarya voltaji mutlaka iki direncli voltaj bolucu ile dusurulmelidir.

Kodun varsayilan ayari:

```text
PIN_BATTERY_ADC = GP26 / ADC0
BATTERY_VOLTAGE_DIVIDER_RATIO = 11.0
BATTERY_MIN_VOLTAGE = 10.5V
BATTERY_MAX_VOLTAGE = 12.8V
BATTERY_BROWNOUT_VOLTAGE = 9.6V
```

Bu ayar 3S LiPo icin dusunulmustur ve elindeki `10k` + `1k` direnclerle kullanilabilir:

```text
Batarya +  --- 10k ---+--- GP26 / ADC0
                      |
                      1k
                      |
Batarya -  -----------+--- Pico GND
```

Bu baglantida ADC pini bataryanin yaklasik `1/11` degerini gorur. Ornek:

- 12.6V tam dolu 3S batarya -> ADC yaklasik 1.15V
- 10.5V dusuk 3S batarya -> ADC yaklasik 0.95V
- 9.6V brownout riski -> ADC yaklasik 0.87V

Kontrol sirası:

1. Bataryayi baglamadan once direncleri multimetreyle olc.
2. Once sadece voltaj bolucuyu bataryaya bagla, Pico'ya baglama.
3. Ortadaki noktada 3.3V altinda gerilim oldugunu multimetreyle dogrula.
4. Sonra orta noktayi GP26'ya bagla.
5. Batarya eksi hattini Pico GND ile ortakla.
6. Seri monitor veya MAVLink telemetry uzerinden batarya uyarisi gelip gelmedigini kontrol et.

Dikkat:

- `10k` ust, `1k` alt direnctir. Ters baglarsan ADC pini cok yuksek voltaj gorebilir.
- 2S batarya icin de calisir; ama yazilim esikleri 3S oldugu icin `BATTERY_MIN_VOLTAGE`, `BATTERY_MAX_VOLTAGE` ve `BATTERY_BROWNOUT_VOLTAGE` ayarlanmalidir.
- 4S batarya icin bu bolucu elektriksel olarak ADC acisindan guvenli kalir, fakat yazilim esikleri 4S'e gore ayarlanmadan ucus yapma.
- Batarya monitoru failsafe/preflight zincirine baglidir; kritik voltajda arm engellenebilir veya failsafe nedeni raporlanabilir.

## 4. Firmware Yukleme

### PlatformIO ile derleme

Proje kok dizininde:

```bash
pio run -e pico
```

Basarili olursa firmware dosyasi burada olusur:

```text
.pio/build/pico/firmware.uf2
```

### Pico 2'ye yukleme

1. Pico 2 uzerindeki BOOTSEL tusuna basili tut.
2. USB kabloyu tak.
3. Bilgisayarda RPI-RP2 benzeri bir surucu acilir.
4. `firmware.uf2` dosyasini bu surucuye kopyala.
5. Kart yeniden baslar.

## 5. Ilk Boot Kontrolu

Seri monitor:

```bash
pio device monitor -b 115200
```

Beklenenler:

- Boot banner gorunur.
- MPU6050 WHOAMI gercek degerle raporlanir.
- Ilk kurulumda `Calibration Save` gorunebilir.
- Sonraki boot'ta `Calibration Load` gorunmelidir.
- Sensor health `OK` seviyesine gelmelidir.

## 6. Kalibrasyon Nasil Yapilir

### Configurator ile komutlu kalibrasyon

1. Pico'yu USB ile bilgisayara bagla.
2. `tools/aeropico-configurator` uygulamasini ac.
3. Port secip baglan.
4. `IMU Kalibrasyon` icin modeli tamamen sabit tut ve komutu gonder.
5. `Mag Kalibrasyon` icin ilk basista toplama baslar; modeli roll/pitch/yaw eksenlerinde yavasca cevir; ikinci basista offset flash'a kaydedilir.
6. `Sensor Kontrol` ve `Preflight Kontrol` butonlari firmware'den gercek ACK ve sebep mesaji alir.
7. `Servo Yon Testi` yalnizca disarmed/safe durumda kabul edilir. Pervane takiliyken kullanma.

### IMU boot kalibrasyonu

Firmware ilk kez kalibrasyon bulamazsa boot sirasinda otomatik IMU kalibrasyonu yapar.

Yapman gereken:

1. Ucak veya test duzeni tamamen sabit olsun.
2. GY-87 titreşimsiz dursun.
3. Kart yatay ve normal ucus yonunde dursun.
4. Boot sirasinda karti oynatma.
5. Seri monitorde gyro bias, accel bias ve gyro temp coeff degerlerini gor.
6. Kalibrasyon flash'a kaydedilirse ikinci boot'ta tekrar otomatik kalibrasyon yapmaz, kayitli degeri yukler.

Kalibrasyon hataliysa:

- Karti sabitle.
- Sensor kablolarini kontrol et.
- Firmware'i tekrar yukleyip kalibrasyon storage temizleme araci eklenene kadar flash/firmware temiz kurulum yap.

### Manyetometre hard-iron kalibrasyonu

Manyetometre icin altyapi vardir. Uygulamada kalibrasyon yaparken model farkli yonlere cevrilir ve min/max manyetik alan degerlerinden hard-iron offset hesaplanir.

Pratikte yapilacak hareket:

1. Modeli motor/ESC gucu kapaliyken eline al.
2. Roll, pitch ve yaw eksenlerinde yavasca farkli yonlere cevir.
3. Metal masa, vida, hoparlor, motor mıknatısı ve yuksek akim kablolarindan uzak dur.
4. Kalibrasyon tamamlaninca hard-iron offset flash'a kaydedilmelidir.

Not: Bu akis Configurator uzerinden MAVLink servis komutu olarak baslatilir ve sonuc `COMMAND_ACK` + `STATUSTEXT` ile raporlanir.

## 7. Masa Testleri

### Yazilim testleri

Donanim gerekmeden:

```bash
pio test -e native --without-uploading
pio test -e native_link --without-uploading
python3 tools/ci/check_architecture.py
python3 tools/fault_injection/fault_injection.py --json-report fault_matrix_report.json
pio check -e pico --skip-packages --fail-on-defect=high --silent
pio run -e pico
```

Bu komutlar ne ise yarar:

- `pio test -e native --without-uploading`: Bilgisayarda calisan unit testleri kosar; sensor fusion, mixer, failsafe, parametre, battery monitor gibi mantik hatalarini yakalar.
- `pio test -e native_link --without-uploading`: Modullerin birlikte linklenebildigini kontrol eder; eksik sembol/include hatalarini erken yakalar.
- `python3 tools/ci/check_architecture.py`: Mimari kurallari kontrol eder; yanlis katmana donanim bagimliligi sizmasini engeller.
- `python3 tools/fault_injection/fault_injection.py`: Dusuk batarya, sensor hatasi, RC loss gibi ariza senaryolarinin beklenen testlerle kapatildigini kontrol eder.
- `pio check -e pico --skip-packages --fail-on-defect=high --silent`: Statik analiz yapar; yuksek onemli kod sorunlarinda hata verir.
- `pio run -e pico`: Gercek Pico 2 firmware'inin derlenip derlenmedigini kontrol eder.

### Donanim bench test

`docs/Bench_Test_Checklist.md` dosyasini takip et.

Kontrol sirasina onerilen akıs:

1. Pico USB ile aciliyor mu?
2. GY-87 WHOAMI dogru mu?
3. Boot kalibrasyonu yapilip kaydediliyor mu?
4. Ikinci boot'ta `Calibration Load` gorunuyor mu?
5. SBUS GP1 uzerinden okunuyor mu?
6. Alıcı failsafe kapatildiginda sistem failsafe'e giriyor mu?
7. Servo cikislarinda sinyal var mi?
8. Servo min/max/trim/reverse parametreleri beklenen yonde calisiyor mu?
9. Watchdog sadece flight loop saglikliyken besleniyor mu?
10. GP26 battery ADC okuması multimetreyle uyumlu mu?
11. Dusuk voltaj/brownout esikleri preflight ve failsafe'e yansiyor mu?
12. Blackbox ve MAVLink telemetry alanlari doluyor mu?

## 8. HIL Nasil Kullanilir

HIL, bilgisayardan kartin seri portuna basit saglik ve smoke testleri gondermek icin kullanilir.

1. Pico 2 firmware yuklu olsun.
2. Pico USB ile bagli olsun.
3. Seri portu bul:

```bash
pio device list
```

4. HIL smoke testini calistir:

```bash
python3 tools/hil_smoke/hil_smoke.py --port /dev/tty.usbmodemXXXX
```

Linux icin port ornegi:

```bash
python3 tools/hil_smoke/hil_smoke.py --port /dev/ttyACM0
```

GitHub Actions uzerinde HIL sadece manuel calisir. Donanim GitHub runner'a takili olmadigi icin normal push testlerinde HIL atlanir.

## 9. Ilk Ucus Oncesi

`docs/First_Flight_Checklist.md` dosyasini yazdir veya ekranda ac.

Kritik kontrol:

- Pervane yonu dogru.
- Servo yonleri dogru.
- Manual modda kumanda yuzeyleri dogru yonde.
- Stabilize modda model egilince yuzeyler ters yonde duzeltme veriyor.
- Failsafe throttle kapatiyor veya belirlenen guvenli cikisa gidiyor.
- Batarya ve BEC voltaji yuk altinda dusmuyor.
- GP26 ADC voltaji 3.3V altinda kalıyor ve batarya telemetry degeri multimetreyle uyumlu.
- CG ve mekanik trim dogru.

## 10. Sorun Giderme

### WHOAMI hatasi

- SDA/SCL ters olabilir.
- GY-87 besleme yoktur.
- GND ortak degildir.
- I2C pull-up 5V'a gidiyordur.

### SBUS okunmuyor

- Inverter cikisi GP1'e gelmiyor olabilir.
- Alici 5V besleme almiyor olabilir.
- GND ortak degildir.
- UART0/GP1 yerine farkli pine baglanmistir.

### Servo hareket etmiyor

- Servo 5V almiyor olabilir.
- GND ortak degildir.
- Sinyal pini yanlistir.
- Servo ters takilmistir.
- Arming/preflight sistemi cikislari kilitliyor olabilir.

### Testler gecmiyor

- PlatformIO cache izin problemi olabilir.
- `~/.platformio` klasorunun sahibi kullanici olmali.
- Gerekirse PlatformIO'yu yeniden kur:

```bash
python3 -m pip install -U platformio
```

## 11. Ucus Icin Teslim Paketi

Projeyi hic bilmeyen birine verirken sunlari ver:

- Bu kullanma kilavuzu
- `docs/Bench_Test_Checklist.md`
- `docs/First_Flight_Checklist.md`
- Pin baglanti semasi
- Firmware `.uf2` dosyasi
- Hangi branch/tag kullanildigi: `pico2-catalyi`, `v0.3-catalyi`
- Hangi servo/ESC/BEC/alici kullanildigi
- Kalibrasyonun ne zaman yapildigi
- Ilk ucusta kullanilacak ucak, batarya ve kumanda ayarlari
