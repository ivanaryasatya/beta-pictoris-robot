#pragma once
#include <Arduino.h>
#include "pins.h"

static unsigned long previousMillisLedBlink = 0;
static const long ledBlinkInterval = 500;
static bool ledBlinkLedState = LOW;
static bool ledBlinkEnabled = false;
static bool ledBlinkInitialized = false;

void start() {
  if (!ledBlinkInitialized)
  {
    pinMode(builtinLedPin, OUTPUT);
    ledBlinkInitialized = true;
  }
  ledBlinkEnabled = true;
}

void run()
{

  if (!ledBlinkEnabled)
    return;

  static unsigned long currentMillisLedBlink = millis();

  if (currentMillisLedBlink - previousMillisLedBlink >= ledBlinkInterval)
  {
    previousMillisLedBlink = currentMillisLedBlink;

    ledBlinkLedState = !ledBlinkLedState;
    digitalWrite(builtinLedPin, ledBlinkLedState);
  }
}

void stop()
{
  ledBlinkEnabled = false;
  ledBlinkLedState = LOW;
  digitalWrite(builtinLedPin, LOW);
}
