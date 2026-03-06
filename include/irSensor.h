#pragma once

#include <Arduino.h>

class TCRT5000Analog {
  private:
    int analogPin;        // pin analog sensor
    int thresholdValue;   // nilai ambang untuk deteksi objek

  public:
    // Constructor
    TCRT5000Analog(int pin, int threshold = 500) {
      analogPin = pin;
      thresholdValue = threshold;
      pinMode(analogPin, INPUT);
    }

    // Update threshold jika diperlukan
    void setThreshold(int threshold) {
      thresholdValue = threshold;
    }

    // Baca nilai analog (0-4095 ESP32)
    int readRaw() {
      return analogRead(analogPin);
    }

    // Deteksi apakah ada objek (nilai lebih rendah dari threshold)
    bool isDetected() {
      int val = readRaw();
      return val < thresholdValue; // biasanya objek gelap = rendah
    }
};




// #include "irSensor.h"

// // Sensor 1 di pin 34
// TCRT5000Analog sensor1(34, 800);

// // Sensor 2 di pin 35
// TCRT5000Analog sensor2(35, 800);

// void setup() {
//   Serial.begin(115200);
// }

// void loop() {
//   int raw1 = sensor1.readRaw();
//   bool detect1 = sensor1.isDetected();

//   int raw2 = sensor2.readRaw();
//   bool detect2 = sensor2.isDetected();

//   Serial.print("Sensor1 raw: "); Serial.print(raw1);
//   Serial.print(" Detected? "); Serial.println(detect1);

//   Serial.print("Sensor2 raw: "); Serial.print(raw2);
//   Serial.print(" Detected? "); Serial.println(detect2);

//   delay(500);
// }