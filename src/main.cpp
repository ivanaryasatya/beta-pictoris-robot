#include <Arduino.h>
#include "led_control.h"

void setup() {
  Serial.begin(115200);
  initLED();
}

void loop() {
  ledOn();
  delay(1000);
  ledOff();
  delay(1000);
}
