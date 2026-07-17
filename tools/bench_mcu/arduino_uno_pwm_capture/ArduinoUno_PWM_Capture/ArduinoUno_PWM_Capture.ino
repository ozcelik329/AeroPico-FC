/*
  AeroPico PWM Capture - Arduino Uno

  Wiring:
    AeroPico servo output GP16/17/18/19 -> Uno D8
    AeroPico GND                       -> Uno GND

  The Uno input can read Pico 3.3V HIGH reliably in most cases.
  Do not connect Uno 5V outputs into Pico inputs.
*/

#include <Arduino.h>

#ifndef CAPTURE_PIN
#define CAPTURE_PIN 8
#endif

void setup() {
  Serial.begin(115200);
  pinMode(CAPTURE_PIN, INPUT);
  Serial.println();
  Serial.println("AeroPico Arduino Uno PWM capture ready");
  Serial.println("Input: D8, expected servo PWM: 1000-2000 us high, ~20000 us period");
}

void loop() {
  const unsigned long highUs = pulseIn(CAPTURE_PIN, HIGH, 30000UL);
  const unsigned long lowUs = pulseIn(CAPTURE_PIN, LOW, 30000UL);

  if (highUs == 0) {
    Serial.println("no high pulse");
  } else {
    const unsigned long periodUs = highUs + lowUs;
    Serial.print("pulse_us=");
    Serial.print(highUs);
    Serial.print(" period_us=");
    Serial.print(periodUs);
    Serial.print(" freq=");
    if (periodUs > 0) Serial.print(1000000.0f / static_cast<float>(periodUs), 2);
    else Serial.print(0);
    Serial.println("Hz");
  }
  delay(250);
}
