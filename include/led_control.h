#pragma once
#include <Arduino.h>
#define LED_PIN 2

inline void initLED() {
  pinMode(LED_PIN, OUTPUT);
}

inline void ledOn() {
  digitalWrite(LED_PIN, HIGH);
}

inline void ledOff() {
  digitalWrite(LED_PIN, LOW);
}
