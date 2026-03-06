#pragma once

#include <Arduino.h>
#include <Servo.h>

class servoController {
  private:
    Servo _servo;
    uint8_t _pin;
    bool _isContinuous;

  public:
    servoController(uint8_t pin, bool continuous = false) {
      _pin = pin;
      _isContinuous = continuous;
    }

    void begin() {
      _servo.attach(_pin);
    }

    void setAngle(int angle) {
      if (!_isContinuous) {
        angle = constrain(angle, 0, 180);
        _servo.write(angle);
      }
    }

    void rotate(int speed) {
      if (_isContinuous) {
        speed = constrain(speed, -100, 100);

        int pulse = map(speed, -100, 100, 1000, 2000);
        _servo.writeMicroseconds(pulse);
      }
    }

    void stop() {
      if (_isContinuous) {
        _servo.writeMicroseconds(1500);
      }
    }

    void detach() {
      _servo.detach();
    }

    bool isContinuous() {
      return _isContinuous;
    }
};




// #include "servoController.h"

// servoController servo180(9, false);   // Pin 9, servo 180°
// servoController servo360(10, true);   // Pin 10, servo 360°

// void setup() {
//   servo180.begin();
//   servo360.begin();
// }

// void loop() {

//   // Servo 180°
//   servo180.setAngle(45);
//   delay(1000);
//   servo180.setAngle(135);
//   delay(1000);

//   // Servo 360°
//   servo360.rotate(60);   // putar kanan 60%
//   delay(2000);
//   servo360.stop();
//   delay(1000);
// }