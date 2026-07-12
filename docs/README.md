# AeroPico FC Docs

Bu klasor yalnizca aktif ve bakimi suren proje dokumanlarini tutar.

## Ana Dokumanlar

- `AeroPico_FC_Iyilestirme_Yol_Haritasi.md`: Guncel uygulanabilir yol haritasi.
- `AeroPico_FC_v0.2.0_Professional_Software_Architecture_Review.md`: Guncel mimari ve risk inceleme raporu.
- `Project_Structure.md`: Kod katmanlari ve yeni dosya ekleme kurallari.

## Test ve Operasyon

- `AeroPico_FC_Kullanma_Kilavuzu.md`: Hic bilmeyen kullanici icin kurulum, yukleme, kalibrasyon, HIL ve ucus hazirlik kilavuzu.
- `AeroPico_FC_Gelistirici_Kullanma_Kilavuzu.md`: Hic bilmeyen gelistirici icin araclar, testler, CI, GitHub CLI ve gelistirme akisi.
- `Bench_Test_Checklist.md`: Donanim masasi dogrulama listesi.
- `First_Flight_Checklist.md`: Ilk ucus oncesi ve sonrasi kontrol listesi.

## Tasarim Notlari

- `Calibration_Persistence_Research.md`: Kalibrasyon saklama tasarimi.
- `EKF_Design_Note.md`: Estimator/EKF hazirlik notlari.
- Not: Mevcut `BaroVerticalKalman` tam-durum EKF degildir; yalnizca irtifa ve dikey hiz icin 2-durumlu bir Kalman filtresidir.
- `Innovation_Backlog.md`: Ileride urun farki yaratabilecek fikirler.
- `V1_0_Remaining_Work.md`: V1.0 icin kalan mimari, guvenlik, performans ve
  fiziksel dogrulama isleri.
- `External_Review_Findings_2026-07-09.md`: Harici PDF incelemesindeki
  P0/P1/P2 bulgularinin kod dogrulamasi ve kapanis takibi.

## Devre Disi Hazirliklar

- GPS altyapisi `src/drivers/gps/` altinda NMEA GGA parser ve UART manager olarak hazir tutulur; `GPS_MODULE_ENABLED=0` oldugu icin V1.0 manuel/stabilize akisini etkilemez.
- ESP32-CAM altyapisi `src/drivers/camera/` altinda link health izleme olarak hazir tutulur; `ESP32_CAM_LINK_ENABLED=0` oldugu icin varsayilan firmware'de kamera baglantisi baslatilmaz.

Eski faz snapshot'lari, tarihli brainstorm dosyalari ve guncel markdown raporunun eski PDF/DOCX export'lari tutulmaz.
