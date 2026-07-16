#include <Wire.h>

#define MAG 0x2C

bool writeReg(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(MAG);
  Wire.write(reg);
  Wire.write(val);
  return Wire.endTransmission() == 0;
}

bool readBytes(uint8_t reg, uint8_t *buf, uint8_t len) {
  Wire.beginTransmission(MAG);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(MAG, len) != len) return false;
  for (uint8_t i = 0; i < len; i++) buf[i] = Wire.read();
  return true;
}

int16_t le16(uint8_t *p) {
  return (int16_t)((p[1] << 8) | p[0]);
}

int16_t be16(uint8_t *p) {
  return (int16_t)((p[0] << 8) | p[1]);
}

void dumpRegs() {
  Serial.println("dump 0x00-0x1F:");
  for (uint8_t r = 0; r < 0x20; r++) {
    uint8_t v = 0xFF;
    readBytes(r, &v, 1);
    Serial.print("0x");
    if (r < 16) Serial.print("0");
    Serial.print(r, HEX);
    Serial.print("=");
    if (v < 16) Serial.print("0");
    Serial.print(v, HEX);
    Serial.print(" ");
    if ((r & 0x07) == 0x07) Serial.println();
  }
}

void printVec(const char *label, uint8_t startReg, bool littleEndian) {
  uint8_t b[6];

  if (!readBytes(startReg, b, 6)) {
    Serial.print(label);
    Serial.println(" read failed");
    return;
  }

  int16_t x = littleEndian ? le16(&b[0]) : be16(&b[0]);
  int16_t y = littleEndian ? le16(&b[2]) : be16(&b[2]);
  int16_t z = littleEndian ? le16(&b[4]) : be16(&b[4]);

  Serial.print(label);
  Serial.print(" = ");
  Serial.print(x); Serial.print(" ");
  Serial.print(y); Serial.print(" ");
  Serial.println(z);
}

void setup() {
  Serial.begin(115200);
  delay(500);

  Wire.begin(D2, D1);
  Wire.setClock(100000);

  Serial.println("0x2C mag probe");

  Wire.beginTransmission(MAG);
  Serial.print("present: ");
  Serial.println(Wire.endTransmission() == 0 ? "yes" : "no");

  dumpRegs();

  // Conservative QMC-like init attempts
  writeReg(0x0A, 0x00);
  delay(10);
  writeReg(0x0B, 0x01);
  delay(10);
  writeReg(0x09, 0x1D);
  delay(50);

  Serial.println("after QMC5883L-like init:");
  dumpRegs();

  // Possible QMC5883P-ish init attempts
  writeReg(0x0A, 0xC3);
  delay(10);
  writeReg(0x0B, 0x08);
  delay(10);
  writeReg(0x0C, 0x01);
  delay(50);

  Serial.println("after QMC5883P-like init:");
  dumpRegs();
}

void loop() {
  printVec("LE data@0x00", 0x00, true);
  printVec("LE data@0x01", 0x01, true);
  printVec("BE data@0x00", 0x00, false);
  printVec("BE data@0x01", 0x01, false);

  Serial.println("rotate board...");
  delay(500);
}