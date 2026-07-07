# AeroPico FC - Inovasyon Beyin Firtinasi

Tarih: 2026-07-07

Bu fikirler GPS/otonom moda hemen girmeden, mevcut guvenlik ve test altyapisinin uzerine kurulabilecek farklilastirici gelistirmelerdir.

## En Yuksek Getiri Verecek 8 Fikir

| Oncelik | Fikir | Neden Degerli | Ilk Adim |
|---|---|---|---|
| 1 | Sensor guven skoru | Sadece OK/FAIL yerine IMU, baro, mag ve RC icin kalite puani verir. GCS ve pre-arm kararlarini akillandirir. | `SensorHealth` yanina `SensorQuality` skoru ve stale yas alanlari ekle. |
| 2 | Bench auto-test modu | Ticari urun hissi verir: servo sweep, RC kanal haritasi, sensor hareket testi tek komutla yapilir. | MAVLink komutu veya param ile test modunu tetikle, sonucu blackbox/GCS'e yaz. |
| 3 | Adaptive filtering | Titreşim ve manevra durumuna gore median/IIR agirligini degistirir. Daha sakin ama gecikmesi dusuk kontrol saglar. | IMU varyansina gore filtre pencere/alpha degerini secen kucuk politika yaz. |
| 4 | Timing-aware control | Loop jitter veya budget ihlali artinca sistemi kendini koruyan moda alir. | `TimingBudgetStatus` uzerinden PID cikis yumuşatma veya status text uret. |
| 5 | EKF-lite altitude estimator | Tam EKF'ye atlamadan baro + ivme Z ile daha iyi altitude/vertical speed cikarir. | ComplementaryEstimator'a acceleration-Z opsiyonel girdisi ve innovation testi ekle. |
| 6 | Pre-arm explainability | Kullaniciya neden arm olmadigini acik soyler: RC yok, sensor stale, kalibrasyon eksik, timing ihlali vb. | `PreArmStatus` veri tipi ve MAVLink STATUSTEXT mesajlari ekle. |
| 7 | Blackbox flight score | Her bench/ucus sonunda kalite skoru uretir: failsafe sayisi, stale sensor, max timing, max roll/pitch. | Blackbox summary accumulator yaz. |
| 8 | Parametre profilleri | Maiden/stable/agile gibi profil setleriyle ayar yonetimi kolaylasir. | ParamManager icin profil isimleri ve RAM storage prototipi ekle. |

## PX4/ArduPilot'a Karsi Farklilasabilecek Yonalim

- Daha kucuk ve okunabilir sabit kanat cekirdegi.
- Bench-first kurulum: yeni kullanici once masada sistemi dogrular, sonra ucar.
- GCS'e anlasilir durum mesajlari: sadece hata kodu degil, neden ve onerilen aksiyon.
- RP2350 hedefinde dusuk bellek kullanimi ve hizli build/test dongusu.
- Ucus kontrolcusu + egitim platformu karakteri: kodu kurcalayan kullanici sistemi anlayabilir.

## Hemen Baslanabilecek Mini Faz

1. `SensorQuality` veri tipini tasarla.
2. IMU stale yasi, RC override yasi ve timing budget ihlallerini tek health snapshot'ta topla.
3. Pre-arm status prototipi yaz.
4. Native testleri ekle.
5. Blackbox/MAVLink'e sade ozet alanlari tasima planini cikar.

## Simdilik Bekletilecek Fikirler

- Tam waypoint navigation.
- Return-to-home.
- Tam TECS/L1 kontrol.
- Buyuk SITL/HIL simulatoru.
- Çok platformlu multirotor destekleri.

Bu bekletilenler degerli, ama once AeroPico'nun ayirt edici tarafi olan hafif, testli, anlasilir ve sabit kanat odakli cekirdegi olgunlastirmak daha iyi yatirim.
