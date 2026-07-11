# AeroPico FC Gelistirici Kullanma Kilavuzu

Bu kilavuz, projeyi hic bilmeyen bir gelistiricinin kodu kurmasi, testleri calistirmasi, CI sonucunu okuması, GitHub'a degisiklik gondermesi ve mimari kurallara uygun gelistirme yapmasi icindir.

## 1. Gerekli Araclar

- Git
- Python 3
- PlatformIO
- Bir kod editoru
- GitHub hesabi
- Istege bagli: GitHub CLI (`gh`)

PlatformIO kurulumu:

```bash
python3 -m pip install -U platformio
```

Kontrol:

```bash
pio --version
```

## 2. Repo Kurulumu

```bash
git clone https://github.com/ozcelik329/AeroPico-FC.git
cd AeroPico-FC
git checkout pico2-catalyi
```

Mevcut branch ve durum:

```bash
git status --short --branch
```

Guncel kodu almak:

```bash
git pull origin pico2-catalyi
```

## 3. Proje Mantigi

AeroPico FC sabit kanat icin RP2350 tabanli bir flight controller yazilimidir. V1.0 hedefi otonom mission/RTL degil; manuel ve stabilize ucusun guvenli, testli, moduler altyapisidir.

Ana katmanlar:

- `src/core/`: ucus mantigi, scheduler, control pipeline, sensor fusion
- `src/drivers/`: sensor, RX, output ve donanim suruculeri
- `src/hal/`: platform soyutlama
- `src/estimators/`: altitude/vertical estimator
- `src/storage/`: kalibrasyon ve parametre saklama
- `src/telemetry/`: MAVLink, ParamManager, Blackbox
- `test/`: native testler
- `docs/`: operasyon, mimari ve gelistirme dokumanlari

Yeni dosya eklemeden once `docs/Project_Structure.md` okunmalidir.

## 4. Kodlama Kurallari

- Kritik ucus yolunda heap kullanma.
- ISR icinde log/printf yapma.
- Sensor ve control kodunda gereksiz virtual kullanma.
- Donanim ayrintisini `drivers/` veya `hal/` disina tasirma.
- Test yazmadan davranis degistirme.
- Buyuk siniflari kucuk, atomik modullere bol.
- Dosya 500 satira yaklasiyorsa sorumluluklari yeniden ayir.
- Performans kazanimi olcmeye calis; "zekice" kod okunabilirlik pahasina olmamali.

## 5. Testler Nasil Calistirilir

Tum native testler:

```bash
pio test -e native --without-uploading
```

Ayrı translation unit/link testi:

```bash
pio test -e native_link --without-uploading
```

Tek test:

```bash
pio test -e native --without-uploading --filter test_sensor_fusion
```

Pico firmware build:

```bash
pio run -e pico
```

Statik analiz:

```bash
pio check -e pico --skip-packages --fail-on-defect=high --silent
```

Mimari politika kontrolu:

```bash
python3 tools/ci/check_architecture.py
```

Fault injection smoke:

```bash
python3 tools/fault_injection/fault_injection.py --json-report fault_matrix_report.json
```

HIL smoke:

```bash
python3 tools/hil_smoke/hil_smoke.py --port /dev/ttyACM0
```

macOS port ornegi:

```bash
python3 tools/hil_smoke/hil_smoke.py --port /dev/tty.usbmodemXXXX
```

### Test ve Tool Sozlugu

Bu projede "test" sadece kodun derlenmesi degildir; flight controller gibi guvenlik kritik bir sistemde her katman farkli ariza turunu yakalar.

| Komut / Tool | Ne ise yarar | Ne zaman calistirilir |
|---|---|---|
| `pio test -e native --without-uploading` | Donanim olmadan bilgisayarda unit testleri kosar. PID, mixer, sensor fusion, battery monitor, failsafe, parametre sistemi gibi moduller burada dogrulanir. | Her kod degisikliginden sonra |
| `pio test -e native_link --without-uploading` | Ayri derlenen modullerin birlikte linklenmesini kontrol eder. Include, sembol ve build organizasyonu sorunlarini yakalar. | Buyuk refactor sonrasi |
| `pio run -e pico` | RP2350/Pico 2 firmware'ini gercek hedef icin derler. RAM/flash kullanimini gosterir. | Her merge/push oncesi |
| `pio check -e pico --skip-packages --fail-on-defect=high --silent` | Statik analiz yapar ve yuksek onemli hatalari build kirmiziya dusurur. | CI ve release oncesi |
| `python3 tools/ci/check_architecture.py` | Proje mimari kurallarini denetler. HAL disina donanim bagimliligi sizmasi gibi hatalari yakalar. | Her mimari degisiklikten sonra |
| `python3 tools/fault_injection/fault_injection.py --json-report fault_matrix_report.json` | Ariza matrisi ile testleri eslestirir. RC loss, battery brownout, sensor stale gibi senaryolarin testle kapatildigini kontrol eder. | Safety/failsafe degisikliginden sonra |
| `python3 tools/hil_smoke/hil_smoke.py --port ...` | Gercek Pico seri portu uzerinden temel hardware-in-the-loop smoke testi yapar. | Donanim bagliyken |
| `pio device list` | Bagli seri portlari listeler. HIL ve seri monitor portunu bulmak icin kullanilir. | Kart USB ile takiliyken |
| `pio device monitor -b 115200` | Firmware loglarini canli izler. Boot, sensor, calibration ve failsafe mesajlarini gormek icin kullanilir. | Bench test sirasinda |
| `git diff --check` | Satir sonu bosluklari ve patch format sorunlarini yakalar. | Commit oncesi |
| `gh run list`, `gh run watch` | GitHub Actions CI durumunu terminalden izler. | Push sonrasi |

Minimum merge kapisi:

```bash
pio test -e native --without-uploading
pio test -e native_link --without-uploading
python3 tools/ci/check_architecture.py
python3 tools/fault_injection/fault_injection.py --json-report fault_matrix_report.json
pio check -e pico --skip-packages --fail-on-defect=high --silent
pio run -e pico
git diff --check
```

## 6. GitHub Actions CI

CI dosyasi:

```text
.github/workflows/ci.yml
```

Her push ve pull request icin calisir:

- Pico firmware build
- Native unit tests
- Native full-link integration
- Fault injection smoke
- Static analysis
- Architecture policy check
- Software smoke tests
- Manuel HIL smoke

GitHub Actions sonucu web arayuzunden:

```text
https://github.com/ozcelik329/AeroPico-FC/actions
```

## 7. GitHub CLI Nedir

`gh`, GitHub'u terminalden kullanmayi saglayan resmi CLI aracidir.

Kurulumdan sonra:

```bash
gh auth login
```

CI run listeleme:

```bash
gh run list --branch pico2-catalyi --limit 5
```

Son run detay:

```bash
gh run view
```

Log izleme:

```bash
gh run watch
```

Pull request acma:

```bash
gh pr create --base pico2-catalyi --head <branch-adi>
```

`gh` yoksa GitHub web arayuzu veya REST API kullanilabilir.

## 8. Degisiklik Gonderme Akisi

1. Yeni branch ac:

```bash
git checkout -b codex/kisa-aciklama
```

2. Kod yaz.
3. Testleri calistir.
4. Durumu kontrol et:

```bash
git status --short
git diff --check
```

5. Commit:

```bash
git add <dosyalar>
git commit -m "Kisa ve net mesaj"
```

6. Push:

```bash
git push origin codex/kisa-aciklama
```

7. Pull request ac.

Dogudan `pico2-catalyi` branch'ine push sadece proje sahibi tarafindan veya anlasilan durumlarda yapilmalidir.

## 9. Kalibrasyon Modulu

IMU kalibrasyon hesaplamasi `src/drivers/sensors/SensorCalibration.*` icindedir.

Bu modul:

- Raw MPU sample toplar.
- Gyro bias hesaplar.
- Accel bias hesaplar.
- Z ekseninde 1g yercekimi ofsetini ayirir.
- Gyro sicaklik katsayisini olcer veya varsayilan guvenli degere duser.

`SensorManager` sadece ham ornek okur, sonucu uygular ve storage'a kaydedilecek `ImuCalibration` tipini uretir.

Bu siniri koru: kalibrasyon matematigi tekrar `Sensors.cpp` icine gomulmemeli.

## 10. Battery Monitor ve Voltaj Bolucu

Batarya izleme yolu su dosyalardan olusur:

- `src/config.h`: pin, ADC channel, divider ratio ve voltaj esikleri.
- `src/hal/HAL_ADC.h`: platform bagimsiz ADC arayuzu.
- `src/hal/rp2350/RP2350_ADC.*`: RP2350 ADC implementasyonu.
- `src/core/BatteryMonitor.*`: voltaj, low battery, brownout ve hysteresis karar mantigi.
- `src/main.cpp`: ADC provider'i BatteryMonitor'a baglar ve sonucu preflight/failsafe/event bus zincirine tasir.
- `test/test_battery_monitor/`: low voltage, brownout ve hysteresis unit testleri.

Varsayilan donanim:

```text
GP26 / ADC0
Rtop = 10k
Rbottom = 1k
Divider scale = (Rtop + Rbottom) / Rbottom = 11.0
```

Formul:

```text
ADC_V = VBAT * Rbottom / (Rtop + Rbottom)
VBAT  = ADC_V * BATTERY_VOLTAGE_DIVIDER_RATIO
```

Kodda `BATTERY_VOLTAGE_DIVIDER_RATIO` degistirilirse fiziksel direnclerle uyumlu olmali. Ornek: 20k/10k bolucu kullanilirsa ratio `3.0`, 100k/10k kullanilirsa ratio `11.0` olur.

Gelistirme notlari:

- ADC pini 3.3V uzerinde olamaz; bu yazilimla kurtarilabilecek bir hata degildir.
- Battery monitor mantigi donanimdan bagimsiz kalmali; testlerde voltage provider mock edilmelidir.
- Brownout karari anlik tek olcume baglanmamali; `BatteryMonitor` debounce ve recovery hysteresis kullanir.
- Esikler batarya kimyasina ve hucre sayisina gore parametrelesmelidir. 3S LiPo varsayimi config'tedir, baska batarya icin degistirilmelidir.

## 11. Yeni Test Eklerken

Test klasoru:

```text
test/test_<modul_adi>/test_<modul_adi>.cpp
```

Native testlerde donanim yoktur. Donanim bagimli kodlar icin:

- Mock backend kullan.
- HAL arayuzu kullan.
- `test/native_include/Arduino.h` stub'larini kullan.
- Gercek Pico SDK cagrisini test icine sokma.

## 12. PlatformIO Izin Sorunu

Hata ornegi:

```text
Operation not permitted: ~/.platformio/platforms.lock
```

Sebep: PlatformIO cache klasoru farkli kullanici/root tarafindan olusturulmus olabilir.

Cozum:

```bash
sudo chown -R $USER ~/.platformio
```

Alternatif:

```bash
rm -rf ~/.platformio/.cache
```

Sonra tekrar:

```bash
pio test -e native --without-uploading
```

## 13. Release ve Tag

Anlamli surum etiketi:

```bash
git tag -a v0.3-catalyi -m "AeroPico FC v0.3 catalyi"
git push origin v0.3-catalyi
```

Tag'i tasimak gerekiyorsa dikkatli ol:

```bash
git tag -f -a v0.3-catalyi -m "AeroPico FC v0.3 catalyi"
git push --force origin v0.3-catalyi
```

Tag tasima yalnizca release henuz kullaniciya dagitilmadiysa yapilmalidir.

## 14. Geliştiriciye Teslim Paketi

Projeyi yeni bir gelistiriciye verirken sunlari ver:

- Bu gelistirici kilavuzu
- `docs/Project_Structure.md`
- `docs/AeroPico_FC_Iyilestirme_Yol_Haritasi.md`
- `docs/V1_0_Remaining_Work.md`
- `docs/External_Review_Findings_2026-07-09.md`
- `.github/workflows/ci.yml`
- `platformio.ini`
- Son basarili CI linki
- Hangi branch/tag uzerinde calisilacagi
- Donanim olmadan calistirilmasi gereken test komutlari
- Donanim varsa HIL ve bench checklist
