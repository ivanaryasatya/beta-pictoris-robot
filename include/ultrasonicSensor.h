#pragma once

#include <Arduino.h>

class UltrasonicSensor {
  private:
    int trigPin;
    int echoPin;

  public:
    // Constructor default
    UltrasonicSensor() {
      trigPin = -1;
      echoPin = -1;
    }

    // Inisialisasi pin
    void init(int trig, int echo) {
      trigPin = trig;
      echoPin = echo;

      pinMode(trigPin, OUTPUT);
      pinMode(echoPin, INPUT);
      digitalWrite(trigPin, LOW);
    }

    // Baca jarak (cm)
    float readDistanceCM() {
      if (trigPin < 0 || echoPin < 0) return -1; // error jika belum di-init

      // Kirim trigger 10us
      digitalWrite(trigPin, LOW);
      delayMicroseconds(2);
      digitalWrite(trigPin, HIGH);
      delayMicroseconds(10);
      digitalWrite(trigPin, LOW);

      // Baca durasi echo (timeout 30ms)
      long duration = pulseIn(echoPin, HIGH, 30000);
      if (duration == 0) return -1; // tidak ada objek

      // Hitung jarak cm
      float distance = duration * 0.0343 / 2.0;
      return distance;
    }
};




// #include "ultrasonicSensor.h"

// // Buat object sensor
// UltrasonicSensor sensor1;

// void setup() {
//   Serial.begin(115200);

//   // Init sensor1, TRIG=5, ECHO=18
//   sensor1.init(5, 18);
// }

// void loop() {
//   float jarak = sensor1.readDistanceCM();

//   if (jarak > 0) {
//     Serial.print("Jarak: ");
//     Serial.println(jarak);
//   } else {
//     Serial.println("Tidak terdeteksi");
//   }

//   delay(1000);
// }