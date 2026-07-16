/*
  AeroPico-FC SBUS Generator - ESP32 DevKit v1

  Arduino IDE:
    Board: ESP32 Dev Module
    Serial Monitor: 115200 baud

  Default wiring, with the existing transistor inverter:
    ESP32 GPIO17  -> inverter input
    inverter out  -> AeroPico Pico 2 GP1 / UART0 RX
    ESP32 GND     -> AeroPico GND

  If connecting ESP32 GPIO17 directly to Pico GP1, set SBUS_TX_INVERTED to 0.
*/

#include <Arduino.h>
#include <math.h>

#ifndef SBUS_TX_PIN
#define SBUS_TX_PIN 17
#endif

#ifndef SBUS_TX_INVERTED
#define SBUS_TX_INVERTED 1
#endif

static constexpr uint32_t SBUS_BAUD = 100000;
static constexpr uint32_t SBUS_PERIOD_US = 14000;
static constexpr uint16_t SBUS_MIN = 172;
static constexpr uint16_t SBUS_MID = 992;
static constexpr uint16_t SBUS_MAX = 1811;
static constexpr uint16_t SBUS_LOW_THROTTLE = 172;
static constexpr uint16_t SBUS_HALF_THROTTLE = 900;

static HardwareSerial SbusSerial(2);
static uint8_t sbusFrame[25];
static uint16_t channels[16];
static uint32_t lastFrameUs = 0;
static uint32_t lastPrintMs = 0;

static uint16_t clampSbus(int value) {
  if (value < static_cast<int>(SBUS_MIN)) return SBUS_MIN;
  if (value > static_cast<int>(SBUS_MAX)) return SBUS_MAX;
  return static_cast<uint16_t>(value);
}

static void packSbusFrame(bool failsafe) {
  sbusFrame[0] = 0x0F;
  for (uint8_t i = 1; i < 23; ++i) sbusFrame[i] = 0;

  uint16_t bitIndex = 0;
  for (uint8_t ch = 0; ch < 16; ++ch) {
    const uint16_t value = channels[ch] & 0x07FF;
    for (uint8_t bit = 0; bit < 11; ++bit) {
      if (value & (1U << bit)) {
        const uint16_t byteIndex = 1 + ((bitIndex + bit) >> 3);
        const uint8_t bitOffset = (bitIndex + bit) & 0x07;
        sbusFrame[byteIndex] |= static_cast<uint8_t>(1U << bitOffset);
      }
    }
    bitIndex += 11;
  }

  sbusFrame[23] = failsafe ? 0x08 : 0x00;
  sbusFrame[24] = 0x00;
}

static void updateChannels(uint32_t nowMs) {
  const uint32_t phase = nowMs % 30000UL;
  const float sweep = sinf(static_cast<float>(nowMs % 6000UL) * (2.0f * PI / 6000.0f));

  for (uint8_t i = 0; i < 16; ++i) channels[i] = SBUS_MID;

  channels[0] = clampSbus(static_cast<int>(SBUS_MID) + static_cast<int>(sweep * 420.0f)); // roll
  channels[1] = SBUS_MID;                                                                // pitch
  channels[2] = phase < 9000UL ? SBUS_LOW_THROTTLE
              : phase < 18000UL ? SBUS_HALF_THROTTLE
                                : SBUS_LOW_THROTTLE;                                     // throttle
  channels[3] = SBUS_MID;                                                                // yaw
  channels[4] = phase < 15000UL ? SBUS_MIN : SBUS_MAX;                                   // mode
  channels[5] = phase < 10000UL ? SBUS_MIN
              : phase < 20000UL ? SBUS_MID
                                : SBUS_MAX;                                              // aux
}

static bool inFailsafeGap(uint32_t nowMs) {
  const uint32_t phase = nowMs % 30000UL;
  return phase >= 27000UL && phase < 28500UL;
}

void setup() {
  Serial.begin(115200);
  delay(200);

  for (uint8_t i = 0; i < 16; ++i) channels[i] = SBUS_MID;

  SbusSerial.begin(SBUS_BAUD, SERIAL_8E2, -1, SBUS_TX_PIN, SBUS_TX_INVERTED);

  Serial.println();
  Serial.println("AeroPico ESP32 SBUS generator ready");
  Serial.printf("TX pin: GPIO%d, inverted: %s\n", SBUS_TX_PIN, SBUS_TX_INVERTED ? "yes" : "no");
  Serial.println("Frame: 100000 baud, 8E2, 25 bytes, 14 ms period");
}

void loop() {
  const uint32_t nowUs = micros();
  if (static_cast<uint32_t>(nowUs - lastFrameUs) < SBUS_PERIOD_US) {
    return;
  }
  lastFrameUs += SBUS_PERIOD_US;

  const uint32_t nowMs = millis();
  updateChannels(nowMs);

  if (!inFailsafeGap(nowMs)) {
    packSbusFrame(false);
    SbusSerial.write(sbusFrame, sizeof(sbusFrame));
  }

  if (nowMs - lastPrintMs >= 1000UL) {
    lastPrintMs = nowMs;
    Serial.printf("CH1=%u CH2=%u CH3=%u CH4=%u CH5=%u %s\n",
                  channels[0], channels[1], channels[2], channels[3], channels[4],
                  inFailsafeGap(nowMs) ? "FAILSAFE_GAP" : "SBUS");
  }
}
