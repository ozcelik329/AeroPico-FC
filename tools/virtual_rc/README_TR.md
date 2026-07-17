# AeroPico Virtual RC Bench

Bu arac ESP32 DevKit V1 uzerinde calisan, tarayicidan kontrol edilen bir bench kumandasidir.
Gercek ucus kumandasi degildir; yalnizca masaustu test, RC mapping, mode switch, servo ve failsafe
dogrulamalari icindir.

## Ne Yapar?

- ESP32 kendi WiFi agini acar.
- Telefon veya laptop tarayicisina iki joystickli bir arayuz sunar.
- Joystick degerlerini SBUS kanallarina cevirir.
- Pico 2 tarafinda AeroPico-FC bunu gercek SBUS alici gibi okur.

```text
Telefon/Laptop browser
        |
        | WiFi: AeroPico-VirtualRC
        v
ESP32 DevKit V1
        |
        | SBUS 100000 baud / 8E2
        v
Pico 2 GP1 / UART0 RX
```

## Dosya

```text
tools/virtual_rc/esp32_devkitv1/AeroPicoVirtualRC/AeroPicoVirtualRC.ino
```

## Arduino IDE Ayarlari

```text
Board: ESP32 Dev Module / ESP32 Dev Board
Serial Monitor: 115200 baud
```

## Baglanti

Mevcut transistor inverter devresiyle:

```text
ESP32 GPIO17  -> transistor inverter input
inverter out  -> Pico 2 GP1 / SBUS RX
ESP32 GND     -> Pico 2 GND
```

Inverter kullanmadan dogrudan:

```text
ESP32 GPIO17  -> Pico 2 GP1 / SBUS RX
ESP32 GND     -> Pico 2 GND
```

Dogudan baglanti icin sketch icinde sunu degistir:

```cpp
#define SBUS_TX_INVERTED 0
```

## Kullanim

1. Sketch'i ESP32'ye yukle.
2. Serial Monitor'de su satiri bekle:

```text
AeroPico Virtual RC Bench ready
AP: AeroPico-VirtualRC password: aeropico IP: 192.168.4.1
```

3. Telefon veya laptop ile WiFi agina baglan:

```text
SSID: AeroPico-VirtualRC
Sifre: aeropico
```

4. Tarayicida ac:

```text
http://192.168.4.1
```

## Kanal Haritasi

```text
CH1: Roll
CH2: Pitch
CH3: Throttle
CH4: Yaw
CH5: Flight mode switch
CH6: ileride aux/test icin ayrildi
```

## Guvenlik Davranisi

- Throttle acilista minimumdadir.
- Browser veri gondermezse ESP32 kanallari safe konuma ceker.
- Failsafe butonu SBUS failsafe flag'i gonderir.
- Bu aracla pervane takiliyken test yapma.

## Beklenen AeroPico Sonucu

MAVLink probe veya Configurator tarafinda:

- RC valid olmalidir.
- CH1-CH4 hareket etmelidir.
- CH5 ile MANUAL/STABILIZE esigi degismelidir.
- Browser kapatilinca/telefon uykuya girince failsafe/safe davranis gorulmelidir.
