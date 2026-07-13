# AeroPico-FC Risk Matrix

Tarih: 2026-07-13
Kapsam: Yazilimsal risk taramasi, performans kokulari ve RC1 oncesi duzeltmeler.

| Risk | Seviye | Durum | Aksiyon |
|---|---:|---|---|
| Blackbox kayit uretimi ile UART yaziminin ayni anda yapilmasi kisa sureli telemetry bloklama yaratabilir. | Orta | Kapali | Blackbox frame'leri `ThreadSafeRingBuffer` uzerinden kuyruga alindi; scheduler ayri `blackbox-drain` isiyle tam frame sigdiginda yazar. |
| Parca parca UART yazimi blackbox binary frame senkronunu bozabilir. | Orta | Kapali | `PioUart::availableForWrite()` eklendi; Blackbox yalnizca tam frame sigdiginda yazar. |
| `ThreadSafeRingBuffer` test edilmis ama production path'e bagli degildi. | Dusuk | Kapali | Blackbox telemetry kuyrugu olarak aktif kullanima alindi. Kritik kontrol/sensor veri yolu seqlock blackboard olarak kaldi. |
| Ring buffer indeks ilerletmede genel `%` operatoru gereksiz maliyet yaratabilir. | Dusuk | Kapali | Power-of-two buffer boyutlarinda bit maskesiyle wrap-around kullanildi; non-power-of-two boyutlar icin genel yol korundu. |
| CI/fault-injection raporu untracked dosya olarak calisma agacini kirletebilir. | Dusuk | Kapali | `fault_matrix_report.json` `.gitignore` kapsamina alindi. |
| Boot sensor init/kalibrasyonunda `delay()` bulunuyor. | Dusuk | Kabul | Bu cagri hot flight path'te degil; boot-only davranis. Donanim bench sirasinda boot suresi ve sensor hazirlik mesaji izlenecek. |
| Donanim PWM/SBUS/battery/watchdog/HIL dogrulamasi fiziksel ekipman gerektiriyor. | Yuksek | Bekliyor | Yazilim CI temiz; fiziksel bench checklist ile dogrulanacak. |

## Performans Notlari

- Blackbox artik record uretiminde UART kuyruguna dogrudan basmak yerine sabit boyutlu frame kuyrugu kullaniyor.
- `blackbox-drain` gorevi 100 Hz calisir ve her tick'te en fazla iki record bosaltir; telemetry task'in uzun sure UART'a gomulmesi engellenir.
- Kuyruk boyutu 8 slot tutuldu. Bu, kisa sureli UART tikanmalarini sönumlerken SRAM kullanimini dusuk tutar.
- Kritik kontrol ve sensor path'inde heap allocation eklenmedi.

