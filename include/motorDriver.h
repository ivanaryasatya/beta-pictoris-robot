#pragma once

#include <Arduino.h>

class MotorDriver {
  private:
    int a_in1, a_in2, a_en;
    int b_in1, b_in2, b_en;
    int pwm_freq;
    int pwm_res;
    int channelA, channelB;

    void driveMotor(int in1, int in2, int en, int channel, int speed) {
      speed = constrain(speed, -255, 255);

      if (speed == 0) {
        digitalWrite(in1, LOW);
        digitalWrite(in2, LOW);
        ledcWrite(channel, 0);
        return;
      }

      if (speed > 0) {
        digitalWrite(in1, HIGH);
        digitalWrite(in2, LOW);
      } else {
        digitalWrite(in1, LOW);
        digitalWrite(in2, HIGH);
        speed = -speed; // ubah jadi positif untuk PWM
      }

      ledcWrite(channel, speed);
    }

  public:
    // Constructor
    MotorDriver(int _a_in1, int _a_in2, int _a_en,
                int _b_in1, int _b_in2, int _b_en,
                int _channelA, int _channelB,
                int _pwm_freq = 20000, int _pwm_res = 8) {
      a_in1 = _a_in1; a_in2 = _a_in2; a_en = _a_en;
      b_in1 = _b_in1; b_in2 = _b_in2; b_en = _b_en;
      channelA = _channelA; channelB = _channelB;
      pwm_freq = _pwm_freq; pwm_res = _pwm_res;
    }

    void begin() {
      pinMode(a_in1, OUTPUT);
      pinMode(a_in2, OUTPUT);
      pinMode(b_in1, OUTPUT);
      pinMode(b_in2, OUTPUT);

      ledcSetup(channelA, pwm_freq, pwm_res);
      ledcSetup(channelB, pwm_freq, pwm_res);

      ledcAttachPin(a_en, channelA);
      ledcAttachPin(b_en, channelB);
    }

    void setMotorA(int speed) {
      driveMotor(a_in1, a_in2, a_en, channelA, speed);
    }

    void setMotorB(int speed) {
      driveMotor(b_in1, b_in2, b_en, channelB, speed);
    }

    void stop() {
      setMotorA(0);
      setMotorB(0);
    }
};




// #include "MotorDriver.h"

// // Driver 1
// MotorDriver driver1(27,26,14, 25,33,32, 0,1);

// // Driver 2 (misal motor lain)
// MotorDriver driver2(12,13,4, 15,2,5, 2,3);

// void setup() {
//   Serial.begin(115200);
//   driver1.begin();
//   driver2.begin();
// }

// void loop() {
//   // Driver 1 maju
//   driver1.setMotorA(200);
//   driver1.setMotorB(200);

//   // Driver 2 mundur
//   driver2.setMotorA(-150);
//   driver2.setMotorB(-150);

//   delay(2000);

//   driver1.stop();
//   driver2.stop();

//   delay(2000);
// }