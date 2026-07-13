# AeroPico FC - Nihai Yol Haritasi Karsilastirmasi

Tarih: 2026-07-07

Bu belge mevcut kod tabanini kapsamli muhendislik yol haritasi maddelerine gore yeniden karsilastirir. Donanim elde olmadigi icin fiziksel test gerektiren maddeler bilincli olarak beklemede birakilmistir.

## Ozet

| Alan | Durum | Not |
|---|---|---|
| Ucus kritik temel | Tamamlandi | Boot, FreeRTOS task modeli, SBUS UART secimi, sensor stale korumasi, I2C timeout ve daha guvenli derleme bayraklari eklendi. |
| Kontrol kalitesi | Tamamlandi | PID anti-windup, limitler, dt korumalari, mixer ve flight mode testleri mevcut. |
| MAVLink / ParamManager | Tamamlandi | Override callback mimarisi, override timeout, param callback ve host testleri mevcut. |
| Sensor health / kalibrasyon | Buyuk olcude tamamlandi | Sensor health log/MAVLink'e tasiniyor; IMU/mag kalibrasyon tipleri ve storage API eklendi. Baro/mag health sahada dogrulanmali. |
| Zamanlama / CI | Kod tarafinda tamamlandi | Timing budget blackbox/MAVLink'e raporlaniyor; GitHub Actions dosyasi mevcut; native, native_link, static analysis ve architecture policy yerelde basarili. Uzak CI sonucu GitHub tarafinda izlenir. |
| Estimator / EKF hazirligi | Tamamlandi | `EstimatedState`, `ComplementaryEstimator`, testler ve EKF tasarim notu mevcut. |
| Mimari modularite | Basladi | `RCPipeline` ve `SensorPipeline` ile FlightManager kucultuldu; ControlPipeline, FailsafeManager, HAL ve scheduler sonraki mimari fazda. |
| GPS / otonom mod | Bilincli olarak ertelendi | Altyapi hazirlandi; waypoint, RTH ve GPS parser simdilik kapsam disi. |

## Yerel Dogrulama

- `pio test -e native`: 33 test case basarili.
- `pio run -e pico`: Pico firmware derlemesi basarili.
- Pico build bellek durumu: RAM yaklasik %5.3, flash yaklasik %2.8.

## Donanim Bekleyen Maddeler

1. FreeRTOS FlightTask heartbeat ve boot davranisi.
2. SBUS UART0/GP1 fiziksel okuma.
3. Sensor health ve blackbox alanlarinin gercek sensorlerle dogrulanmasi.
4. Bench checklist sonuclarinin islenmesi.
5. Kalibrasyon storage icin flash/LittleFS implementasyonunun guc kesintisi senaryosuyla test edilmesi.

## Kalan Urunlestirme Riskleri

- Proje klasoru henuz git deposu degil; degisikliklerin surumlenmesi icin `git init` veya mevcut uzak repo ile baglanti kurulmasi gerekiyor.
- Uzak CI sonucu ancak GitHub repo baglantisi ve push/PR sonrasi dogrulanabilir.
- Kalibrasyon saklama API'si hazir, fakat gercek kalici flash yazma henuz uygulanmadi.
- Estimator prototipi ucus akisina baglanmadi; once bench ve log dogrulamasi yapilmali.

## Inovasyon Icin Hazir Zemin

- Sensor health, timing budget, blackbox ve MAVLink gorunurlugu artik inovasyon fikirlerini olculebilir hale getirecek temel veriyi uretebilir.
- Estimator katmani donanimdan bagimsiz oldugu icin EKF, adaptive filtering ve kalite skoru gibi fikirler test-first ilerletilebilir.
- Kod yapisi `board/`, `drivers/`, `filters/`, `storage/`, `estimators/`, `core/`, `telemetry/` ayrimina cekildigi icin yeni ozellikler daha az yan etkiyle eklenebilir.
