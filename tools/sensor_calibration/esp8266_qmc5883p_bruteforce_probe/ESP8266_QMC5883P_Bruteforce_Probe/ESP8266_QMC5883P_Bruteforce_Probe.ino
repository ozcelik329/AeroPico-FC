#include <Wire.h>

// ESP8266 NodeMCU wiring:
// GY-87 VCC -> 3V3, GND -> GND, SDA -> D2/GPIO4, SCL -> D1/GPIO5.
// This probe deliberately tries several QMC/HMC-like profiles because some
// GY-87 boards ship with undocumented 0x2C magnetometer clones.

static constexpr uint8_t MPU_ADDR = 0x68;
static constexpr uint8_t MAG_ADDR = 0x2C;
static constexpr uint32_t SAMPLE_DELAY_MS = 25;
static constexpr uint16_t SAMPLES_PER_WINDOW = 80;

struct RegWrite {
  uint8_t reg;
  uint8_t value;
};

struct Profile {
  const char* name;
  const RegWrite* writes;
  uint8_t writeCount;
};

static const RegWrite qmc5883l_1d[] = {
  {0x0B, 0x01}, // set/reset period
  {0x09, 0x1D}, // continuous, 200 Hz, 8G, OSR high
};

static const RegWrite qmc5883l_0d[] = {
  {0x0A, 0x80}, // soft reset on many QMC5883L clones
  {0x0B, 0x01},
  {0x09, 0x0D}, // continuous, 200 Hz-ish, lower range variant
};

static const RegWrite qmc5883l_11[] = {
  {0x0B, 0x01},
  {0x09, 0x11}, // continuous, slower ODR variant
};

static const RegWrite qmc5883p_c3[] = {
  {0x0B, 0x01},
  {0x0A, 0x40},
  {0x09, 0xC3},
};

static const RegWrite qmc5883p_0a_c3[] = {
  {0x0B, 0x01},
  {0x0A, 0xC3},
};

static const RegWrite qmc5883p_0a_cd[] = {
  {0x0B, 0x01},
  {0x0A, 0xCD},
};

static const RegWrite qmc5883p_0a_dd[] = {
  {0x0B, 0x01},
  {0x0A, 0xDD},
};

static const RegWrite qmc5883p_0a_1d[] = {
  {0x0B, 0x01},
  {0x0A, 0x1D},
};

static const RegWrite qmc5883p_0a_41[] = {
  {0x0B, 0x01},
  {0x0A, 0x41},
};

static const RegWrite qmc5883p_0a_81[] = {
  {0x0B, 0x01},
  {0x0A, 0x81},
};

static const RegWrite qmc5883p_0a_85[] = {
  {0x0B, 0x01},
  {0x0A, 0x85},
};

static const RegWrite qmc5883p_0a_8d[] = {
  {0x0B, 0x01},
  {0x0A, 0x8D},
};

static const RegWrite qmc5883p_80[] = {
  {0x0B, 0x01},
  {0x0D, 0x40},
  {0x0A, 0x80},
};

static const RegWrite qmc5883p_40[] = {
  {0x0B, 0x01},
  {0x0D, 0x80},
  {0x0A, 0x40},
};

static const RegWrite hmc_like[] = {
  {0x00, 0x70},
  {0x01, 0xA0},
  {0x02, 0x00},
};

static const RegWrite minimal_continuous[] = {
  {0x09, 0x01},
};

static const Profile profiles[] = {
  {"no-init", nullptr, 0},
  {"hmc-like-at-2c", hmc_like, (uint8_t)(sizeof(hmc_like) / sizeof(hmc_like[0]))},
  {"qmc5883l-1d", qmc5883l_1d, (uint8_t)(sizeof(qmc5883l_1d) / sizeof(qmc5883l_1d[0]))},
  {"qmc5883l-0d", qmc5883l_0d, (uint8_t)(sizeof(qmc5883l_0d) / sizeof(qmc5883l_0d[0]))},
  {"qmc5883l-11", qmc5883l_11, (uint8_t)(sizeof(qmc5883l_11) / sizeof(qmc5883l_11[0]))},
  {"qmc5883p-c3", qmc5883p_c3, (uint8_t)(sizeof(qmc5883p_c3) / sizeof(qmc5883p_c3[0]))},
  {"qmc5883p-0a-c3", qmc5883p_0a_c3, (uint8_t)(sizeof(qmc5883p_0a_c3) / sizeof(qmc5883p_0a_c3[0]))},
  {"qmc5883p-0a-cd", qmc5883p_0a_cd, (uint8_t)(sizeof(qmc5883p_0a_cd) / sizeof(qmc5883p_0a_cd[0]))},
  {"qmc5883p-0a-dd", qmc5883p_0a_dd, (uint8_t)(sizeof(qmc5883p_0a_dd) / sizeof(qmc5883p_0a_dd[0]))},
  {"qmc5883p-0a-1d", qmc5883p_0a_1d, (uint8_t)(sizeof(qmc5883p_0a_1d) / sizeof(qmc5883p_0a_1d[0]))},
  {"qmc5883p-0a-41", qmc5883p_0a_41, (uint8_t)(sizeof(qmc5883p_0a_41) / sizeof(qmc5883p_0a_41[0]))},
  {"qmc5883p-0a-81", qmc5883p_0a_81, (uint8_t)(sizeof(qmc5883p_0a_81) / sizeof(qmc5883p_0a_81[0]))},
  {"qmc5883p-0a-85", qmc5883p_0a_85, (uint8_t)(sizeof(qmc5883p_0a_85) / sizeof(qmc5883p_0a_85[0]))},
  {"qmc5883p-0a-8d", qmc5883p_0a_8d, (uint8_t)(sizeof(qmc5883p_0a_8d) / sizeof(qmc5883p_0a_8d[0]))},
  {"qmc5883p-80", qmc5883p_80, (uint8_t)(sizeof(qmc5883p_80) / sizeof(qmc5883p_80[0]))},
  {"qmc5883p-40", qmc5883p_40, (uint8_t)(sizeof(qmc5883p_40) / sizeof(qmc5883p_40[0]))},
  {"minimal-cont", minimal_continuous, (uint8_t)(sizeof(minimal_continuous) / sizeof(minimal_continuous[0]))},
};

static bool writeReg(uint8_t addr, uint8_t reg, uint8_t value) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

static bool readRegs(uint8_t addr, uint8_t reg, uint8_t* dst, uint8_t len) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  const uint8_t got = Wire.requestFrom(addr, len);
  if (got != len) return false;
  for (uint8_t i = 0; i < len; ++i) {
    dst[i] = Wire.read();
  }
  return true;
}

static int16_t asLe(const uint8_t* p) {
  return (int16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static int16_t asBe(const uint8_t* p) {
  return (int16_t)(((uint16_t)p[0] << 8) | p[1]);
}

static void enableMpuBypass() {
  writeReg(MPU_ADDR, 0x6A, 0x00); // USER_CTRL: I2C master off
  delay(5);
  writeReg(MPU_ADDR, 0x37, 0x02); // INT_PIN_CFG: BYPASS_EN
  delay(10);
}

static void dumpRegs(uint8_t addr) {
  Serial.println("reg dump 0x00..0x1F");
  for (uint8_t base = 0; base < 0x20; base += 16) {
    for (uint8_t i = 0; i < 16; ++i) {
      uint8_t v = 0xFF;
      if (readRegs(addr, base + i, &v, 1)) {
        if (v < 16) Serial.print('0');
        Serial.print(v, HEX);
      } else {
        Serial.print("--");
      }
      Serial.print(i == 15 ? '\n' : ' ');
    }
  }
}

static void applyProfile(const Profile& profile) {
  enableMpuBypass();
  for (uint8_t i = 0; i < profile.writeCount; ++i) {
    writeReg(MAG_ADDR, profile.writes[i].reg, profile.writes[i].value);
    delay(8);
  }
  delay(80);
}

static void evaluateWindow(const char* profileName, uint8_t startReg, bool bigEndian) {
  int16_t first[3] = {0, 0, 0};
  int16_t last[3] = {0, 0, 0};
  int16_t minv[3] = {32767, 32767, 32767};
  int16_t maxv[3] = {-32768, -32768, -32768};
  uint16_t ok = 0;
  uint16_t saturated = 0;

  for (uint16_t sample = 0; sample < SAMPLES_PER_WINDOW; ++sample) {
    uint8_t raw[6] = {};
    if (readRegs(MAG_ADDR, startReg, raw, sizeof(raw))) {
      int16_t xyz[3];
      for (uint8_t axis = 0; axis < 3; ++axis) {
        xyz[axis] = bigEndian ? asBe(&raw[axis * 2]) : asLe(&raw[axis * 2]);
        if (xyz[axis] < minv[axis]) minv[axis] = xyz[axis];
        if (xyz[axis] > maxv[axis]) maxv[axis] = xyz[axis];
        if (xyz[axis] <= -32760 || xyz[axis] >= 32760) saturated++;
      }
      if (ok == 0) {
        first[0] = xyz[0]; first[1] = xyz[1]; first[2] = xyz[2];
      }
      last[0] = xyz[0]; last[1] = xyz[1]; last[2] = xyz[2];
      ok++;
    }
    delay(SAMPLE_DELAY_MS);
  }

  long span = 0;
  for (uint8_t axis = 0; axis < 3; ++axis) {
    span += (long)maxv[axis] - (long)minv[axis];
  }

  Serial.print(profileName);
  Serial.print(" start=0x");
  Serial.print(startReg, HEX);
  Serial.print(bigEndian ? " BE" : " LE");
  Serial.print(" ok=");
  Serial.print(ok);
  Serial.print(" sat=");
  Serial.print(saturated);
  Serial.print(" span=");
  Serial.print(span);
  Serial.print(" first={");
  Serial.print(first[0]); Serial.print(',');
  Serial.print(first[1]); Serial.print(',');
  Serial.print(first[2]); Serial.print("} last={");
  Serial.print(last[0]); Serial.print(',');
  Serial.print(last[1]); Serial.print(',');
  Serial.print(last[2]); Serial.println('}');
}

static bool scanAddress(uint8_t addr) {
  Wire.beginTransmission(addr);
  return Wire.endTransmission() == 0;
}

void setup() {
  Serial.begin(115200);
  delay(600);

  Wire.begin(D2, D1);
  Wire.setClock(100000);
  enableMpuBypass();

  Serial.println();
  Serial.println("AeroPico 0x2C magnetometer brute-force probe");
  Serial.print("MPU 0x68: ");
  Serial.println(scanAddress(MPU_ADDR) ? "ok" : "missing");
  Serial.print("MAG 0x2C: ");
  Serial.println(scanAddress(MAG_ADDR) ? "ok" : "missing");
}

void loop() {
  Serial.println();
  Serial.println("Rotate board slowly during each profile window.");

  for (uint8_t p = 0; p < sizeof(profiles) / sizeof(profiles[0]); ++p) {
    const Profile& profile = profiles[p];
    Serial.println();
    Serial.print("PROFILE ");
    Serial.println(profile.name);
    applyProfile(profile);
    dumpRegs(MAG_ADDR);

    for (uint8_t startReg = 0x00; startReg <= 0x06; ++startReg) {
      evaluateWindow(profile.name, startReg, false);
      evaluateWindow(profile.name, startReg, true);
    }
  }

  Serial.println();
  Serial.println("Done one full pass. Best candidate: ok high, sat near 0, span high, first != last.");
  delay(2000);
}
