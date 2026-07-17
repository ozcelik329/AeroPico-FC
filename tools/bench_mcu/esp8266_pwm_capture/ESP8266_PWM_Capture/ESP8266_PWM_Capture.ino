/*
  AeroPico PWM Capture - ESP8266 / NodeMCU

  Use as a small logic-analyzer-like servo PWM pulse meter.

  Wiring:
    AeroPico servo output GP16/17/18/19 -> NodeMCU D5 / GPIO14
    AeroPico GND                       -> NodeMCU GND

  Important:
    Only measure 3.3V logic. Do not feed 5V into ESP8266 pins.
*/

#include <Arduino.h>

#ifndef CAPTURE_PIN
#define CAPTURE_PIN 14  // NodeMCU D5
#endif

static volatile uint32_t lastRiseUs = 0;
static volatile uint32_t lastPeriodUs = 0;
static volatile uint32_t lastHighUs = 0;
static volatile uint32_t pulseCount = 0;
static uint32_t lastPrintMs = 0;

void ICACHE_RAM_ATTR onPwmEdge() {
  const uint32_t now = micros();
  if (digitalRead(CAPTURE_PIN)) {
    lastPeriodUs = now - lastRiseUs;
    lastRiseUs = now;
  } else {
    lastHighUs = now - lastRiseUs;
    ++pulseCount;
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);
  pinMode(CAPTURE_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(CAPTURE_PIN), onPwmEdge, CHANGE);
  Serial.println();
  Serial.println("AeroPico ESP8266 PWM capture ready");
  Serial.println("Input: D5 / GPIO14, expected servo PWM: 1000-2000 us high, ~20000 us period");
}

void loop() {
  const uint32_t nowMs = millis();
  if (nowMs - lastPrintMs < 250) return;
  lastPrintMs = nowMs;

  noInterrupts();
  const uint32_t high = lastHighUs;
  const uint32_t period = lastPeriodUs;
  const uint32_t count = pulseCount;
  interrupts();

  if (count == 0) {
    Serial.println("no pulse");
    return;
  }

  const float hz = period > 0 ? 1000000.0f / static_cast<float>(period) : 0.0f;
  Serial.printf("pulse_us=%lu period_us=%lu freq=%.2fHz count=%lu\n",
                static_cast<unsigned long>(high),
                static_cast<unsigned long>(period),
                hz,
                static_cast<unsigned long>(count));
}
