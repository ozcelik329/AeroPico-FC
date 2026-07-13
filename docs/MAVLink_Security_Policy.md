# AeroPico-FC MAVLink Security Policy

Kapsam: V1.0 manuel sabit kanat altyapisi. MAVLink signing henuz uygulanmamistir; bu nedenle firmware tarafinda operasyonel guvenlik kapilari varsayilan olarak korumaci tasarlanir.

## Varsayilan Politika

| Islem | Disarmed | Armed |
|---|---|---|
| Heartbeat / telemetry | Izinli | Izinli |
| Parametre listeleme | Izinli | Izinli |
| Runtime parametre degistirme | Izinli | Reddedilir |
| `PARAM_SAVE` flash yazma | Izinli | Reddedilir |
| RC override | Izinli | Varsayilan reddedilir |

## RC Override

- `RC_CHANNELS_OVERRIDE` yalnizca hedef `MAV_SYSTEM_ID` / `MAV_COMPONENT_ID` uyumluysa islenir.
- `_rcOverrideEnabled=false` ise her durumda reddedilir.
- `_rcOverrideAllowedWhileArmed=false` varsayilandir; armed durumda override kabul edilmez.
- Armed override sadece bilincli entegrasyon testlerinde `setRCOverrideAllowedWhileArmed(true)` ile acilmalidir.

## Parametre Yazma

- Parametre yazma ve kalici kayit `ParamManager` icinde arm-state provider ile korunur.
- Armed durumda runtime degisiklik reddedilir ve `getLastError()` ile neden okunabilir.

## Signing Yol Haritasi

1. MAVLink signing anahtar storage tasarimi.
2. Configurator tarafinda key provisioning akisi.
3. Armed durumda command/override icin signed frame zorunlulugu.
4. Failsafe veya bench modunda unsigned read-only telemetry izni.

