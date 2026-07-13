# AeroPico FC - Ilk Ucus Checklist

Bu liste GPS/otonom mod kapsam disinda, manuel/stabilize temel ucus guvenligi icindir.

## Ucus Oncesi

- Pervane/motor guvenligi kontrol edildi.
- Batarya dolu ve sabitlenmis.
- Kontrol yuzeyleri mekanik olarak serbest.
- Servo yonleri dogru.
- CG ve agirlik merkezi dogrulandi.
- Firmware surumu ve parametreler not edildi.
- Bench checklist gecti.

## Elektronik

- Boot health report temiz.
- IMU `Ok`.
- RC kanallari dogru sirada.
- Failsafe testi yerde gecti.
- Blackbox kaydi basliyor.
- MAVLink heartbeat geliyor.
- Timing budget uyarisi yok.

## Kalkis Profili

- Ilk ucus manuel veya en dusuk riskli stabilize modda.
- Ruzgar dusuk ve sabit.
- Genis alan secildi.
- Ilk kalkista agresif manevra yok.
- Trim ihtiyaci not edilecek.

## Ucus Sirasinda

- Roll/pitch tepkileri normal mi?
- Osilasyon var mi?
- Gecikmeli kontrol hissi var mi?
- Failsafe veya sensor health uyarisi geldi mi?
- Motor/servo anormal davranis yok mu?

## Ucus Sonrasi

- Blackbox logu kaydedildi.
- Max roll/pitch/yaw rate incelendi.
- Timing max degerleri incelendi.
- Sensor health ve stale olaylari incelendi.
- PID gain ve trim notlari kaydedildi.
- Bir sonraki ucus icin tek degisiklik ilkesi uygulanacak.
