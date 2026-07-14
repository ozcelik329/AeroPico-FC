# AeroPico SBUS Generator

Bu klasor, kumanda/verici olmadan AeroPico-FC'nin SBUS girisini bench ortaminda
test etmek icin Arduino IDE ile yuklenebilen iki ornek icerir.

## Onerilen Donanim

1. **ESP32 DevKit v1 / NodeMCU-32S**  
   Onerilen yol budur. Donanimsal UART ile `100000 baud, 8E2` SBUS frame uretir.

2. **Arduino Uno**  
   Yedek yoldur. Timer tabanli bit-bang cikis kullanir. Test icin yeterlidir,
   ancak ESP32 kadar temiz ve esnek degildir.

## Kritik Gerilim Uyarisi

- Pico 2 GPIO pinleri 3.3V seviyesindedir.
- Arduino Uno cikislari 5V seviyesindedir; Uno cikisini Pico GP1'e dogrudan baglama.
- Elindeki transistor inverter devresini kullanirken inverter cikisinin 3.3V pull-up
  ile Pico tarafina gittiginden emin ol.
- ESP32 3.3V cikis verir; istersen dogrudan Pico GP1'e non-inverted UART olarak
  baglayabilirsin. Mevcut transistor inverter yolunu test etmek istiyorsan ESP32
  sketch'inde `SBUS_TX_INVERTED` ayarini acik birak ve ESP32 TX'i inverter girisine bagla.

## AeroPico Tarafi

Mevcut firmware ayari:

```cpp
PIN_SBUS_RX     1
SBUS_UART_BAUD  100000
SBUS_UART_CONFIG SERIAL_8E2
```

Yani AeroPico Pico 2 tarafinda SBUS, GP1 / UART0 RX pininden okunur.

## ESP32 DevKit v1 Baglanti

Varsayilan sketch:

- ESP32 `GPIO17` = SBUS TX
- ESP32 `GND` = AeroPico GND

Iki baglanti secenegi var:

### Secenek A - Mevcut transistor inverter devresini test et

```text
ESP32 GPIO17  -> transistor inverter input
inverter out  -> Pico GP1 / SBUS RX
ESP32 GND     -> Pico GND
```

Bu secenekte sketch varsayilan olarak inverted TX uretir.

### Secenek B - Inverter kullanmadan dogrudan Pico'ya gir

```text
ESP32 GPIO17  -> Pico GP1 / SBUS RX
ESP32 GND     -> Pico GND
```

Bu secenekte sketch icinde:

```cpp
#define SBUS_TX_INVERTED 0
```

olarak degistir.

## Arduino Uno Baglanti

Varsayilan sketch:

- Uno `D9` = SBUS test output
- Uno `GND` = AeroPico GND

Uno 5V oldugu icin Pico GP1'e dogrudan baglama.

```text
Uno D9       -> transistor inverter input
inverter out -> Pico GP1 / SBUS RX
Uno GND      -> Pico GND
```

## Arduino IDE Yukleme

### ESP32

1. Arduino IDE'de ESP32 board paketini kur.
2. Board olarak `ESP32 Dev Module` sec.
3. `tools/sbus_generator/esp32_devkitv1/ESP32DevKitV1_SBUS_Generator.ino`
   dosyasini ac.
4. Upload yap.
5. Serial Monitor'u `115200` baud ac.

### Uno

1. Board olarak `Arduino Uno` sec.
2. `tools/sbus_generator/arduino_uno/ArduinoUno_SBUS_Generator.ino`
   dosyasini ac.
3. Upload yap.
4. Serial Monitor'u `115200` baud ac.

## Uretilen Kanal Senaryolari

Generator 14 ms aralikla SBUS frame yollar.

- CH1 roll: yavas sinus benzeri sweep
- CH2 pitch: merkez
- CH3 throttle: dusuk, sonra orta, sonra tekrar dusuk
- CH4 yaw: merkez
- CH5 mode: manual / stabilize arasinda gecis
- CH6: test aux kanali

Yaklasik 30 saniyede bir `failsafe gap` uygulanir: frame gonderimi kisa sure
durur. AeroPico tarafinda RC timeout/failsafe davranisi gorulmelidir.

## Beklenen AeroPico Sonucu

Configurator veya MAVLink telemetri tarafinda sunlari gormelisin:

- RC valid olur.
- Kanallar 1000-2000 us araligina map edilir.
- CH5 mode degisimi flight mode tarafina yansir.
- Failsafe gap sirasinda RC failsafe tetiklenir.

Bu test gercek RF alici/kumanda testi degildir. Sadece Pico'nun SBUS fiziksel
girisini, UART ayarini, parser'i, kanal mapping'i ve failsafe timeout'unu test eder.
