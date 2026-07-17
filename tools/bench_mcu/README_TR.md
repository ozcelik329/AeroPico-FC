# AeroPico Bench MCU Araçları

Bu klasor, eldeki yardimci mikrodenetleyicileri bench test cihazina cevirmek icin kullanilir.
Ana AeroPico firmware'i degildir.

## Mevcut Roller

```text
Pico 2        -> Ana AeroPico-FC
ESP32 DevKit  -> Virtual RC / SBUS generator
ESP8266       -> Mini PWM logic analyzer / I2C probe
Arduino Uno   -> Yedek PWM capture / servo test yardimcisi
Pico WH       -> Yedek PWM capture
```

## PWM Capture Ne Icin?

Servo/ESC cikislarinin gercekten dogru pulse urettigini kanitlamak icin:

```text
1000 us -> minimum
1500 us -> neutral
2000 us -> maksimum
period  -> yaklasik 20000 us / 50 Hz
```

Bu bir logic analyzer kadar iyi degildir ama bench seviyesinde cok ise yarar.

## ESP8266 PWM Capture

Dosya:

```text
tools/bench_mcu/esp8266_pwm_capture/ESP8266_PWM_Capture/ESP8266_PWM_Capture.ino
```

Arduino IDE:

```text
Board: NodeMCU 1.0 (ESP-12E Module)
Serial Monitor: 115200 baud
```

Baglanti:

```text
AeroPico GP16/17/18/19 -> ESP8266 D5 / GPIO14
AeroPico GND           -> ESP8266 GND
```

Cikti:

```text
pulse_us=1500 period_us=20000 freq=50.00Hz count=123
```

## Arduino Uno PWM Capture

Dosya:

```text
tools/bench_mcu/arduino_uno_pwm_capture/ArduinoUno_PWM_Capture/ArduinoUno_PWM_Capture.ino
```

Baglanti:

```text
AeroPico GP16/17/18/19 -> Uno D8
AeroPico GND           -> Uno GND
```

Uno cikislarini Pico'ya dogrudan verme. Bu arac sadece Pico'nun 3.3V cikisini okur.

## Pico WH PWM Capture

Dosya:

```text
tools/bench_mcu/pico_wh_pwm_capture/PicoWH_PWM_Capture/PicoWH_PWM_Capture.ino
```

Baglanti:

```text
AeroPico GP16/17/18/19 -> Pico WH GP15
AeroPico GND           -> Pico WH GND
```

## Test Sirasi

1. AeroPico Pico 2 firmware'ini yukle.
2. Pervaneyi kesinlikle sok.
3. Capture cihazini sec ve yukle.
4. Once throttle/ESC cikisi GP19'u olc.
5. Sonra GP16, GP17, GP18 servo cikislarini olc.
6. Configurator veya MAVLink servis komutuyla servo test yaptir.
7. Ciktiyi bench checklist'e kaydet.

Beklenen:

```text
safe throttle: 1000 us
neutral surface: 1500 us
servo test low/high: 1000-2000 us arasi
period: 20000 us civari
```
