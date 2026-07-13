# AeroPico Configurator

AeroPico Configurator, AeroPico-FC icin ilk kurulum ve servis aracidir.
Ground Control Station degildir; ucus sirasinda harita, HUD, mission veya
canli kontrol amaciyla kullanilmaz.

Bu klasor repoda tutulacak temiz kaynak halidir. `node_modules`, macOS
metadata dosyalari ve paketlenmis zip arsivleri bilerek repoya alinmaz.

## Kapsam

- USB/serial ile flight controller'a baglanma
- MAVLink parametrelerini okuma
- PID, mixer, servo, RC ve failsafe parametrelerini yazma
- Parametreleri `PARAM_SAVE` ile flash'a kaydetme
- Moduller icin ilk durum ekrani
- Kalibrasyon ve bench test komutlarini MAVLink servis komutu olarak firmware'e gonderme
- Her servis komutu icin pending / accepted / rejected durum takibi
- Heartbeat uzerinden armed/disarmed durumunu gosterme
- Armed durumda IMU/Mag kalibrasyon ve servo test gibi riskli komutlari kilitleme
- AeroPico varsayilan pin haritasi
- Servo, RC, failsafe ve telemetry parametreleri icin istemci tarafi aralik
  dogrulamasi
- Basit config denetimi: servo araligi, RC kanal cakismasi, batarya/RC pin
  atamasi ve modul durumu

## Calistirma

```bash
cd tools/aeropico-configurator
npm install
npm start
```

Hizli syntax kontrolu:

```bash
npm run check
```

## Guvenlik ve Paketleme Notlari

- Electron `contextIsolation`, `sandbox` ve `nodeIntegration: false` ile
  calisir.
- Renderer yalnizca preload uzerinden sinirli serial-port secimi yapabilir.
- Arayuz dis ag fontu veya CDN kullanmaz; offline calisacak sekilde sistem
  fontlariyla tasarlanmistir.
- Zip icinde `node_modules` varsa bu sadece yerel tasima kolayligi icindir;
  gelistirme kaynagi olarak bu klasor kullanilmalidir.

## Beklenen Firmware Tarafi

Mevcut firmware'de MAVLink `PARAM_REQUEST_LIST`, `PARAM_SET`, `PARAM_VALUE`
ve AeroPico servis komutlari desteklenir. Servis komutlari `COMMAND_LONG`
icindeki `MAV_CMD_USER_1` ile tasinir:

| UI komutu | Firmware aksiyonu |
| --- | --- |
| IMU Kalibrasyon | IMU bias kalibrasyonu ve flash kaydi |
| Mag Kalibrasyon | Ilk basista toplama baslatma, ikinci basista hard-iron kaydetme |
| RC Mapping Kontrol | Simdilik parametre tabanli RC mapping'e yonlendirilir |
| Servo Yon Testi | Sadece disarmed/safe durumda kisa servo pulse testi |
| RC Kanal Kontrol | Receiver valid/failsafe durumunu sorgular |
| Sensor Kontrol | IMU zorunlu, mag/baro opsiyonel capability durumunu sorgular |
| Preflight Kontrol | Firmware arming kapisindaki ilk red/OK sebebini ister |

Modul var/yok durumu icin ilk surum heartbeat/status text/sys status
bilgilerinden yararlanir. Firmware her servis sonucunu `COMMAND_ACK` ve
aciklayici `STATUSTEXT` ile dondurur.

## Bilinen Sinirlar

- RC range kalibrasyonu henuz otomatik kanal min/max ogrenmez; mevcut firmware
  bunu runtime RC mapping parametreleriyle yonetir.
- Config denetimi cihazdan gelen parametre/status bilgisinin kalitesine baglidir.
- Bu arac ucus sirasinda kullanilacak bir GCS degildir.

## Bilincli Kapsam Disi

- Ucus sirasinda takip
- Harita / waypoint / mission
- Canli HUD
- RC override
- Log analiz platformu
