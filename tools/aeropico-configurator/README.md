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
- Kalibrasyon ve bench test ekranlari icin arayuz iskeleti
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

Mevcut firmware'de MAVLink `PARAM_REQUEST_LIST`, `PARAM_SET` ve `PARAM_VALUE`
akisi desteklenir. Configurator ilk surumde bu protokolu kullanir.

Modul var/yok durumu icin ilk surum heartbeat/status text/sys status
bilgilerinden yararlanir. Daha sonraki firmware adiminda tek bir capability
mesaji veya `STATUSTEXT` standardi eklenirse ekran daha kesin hale gelir.

## Bilinen Sinirlar

- Kalibrasyon ve bench test butonlari su an guvenli UI iskeletidir; firmware
  tarafinda komut akisi netlestikce MAVLink komutlarina baglanacaktir.
- Config denetimi cihazdan gelen parametre/status bilgisinin kalitesine baglidir.
- Bu arac ucus sirasinda kullanilacak bir GCS degildir.

## Bilincli Kapsam Disi

- Ucus sirasinda takip
- Harita / waypoint / mission
- Canli HUD
- RC override
- Log analiz platformu
