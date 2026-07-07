# AeroPico FC - Faz 8 Profesyonel FC Altyapisi

Hedef: RP2350 icin en temiz mimariye sahip acik kaynak sabit kanat flight controller cekirdegi.

Bu fazin amaci yeni ucus ozelligi eklemek degil; HAL, scheduler, preflight health, runtime parametre ve logging standartlarini oturtmaktir.

## 1. HAL

Baslatildi:

- `src/hal/HAL_GPIO.h`
- `src/hal/HAL_PWM.h`
- `src/hal/HAL_I2C.h`
- `src/hal/HAL_SPI.h`
- `src/hal/HAL_UART.h`
- `src/hal/HAL_Timer.h`
- `src/hal/rp2350/RP2350_Timer.*`
- `src/hal/rp2350/RP2350_PWM.*`

Sonraki adim: `Sensors`, `RX`, `Output` ve `PioUart` icindeki dogrudan RP2350/Arduino erisimlerini bu arayuzlerin arkasina kademeli tasimak.

## 2. Deterministik Scheduler

Baslatildi:

- `src/core/Scheduler.*`
- `test/test_scheduler/`

Hedef frekanslar:

- 400Hz control loop
- 200Hz IMU update
- 100Hz attitude estimation
- 50Hz RC input
- 20Hz MAVLink telemetry
- 10Hz GPS/barometer
- 5Hz logging
- 1Hz health report

Sonraki adim: Mevcut FreeRTOS task yapisini koruyarak once telemetry/health/log gibi dusuk riskli isleri scheduler'a baglamak.

## 3. Health / Preflight

Baslatildi:

- `src/core/PreflightHealth.*`
- `test/test_preflight/`

Ilk desteklenen karar:

- ARM olabilir mi?
- Hayirsa ilk sebep ne?
- Kac zorunlu check basarisiz?

Sonraki adim: BootCheck, SensorCheck, RCCheck, BatteryCheck, MemoryCheck ve FailsafeCheck kaynaklarini gercek sistem durumuna baglamak.

## 4. Parametre Sistemi

Bekliyor:

- PID disinda servo min/max, reverse, trim, failsafe timeout, mixer gain ve RC mapping runtime parametre olmali.
- Kalici storage API hazir; ParamManager bunun uzerinden genisletilmeli.

## 5. Test ve CI

Mevcut durum:

- Scheduler testleri eklendi.
- Preflight testleri eklendi.
- Native test sayisi 46/46 basarili.
- Pico firmware derlemesi basarili.
- Watchdog besleme karari `WatchdogGate` ile test edilebilir hale getirildi; artik Core 0 yalnizca flight loop running, heartbeat fresh ve timing budget OK ise watchdog besler.

Sonraki testler:

- `test_hal_mock`
- `test_failsafe`
- `test_rc_loss`
- `test_param_profiles`
- `test_scheduler_overrun`
- `test_watchdog_gate`
