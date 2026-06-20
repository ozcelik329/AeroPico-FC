

# AeroPico FC: Raspberry Pi Pico Sabit Kanat Uçuş Kontrolcüsü

Bu proje, Raspberry Pi Pico kullanarak geliştirilen, **sabit kanatlı uçaklara özgü** açık kaynaklı bir uçuş kontrolcüsü yazılımıdır. Proje, profesyonel sistem mühendisliği prensiplerine uygun olarak **katmanlı mimari (Layered Architecture)** ile yeniden yapılandırılmıştır.

## 📂 Dosya Yapısı ve Mimari

Projemiz, kodun sürdürülebilirliği ve modülerliği için şu şekilde organize edilmiştir:

* `src/main.cpp`: Sistemin kalbi, "Orkestra Şefi". `setup()` ve `loop()` döngülerini yönetir.
* `src/config.h`: Donanım pinleri, PID değerleri ve kullanıcı tarafından değiştirilebilir parametreler.
* `src/def.h`: Sistemin değişmezleri, MAVLink mesaj ID'leri ve matematiksel sabitler.
* `src/core/`: Uçuş modları (Stabilize, Manual, RTH) ve ana zamanlayıcı (SystemTimer) yönetimi.
* `src/drivers/`: Sensör (GY-87), Alıcı (SBUS) ve Motor/Servo (PWM) donanım sürücüleri.
* `src/telemetry/`: GCS (Yer İstasyonu) ile haberleşme için MAVLink protokolü işleyicisi.
* `src/utils/`: Matematiksel filtreler (Mahony/Madgwick), yardımcı fonksiyonlar ve Black Box (SD Kart) kayıt sistemi.

## 🏗 Mimari Prensipler

Profesyonel standartlarda çalışmak için şu kurallar benimsenmiştir:

1. **Modülerlik:** Her modül kendi başına bir sınıf (`class`) olarak tanımlanır.
2. **Bağımlılık Yönetimi:** Bilgi akışı `main.cpp` -> `core/` -> `drivers/` veya `telemetry/` hiyerarşisinde akar.
3. **Güvenlik:** `Header Guards` (#ifndef) kullanımı ve donanım bağımsızlığı (HAL).

## 🚀 Nasıl Derlenir?

1. PlatformIO ile projeyi açın.
2. `platformio.ini` dosyasında gerekli kütüphanelerin tanımlı olduğundan emin olun.
3. VS Code üzerindeki "Build" butonuna basın.
4. Oluşan `firmware.uf2` dosyasını `BOOTSEL` modundaki Pico'ya sürükleyin.

## 🛠 Donanım Gereksinimleri

* Raspberry Pi Pico
* GY-87 Sensör Modülü (MPU6050 + HMC5883L + BMP180)
* SBUS destekli alıcı
* ESC ve Fırçasız Motor / Servolar

## 📋 Gelecek Geliştirmeler (Roadmap)

* [ ] GCS (Yer İstasyonu) entegrasyonu ve MAVLink haberleşmesi.
* [ ] SD Kart ile uçuş verisi (Black Box) kaydı.
* [ ] İleri seviye sensör filtreleme (EKF/Mahony).
* [ ] Failsafe ve otomatik uçuş modları (RTH, Auto-Level).

---

### 💡 Geliştirici Notu

Proje, hobi seviyesinden profesyonel bir "Uçuş Bilgisayarı" seviyesine taşınmaktadır. Kod değişikliklerinde `config.h` (kullanıcı ayarları) ve `def.h` (sistem sabitleri) ayrımına dikkat edilmesi önemle rica olunur.

---

**README dosyasını bu şekilde güncellemek, projeyi dışarıdan inceleyecek kişiler (veya gelecekteki sen) için çok daha anlaşılır bir dökümantasyon sunacaktır.** Şimdi, bu dökümanı da bir `git commit` ile GitHub'a göndermek ister misin, yoksa doğrudan `config.h` üzerinde çalışmaya mı başlayalım?
