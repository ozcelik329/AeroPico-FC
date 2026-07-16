/*
  AeroPico QMC5883P-like magnetometer calibrator for ESP8266.

  This GY-87 variant exposes the magnetometer at I2C address 0x2C.
  Bench probing showed the useful XYZ stream at register 0x01, big-endian.

  Wiring:
    GY-87 VCC -> ESP8266 3V3
    GY-87 GND -> ESP8266 GND
    GY-87 SDA -> ESP8266 D2 / GPIO4
    GY-87 SCL -> ESP8266 D1 / GPIO5
*/

#include <Arduino.h>
#include <Wire.h>

static constexpr uint8_t PIN_SDA = D2;
static constexpr uint8_t PIN_SCL = D1;
static constexpr uint8_t MPU_ADDR = 0x68;
static constexpr uint8_t MAG_ADDR = 0x2C;

static int16_t minX = 32767;
static int16_t minY = 32767;
static int16_t minZ = 32767;
static int16_t maxX = -32768;
static int16_t maxY = -32768;
static int16_t maxZ = -32768;
static uint32_t lastPrintMs = 0;
static uint32_t sampleCount = 0;

static bool writeRegAt(uint8_t addr, uint8_t reg, uint8_t val) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.write(val);
  return Wire.endTransmission() == 0;
}

static bool writeReg(uint8_t reg, uint8_t val) {
  return writeRegAt(MAG_ADDR, reg, val);
}

static bool readBytes(uint8_t reg, uint8_t *buf, uint8_t len) {
  Wire.beginTransmission(MAG_ADDR);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(MAG_ADDR, len) != len) return false;
  for (uint8_t i = 0; i < len; ++i) buf[i] = Wire.read();
  return true;
}

static int16_t be16(const uint8_t *p) {
  return static_cast<int16_t>((static_cast<uint16_t>(p[0]) << 8) | p[1]);
}

static bool magPresent() {
  Wire.beginTransmission(MAG_ADDR);
  return Wire.endTransmission() == 0;
}

static void enableMpuBypass() {
  writeRegAt(MPU_ADDR, 0x6B, 0x00);  // wake MPU6050
  delay(20);
  writeRegAt(MPU_ADDR, 0x6A, 0x00);  // disable MPU I2C master mode
  writeRegAt(MPU_ADDR, 0x37, 0x02);  // enable bypass to aux I2C
  delay(20);
}

static void initMag() {
  // Conservative init sequence for the observed QMC5883P-like 0x2C variant.
  // If a clone ignores one of these writes, the data-read path still exposes it.
  writeReg(0x0A, 0xC3);
  delay(10);
  writeReg(0x0B, 0x08);
  delay(10);
  writeReg(0x0C, 0x01);
  delay(50);
}

static bool readMag(int16_t &x, int16_t &y, int16_t &z) {
  uint8_t b[6];
  if (!readBytes(0x01, b, sizeof(b))) return false;
  x = be16(&b[0]);
  y = be16(&b[2]);
  z = be16(&b[4]);
  return true;
}

static void updateMinMax(int16_t x, int16_t y, int16_t z) {
  if (x < minX) minX = x;
  if (y < minY) minY = y;
  if (z < minZ) minZ = z;
  if (x > maxX) maxX = x;
  if (y > maxY) maxY = y;
  if (z > maxZ) maxZ = z;
}

static void printCalibration() {
  const float offX = (minX + maxX) * 0.5f;
  const float offY = (minY + maxY) * 0.5f;
  const float offZ = (minZ + maxZ) * 0.5f;
  const float spanX = (maxX - minX) * 0.5f;
  const float spanY = (maxY - minY) * 0.5f;
  const float spanZ = (maxZ - minZ) * 0.5f;
  const float avgSpan = (spanX + spanY + spanZ) / 3.0f;
  const float scaleX = spanX > 1.0f ? avgSpan / spanX : 1.0f;
  const float scaleY = spanY > 1.0f ? avgSpan / spanY : 1.0f;
  const float scaleZ = spanZ > 1.0f ? avgSpan / spanZ : 1.0f;

  Serial.printf("samples=%lu\n", static_cast<unsigned long>(sampleCount));
  Serial.printf("mag_min_raw = { %d, %d, %d }\n", minX, minY, minZ);
  Serial.printf("mag_max_raw = { %d, %d, %d }\n", maxX, maxY, maxZ);
  Serial.printf("mag_hard_iron_offset_raw = { %.1f, %.1f, %.1f }\n", offX, offY, offZ);
  Serial.printf("mag_soft_iron_scale = { %.4f, %.4f, %.4f }\n", scaleX, scaleY, scaleZ);
}

void setup() {
  Serial.begin(115200);
  delay(500);

  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(100000);

  Serial.println();
  Serial.println("AeroPico QMC5883P-like mag calibrator");
  Serial.println("I2C address: 0x2C, data: BE @ 0x01");
  enableMpuBypass();
  Serial.print("present: ");
  Serial.println(magPresent() ? "yes" : "no");

  initMag();
  Serial.println("Rotate the board slowly through all orientations for 60-90 seconds.");
  Serial.println("Use the final min/max/offset/scale values after all axes have moved widely.");
}

void loop() {
  int16_t x, y, z;
  if (readMag(x, y, z)) {
    updateMinMax(x, y, z);
    ++sampleCount;
  } else {
    Serial.println("mag read failed");
    delay(500);
    return;
  }

  const uint32_t now = millis();
  if (now - lastPrintMs >= 500) {
    lastPrintMs = now;
    Serial.printf("mag_raw = { %d, %d, %d }\n", x, y, z);
    printCalibration();
    Serial.println("rotate board...");
    Serial.println();
  }
}
