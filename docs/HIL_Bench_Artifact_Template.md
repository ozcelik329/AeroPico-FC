# AeroPico-FC HIL / Bench Artifact Template

Bu dosya her release candidate icin fiziksel kanit dosyasi olarak kopyalanip doldurulmalidir. Donanim kaniti olmadan `v1.0.0` final etiketi verilmez; yazilim yalnizca RC kabul edilir.

## Release Bilgisi

- Release/tag:
- Commit SHA:
- Tarih:
- Kart:
- Firmware build komutu:
- Testi yapan:

## 1. Servo PWM Logic Analyzer Capture

| Kanal | Pin | Komut | Beklenen | Olculen | Sonuc |
|---|---:|---:|---:|---:|---|
| Aileron | GP | 1000 us | 1000 +/- 10 us | | |
| Aileron | GP | 1500 us | 1500 +/- 10 us | | |
| Aileron | GP | 2000 us | 2000 +/- 10 us | | |
| Elevator | GP | 1000 us | 1000 +/- 10 us | | |
| Elevator | GP | 1500 us | 1500 +/- 10 us | | |
| Elevator | GP | 2000 us | 2000 +/- 10 us | | |
| Rudder | GP | 1000 us | 1000 +/- 10 us | | |
| Rudder | GP | 1500 us | 1500 +/- 10 us | | |
| Rudder | GP | 2000 us | 2000 +/- 10 us | | |
| Throttle | GP | 1000 us | 1000 +/- 10 us | | |
| Throttle | GP | 1500 us | 1500 +/- 10 us | | |
| Throttle | GP | 2000 us | 2000 +/- 10 us | | |

Capture dosyalari:

- Logic analyzer session:
- Ekran goruntusu:

## 2. SBUS / RC Hardware Test

- SBUS pin:
- Baud: 100000
- Inverter/transistor devresi:
- Kanal sirasi dogrulandi mi:
- Mode channel:
- RC kapatilinca failsafe sure olcumu:
- Failsafe output sonucu:

## 3. Battery ADC / Brownout Test

- Voltaj bolucu direncleri:
- Olculen batarya voltaji:
- ADC pin voltaji:
- Firmwarede hesaplanan voltaj:
- Multimetre farki:
- Low voltage preflight sonucu:
- Brownout esigi sonucu:

## 4. Timing / WCET / Jitter

- Test suresi: minimum 10 dakika
- Control loop hedefi: 500 Hz
- Max total runtime us:
- Avg total runtime us:
- Max jitter us:
- Deadline miss sayisi:
- Watchdog reset oldu mu:
- Blackbox timing record dosyasi:

## 5. MAVLink / Configurator Safety

- Armed iken PARAM_SET reddedildi mi:
- Armed iken RC override reddedildi mi:
- Disarmed RC override test sonucu:
- Configurator servo test sadece safe durumda calisti mi:
- Preflight reason metinleri gorundu mu:

## 6. Sonuc

- Genel sonuc: Gecti / Kaldi
- Bloklayici hata:
- Release icin karar:

