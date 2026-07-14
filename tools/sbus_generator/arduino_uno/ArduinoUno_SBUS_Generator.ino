/*
  AeroPico-FC SBUS Generator - Arduino Uno

  Arduino IDE:
    Board: Arduino Uno
    Serial Monitor: 115200 baud

  Wiring:
    Uno D9       -> transistor inverter input
    inverter out -> AeroPico Pico 2 GP1 / UART0 RX
    Uno GND      -> AeroPico GND

  WARNING:
    Uno GPIO is 5V. Do not connect D9 directly to Pico GP1.

  This sketch bit-bangs 100000 baud 8E2 SBUS frames. ESP32 DevKit v1 is the
  preferred generator when available because it uses hardware UART.
*/

#include <Arduino.h>

#ifndef SBUS_OUT_PIN
#define SBUS_OUT_PIN 9
#endif

#ifndef SBUS_OUTPUT_INVERTED
#define SBUS_OUTPUT_INVERTED 1
#endif

static constexpr uint16_t SBUS_MIN = 172;
static constexpr uint16_t SBUS_MID = 992;
static constexpr uint16_t SBUS_MAX = 1811;
static constexpr uint16_t SBUS_LOW_THROTTLE = 172;
static constexpr uint16_t SBUS_HALF_THROTTLE = 900;
static constexpr uint16_t BIT_US = 10;
static constexpr uint32_t SBUS_PERIOD_US = 14000;

static uint8_t sbusFrame[25];
static uint16_t channels[16];
static uint32_t lastFrameUs = 0;
static uint32_t lastPrintMs = 0;

static inline void sbusWriteLevel(bool logicalHigh) {
  const bool pinHigh = SBUS_OUTPUT_INVERTED ? !logicalHigh : logicalHigh;
  if (pinHigh) PORTB |= _BV(PORTB1);   // D9
  else         PORTB &= ~_BV(PORTB1);  // D9
}

static inline void bitDelay() {
  delayMicroseconds(BIT_US);
}

static uint16_t clampSbus(int value) {
  if (value < (int)SBUS_MIN) return SBUS_MIN;
  if (value > (int)SBUS_MAX) return SBUS_MAX;
  return (uint16_t)value;
}

static void packSbusFrame(bool failsafe) {
  sbusFrame[0] = 0x0F;
  for (uint8_t i = 1; i < 23; ++i) sbusFrame[i] = 0;

  uint16_t bitIndex = 0;
  for (uint8_t ch = 0; ch < 16; ++ch) {
    uint16_t value = channels[ch] & 0x07FF;
    for (uint8_t bit = 0; bit < 11; ++bit) {
      if (value & (1U << bit)) {
        const uint16_t byteIndex = 1 + ((bitIndex + bit) >> 3);
        const uint8_t bitOffset = (bitIndex + bit) & 0x07;
        sbusFrame[byteIndex] |= (uint8_t)(1U << bitOffset);
      }
    }
    bitIndex += 11;
  }

  sbusFrame[23] = failsafe ? 0x08 : 0x00;
  sbusFrame[24] = 0x00;
}

static void writeSbusByte(uint8_t value) {
  uint8_t parity = 0;

  sbusWriteLevel(false); // start bit
  bitDelay();

  for (uint8_t i = 0; i < 8; ++i) {
    const bool bit = (value >> i) & 0x01;
    parity ^= bit ? 1 : 0;
    sbusWriteLevel(bit);
    bitDelay();
  }

  sbusWriteLevel(parity != 0); // even parity bit
  bitDelay();
  sbusWriteLevel(true);        // stop 1
  bitDelay();
  sbusWriteLevel(true);        // stop 2
  bitDelay();
}

static void writeSbusFrame() {
  noInterrupts();
  for (uint8_t i = 0; i < sizeof(sbusFrame); ++i) {
    writeSbusByte(sbusFrame[i]);
  }
  sbusWriteLevel(true);
  interrupts();
}

static void updateChannels(uint32_t nowMs) {
  const uint32_t phase = nowMs % 30000UL;
  const int16_t triangle = (int16_t)((nowMs / 6UL) % 840UL);
  const int16_t sweep = triangle < 420 ? triangle : (840 - triangle);

  for (uint8_t i = 0; i < 16; ++i) channels[i] = SBUS_MID;

  channels[0] = clampSbus((int)SBUS_MID - 210 + sweep);          // roll
  channels[1] = SBUS_MID;                                        // pitch
  channels[2] = phase < 9000UL ? SBUS_LOW_THROTTLE
              : phase < 18000UL ? SBUS_HALF_THROTTLE
                                : SBUS_LOW_THROTTLE;             // throttle
  channels[3] = SBUS_MID;                                        // yaw
  channels[4] = phase < 15000UL ? SBUS_MIN : SBUS_MAX;           // mode
  channels[5] = phase < 10000UL ? SBUS_MIN
              : phase < 20000UL ? SBUS_MID
                                : SBUS_MAX;                      // aux
}

static bool inFailsafeGap(uint32_t nowMs) {
  const uint32_t phase = nowMs % 30000UL;
  return phase >= 27000UL && phase < 28500UL;
}

void setup() {
  Serial.begin(115200);
  pinMode(SBUS_OUT_PIN, OUTPUT);
  sbusWriteLevel(true);

  for (uint8_t i = 0; i < 16; ++i) channels[i] = SBUS_MID;

  Serial.println();
  Serial.println("AeroPico Arduino Uno SBUS generator ready");
  Serial.println("Output: D9, 100000 baud 8E2 bit-bang, 14 ms period");
  Serial.println("Do not connect Uno D9 directly to Pico GP1.");
}

void loop() {
  const uint32_t nowUs = micros();
  if ((uint32_t)(nowUs - lastFrameUs) < SBUS_PERIOD_US) {
    return;
  }
  lastFrameUs += SBUS_PERIOD_US;

  const uint32_t nowMs = millis();
  updateChannels(nowMs);

  if (!inFailsafeGap(nowMs)) {
    packSbusFrame(false);
    writeSbusFrame();
  }

  if (nowMs - lastPrintMs >= 1000UL) {
    lastPrintMs = nowMs;
    Serial.print("CH1=");
    Serial.print(channels[0]);
    Serial.print(" CH2=");
    Serial.print(channels[1]);
    Serial.print(" CH3=");
    Serial.print(channels[2]);
    Serial.print(" CH5=");
    Serial.print(channels[4]);
    Serial.println(inFailsafeGap(nowMs) ? " FAILSAFE_GAP" : " SBUS");
  }
}
