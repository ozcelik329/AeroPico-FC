# AeroPico ESP32 MAVLink Bridge

Bu klasor, AeroPico-FC ile Mission Planner / QGroundControl arasinda opsiyonel
WiFi MAVLink koprusu icin hazirlik alanidir.

Varsayilan v1.0 RCI test yolu Pico 2 USB Serial uzerinden MAVLink'tir. ESP32
koprusu daha sonra donanimla dogrulanacak opsiyonel companion katmanidir.

## Hedef Akis

```text
AeroPico Pico 2 GP12/GP13 PIO UART
  -> ESP32 UART
  -> WiFi UDP 14550
  -> Mission Planner / QGroundControl / AeroPico GCS
```

## Beklenen Ayarlar

- Pico MAVLink UART: `ESP32_CAM_UART_BAUD` varsayilan `57600`
- GCS UDP portu: `14550`
- ESP32 tarafinda UART RX/TX pinleri kullanilan karta gore secilir.
- FC tarafinda USB MAVLink aktifken ESP32 koprusu zorunlu degildir.

## Guvenlik

- Servo test, calibration ve override komutlari FC tarafinda safe/disarmed
  kapilarindan gecmelidir.
- ESP32 koprusu sadece paket tasiyici olmalidir; arm/failsafe karari ESP32'de
  verilmemelidir.
