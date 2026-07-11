# AeroPico FC - Bench Test Checklist

Bu liste ilk enerji verme, firmware degisikligi ve ucus oncesi masa testleri icindir. GPS/otonom mod testleri kapsam disidir.

## 1. Guc ve Boot

- USB veya guvenli bench beslemesi kullan.
- Pervane takili olmasin.
- Battery ADC kullanilacaksa voltaj bolucu orani ve ADC pin baglantisi multimetreyle dogrulandi mi?
- Boot banner gorunuyor mu?
- Watchdog reset uyarisi varsa nedeni not edildi mi?
- Health report icinde IMU, DMA, RC, baro ve mag durumlari beklenen gibi mi?
- Ilk boot sonrasi kalibrasyon kaydi olustu mu, ikinci boot'ta `Calibration Load` gorunuyor mu?

## 2. Sensor Sagligi

- Kart sabitken gyro degerleri sifira yakin mi?
- Accel degerleri beklenen eksende yaklasik 1g gosteriyor mu?
- `SensorHealth` durumu `Ok` oluyor mu?
- Sensor quality skoru arm oncesi yeterli mi ve stale age raporu anlamli mi?
- Pre-arm reddinde `IMU not available`, sensor health veya quality nedeni acikca gorunuyor mu?
- Sensor kablosu/hatasi simule edildiginde stale/invalid durum blackbox veya telemetry'de gorunuyor mu?
- Median filtre aktifken ham verideki tekil sicramalar attitude hesabina belirgin yansimiyor mu?
- Mag/baro yardimci RX+TX DMA okumalarinda timeout veya kanal tahsis hatasi raporlanmiyor mu?
- Baro basinci azaltilinca EKF-lite relative altitude ve vertical-speed degerleri beklenen yonde degisiyor mu?

## 3. RC ve Failsafe

- SBUS UART0/GP1 uzerinden kanal verileri geliyor mu?
- Roll, pitch, throttle, yaw kanal sirasi dogru mu?
- Runtime RC kanal esleme parametreleri degistirildiginde dogru kanallar kullaniliyor mu?
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
- Runtime MAVLink stream hizlari ve blackbox log hizi ayarlanan frekanslara uyuyor mu?
- SYS_STATUS sensor health bitleri sensor durumuyla uyumlu mu?
- RC override verildiginde kanallar degisiyor mu?
- RC override kesilince timeout ile temizleniyor mu?
- Bir parametre degistirildikten sonra `PARAM_SAVE=1` gonderilip reboot sonrasi deger korunuyor mu?
- Blackbox satirinda `sensorHealth` alani gorunuyor mu?

## 6. Zamanlama ve Watchdog

- Core 1 heartbeat taze kaliyor mu?
- Core0 scheduler frekanslari beklenen seviyede mi: sensor/state 200Hz, RC 50Hz, preflight 20Hz, watchdog gate 100Hz?
- Core1 control loop 500Hz civarinda ve timing budget icinde mi?
- Timing max/average/jitter ve deadline-miss sayaclari uzun sureli bench kosusunda incelendi mi?
- Uzun sureli bench calismasinda watchdog reset yok mu?
- Bilerek Core 1 donmasi veya sensor task blokaj testi yapildiginda watchdog beklenen sekilde resetliyor mu?
- Timing budget ihlali varsa loga alindi mi?

## 7. Battery / Brownout

- `BATTERY_ADC_ENABLED` acilmadan once ADC pininde 0-3.3V araliginda guvenli sinyal var mi?
- Okunan voltaj multimetre ile uyumlu mu?
- Dusuk voltaj esiginde preflight battery check arm'i engelliyor mu?
- Brownout esiginin altinda neden `Battery brownout risk` olarak raporlaniyor mu?

## 8. Test Sonucu

- Test tarihi:
- Firmware commit/surum:
- Kart:
- Sensor paketi:
- RC alici:
- Sonuc: Gecti / Kaldi
- Notlar:
