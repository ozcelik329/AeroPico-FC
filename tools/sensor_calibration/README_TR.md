# AeroPico Sensör Kalibrasyon Araçları

Bu klasördeki araçlar, AeroPico firmware yüklemeden önce sensör kartını ayrı bir mikrodenetleyiciyle doğrulamak için kullanılır.

## ESP8266 GY-87 Kalibratör

Dosya:

```text
tools/sensor_calibration/esp8266_gy87_calibrator/ESP8266_GY87_Calibrator/ESP8266_GY87_Calibrator.ino
```

Arduino IDE'de bu `.ino` dosyasını aç.

Kart seçimi:

```text
NodeMCU 1.0 (ESP-12E Module)
```

Seri monitör:

```text
115200 baud
```

Bağlantı:

```text
GY-87 VCC  -> ESP8266 3V3
GY-87 GND  -> ESP8266 GND
GY-87 SDA  -> ESP8266 D2 / GPIO4
GY-87 SCL  -> ESP8266 D1 / GPIO5
```

Kesinlikle 5V pull-up veya 5V I2C kullanma.

## Test Sırası

1. Kartı düz ve hareketsiz koy.
2. Sketch'i yükle.
3. İlk birkaç saniye boyunca karta dokunma.
4. Seri monitörde şu satırları bekle:

```text
MPU6050: OK
BMP085/BMP180: OK
Mag 0x2C: present, simple QMC read trial
```

5. `MPU offsets` ve `ground_pressure_pa` çıktısını not al.
6. Live mode başladıktan sonra sensör kartını her yönde yavaşça döndür.
7. `mag_min`, `mag_max` ve `mag_hard_iron_offset_raw` değerlerini not al.

## Beklenen Değerler

MPU6050:

- Düz zeminde `accel_g` yaklaşık `{ 0, 0, 1 }` olmalı.
- Hareketsizken `gyro_dps` değerleri sıfıra yakın olmalı.

BMP085/BMP180:

- Basınç çoğu ortamda yaklaşık `95000-103000 Pa` aralığında olur.
- Sıcaklık oda sıcaklığına yakın olmalı.

Manyetometre:

- Standart QMC5883L genelde `0x0D` adresindedir.
- Bu GY-87 varyantında QMC-family cihaz `0x2C` olarak görünebilir.
- Kart döndürülünce `mag_raw` değerleri anlamlı şekilde değişmelidir.

## Not

Bu araç uçuş firmware'i değildir. Sadece sensör bring-up ve ilk kalibrasyon içindir.

## QMC5883P / 0x2C Manyetometre Kalibratörü

Bu GY-87 varyantında manyetometre `0x2C` adresinde göründü ve denemede en anlamlı veri şu profilden geldi:

```text
Adres: 0x2C
Veri başlangıcı: 0x01
Endian: big-endian
```

Arduino IDE'de şu dosyayı aç:

```text
tools/sensor_calibration/esp8266_qmc5883p_mag_calibrator/ESP8266_QMC5883P_Mag_Calibrator/ESP8266_QMC5883P_Mag_Calibrator.ino
```

Seri monitör:

```text
115200 baud
```

Kullanım:

1. Sketch'i ESP8266'ya yükle.
2. Seri monitörde `present: yes` gör.
3. Sensör kartını 60-90 saniye boyunca her eksende yavaşça döndür.
4. En son görünen şu değerleri not al:

```text
mag_min_raw
mag_max_raw
mag_hard_iron_offset_raw
mag_soft_iron_scale
```
