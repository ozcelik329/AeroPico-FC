/*
  AeroPico PWM Capture - Raspberry Pi Pico WH

  Arduino IDE:
    Board: Raspberry Pi Pico W / Pico
    Serial Monitor: 115200 baud

  Wiring:
    AeroPico servo output GP16/17/18/19 -> Pico WH GP15
    AeroPico GND                       -> Pico WH GND
*/

#include <Arduino.h>

#ifndef CAPTURE_PIN
#define CAPTURE_PIN 15
#endif

static volatile uint32_t lastRiseUs = 0;
static volatile uint32_t highUs = 0;
static volatile uint32_t periodUs = 0;
static volatile uint32_t pulseCount = 0;
static uint32_t lastPrintMs = 0;

static void onPwmEdge() {
  const uint32_t now = micros();
  if (digitalRead(CAPTURE_PIN)) {
    periodUs = now - lastRiseUs;
    lastRiseUs = now;
  } else {
    highUs = now - lastRiseUs;
    ++pulseCount;
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(CAPTURE_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(CAPTURE_PIN), onPwmEdge, CHANGE);
  Serial.println();
  Serial.println("AeroPico Pico WH PWM capture ready");
  Serial.println("Input: GP15, expected servo PWM: 1000-2000 us high, ~20000 us period");
}

void loop() {
  const uint32_t nowMs = millis();
  if (nowMs - lastPrintMs < 250) return;
  lastPrintMs = nowMs;

  noInterrupts();
  const uint32_t high = highUs;
  const uint32_t period = periodUs;
  const uint32_t count = pulseCount;
  interrupts();

  if (count == 0) {
    Serial.println("no pulse");
    return;
  }

  Serial.print("pulse_us=");
  Serial.print(high);
  Serial.print(" period_us=");
  Serial.print(period);
  Serial.print(" freq=");
  Serial.print(period > 0 ? 1000000.0f / static_cast<float>(period) : 0.0f, 2);
  Serial.print("Hz count=");
  Serial.println(count);
}
