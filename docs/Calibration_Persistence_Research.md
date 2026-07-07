# AeroPico FC - Kalibrasyon Parametrelerini Kalici Saklama Arastirmasi

Hedef: IMU ve manyetometre kalibrasyonlarini her acilista yeniden yapmak zorunda kalmadan guvenli sekilde saklamak.

## Saklanacak Veriler

- IMU gyro bias: X/Y/Z
- IMU accel bias: X/Y/Z
- Manyetometre hard-iron bias: X/Y/Z
- Kalibrasyon versiyonu
- CRC veya basit checksum

## Onerilen Veri Yapisi

```cpp
struct CalibrationBlob {
    uint32_t magic;
    uint16_t version;
    uint16_t size;
    ImuCalibration imu;
    MagCalibration mag;
    uint32_t checksum;
};
```

## Saklama Secenekleri

1. RP flash son sayfa
   - Arti: Ek dosya sistemi gerekmez.
   - Eksi: Flash erase/write dikkat ister, wear-leveling yoktur.

2. LittleFS
   - Arti: Dosya olarak saklama kolaydir.
   - Eksi: PlatformIO/partition ayari ve runtime dosya sistemi bakimi gerekir.

3. GCS uzerinden parametre olarak saklama
   - Arti: MAVLink parametre sistemiyle uyumlu.
   - Eksi: Çok sayida parametre ve atomiklik sorunu.

## Ilk Uygulama Onerisi

Ilk ticari-gibi ama sade cozum:

- `src/storage/CalibrationStorage.h/.cpp` ac.
- `load(CalibrationBlob&)` ve `save(const CalibrationBlob&)` API'si tasarla.
- Ilk versiyonda yalnizca RAM/mock implementasyon ile native test yaz.
- Donanim dogrulamasi sonrasi flash implementasyon ekle.

## Guvenlik Kurallari

- Checksum gecersizse kalibrasyon uygulanmaz.
- Magic/version uyumsuzsa kalibrasyon uygulanmaz.
- IMU bias degerleri makul fiziksel sinirlar disindaysa reddedilir.
- Kayit sirasinda guc kesilirse eski gecerli kayit korunmalidir.

## Durum

- `ImuCalibration` ve `MagCalibration` veri tipleri eklendi.
- `SensorManager` bu kalibrasyonlari disaridan alip uygulayabilecek hale getirildi.
- `src/storage/CalibrationStorage.h/.cpp` eklendi.
- `CalibrationBlob` icin magic, version, size ve checksum dogrulamasi var.
- `MemoryCalibrationStorage` ile native test edilebilir storage implementasyonu eklendi.
- `test/test_calibration_storage/` icerisinde gecerli blob, bozuk blob reddi ve round-trip testleri var.
- Kalici flash yazma henuz uygulanmadi; donanim dogrulamasi sonrasi RP flash son sayfa veya LittleFS implementasyonu eklenecek.
