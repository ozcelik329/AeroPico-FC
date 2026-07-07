# AeroPico FC - Bench Test Checklist

Bu liste ilk enerji verme, firmware degisikligi ve ucus oncesi masa testleri icindir. GPS/otonom mod testleri kapsam disidir.

## 1. Guc ve Boot

- USB veya guvenli bench beslemesi kullan.
- Pervane takili olmasin.
- Boot banner gorunuyor mu?
- Watchdog reset uyarisi varsa nedeni not edildi mi?
- Health report icinde IMU, DMA, RC, baro ve mag durumlari beklenen gibi mi?

## 2. Sensor Sagligi

- Kart sabitken gyro degerleri sifira yakin mi?
- Accel degerleri beklenen eksende yaklasik 1g gosteriyor mu?
- `SensorHealth` durumu `Ok` oluyor mu?
- Sensor kablosu/hatasi simule edildiginde stale/invalid durum blackbox veya telemetry'de gorunuyor mu?
- Median filtre aktifken ham verideki tekil sicramalar attitude hesabina belirgin yansimiyor mu?

## 3. RC ve Failsafe

- SBUS UART0/GP1 uzerinden kanal verileri geliyor mu?
- Roll, pitch, throttle, yaw kanal sirasi dogru mu?
- RC kapatilinca 500 ms civarinda failsafe devreye giriyor mu?
- Failsafe durumunda throttle `PWM_MIN`, yuzeyler `PWM_NEUTRAL` oluyor mu?
- Failsafe arm durumunu kapatiyor mu?

## 4. Servo ve Mixer

- Pervane yokken servo yonleri dogru mu?
- Aileron, elevator ve rudder ters tepki vermiyor mu?
- Servo cikislari her durumda `PWM_MIN..PWM_MAX` araliginda kaliyor mu?
- Trimler ve mixer gain degerleri beklenen etkide mi?

## 5. MAVLink ve Blackbox

- Heartbeat 1 Hz civarinda geliyor mu?
- ATTITUDE ve RC_CHANNELS akislari stabil mi?
- SYS_STATUS sensor health bitleri sensor durumuyla uyumlu mu?
- RC override verildiginde kanallar degisiyor mu?
- RC override kesilince timeout ile temizleniyor mu?
- Blackbox satirinda `sensorHealth` alani gorunuyor mu?

## 6. Zamanlama ve Watchdog

- Core 1 heartbeat taze kaliyor mu?
- Uzun sureli bench calismasinda watchdog reset yok mu?
- Bilerek Core 1 donmasi veya sensor task blokaj testi yapildiginda watchdog beklenen sekilde resetliyor mu?
- Timing budget ihlali varsa loga alindi mi?

## 7. Test Sonucu

- Test tarihi:
- Firmware commit/surum:
- Kart:
- Sensor paketi:
- RC alici:
- Sonuc: Gecti / Kaldi
- Notlar:
