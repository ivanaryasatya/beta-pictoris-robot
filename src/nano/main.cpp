#include <Arduino.h>
#include "uartProtocol.h"
#include "pins.h"
#include "servoController.h"
#include "irSensor.h"
#include "hallSensor.h"
#include "ultrasonicSensor.h"
#include "serialLogger.h"
#include "buzzerRunner.h"
#include "buzzerMelody.h"

UARTProtocol uart(Serial);

Servo catcherServo;
Servo armServo;

HallSensor rHallSensor(pins.hallSensor.right, 500);
HallSensor lHallSensor(pins.hallSensor.left, 500);

TCRT5000Analog irCather(pins.irSensor.catcher, 800);
TCRT5000Analog irDropPoint(pins.irSensor.dropPoint, 800);
TCRT5000Analog irShoot(pins.irSensor.shoot, 800);
TCRT5000Analog irSpeedMotorRight(pins.irSensor.speedMotorRight, 800);
TCRT5000Analog irSpeedMotorLeft(pins.irSensor.speedMotorLeft, 800);

BuzzerMusic buzzer(pins.buzzer.buzzerPin); // Akses variabel angka pin di dalam struct buzzer

UltrasonicSensor ultrasonic;

byte numReturn(const bool value) {
  if (value) return 1;
  return 0;
}

int counter = 1;

void(* resetFunc) (void) = 0;

// --- Variabel Global Kalkulasi RPM ---
unsigned long lastRpmTime = 0;
volatile int rightPulseCount = 0;
volatile int leftPulseCount = 0;
float rightRpm = 0.0;
float leftRpm = 0.0;
bool lastRightState = false;
bool lastLeftState = false;
const int ENCODER_HOLES = 20; // Sesuaikan dengan jumlah lubang/garis pada piringan encoder Anda

struct SensorState {
    unsigned long ultrasonic = 0;

    struct Ir {
        bool catcher = false;
        bool dropPoint = false;
        bool shoot = false;
        float speedMotorRight = 0;
        float speedMotorLeft = 0;
    } ir;
    
    struct Hall {
        bool right = false;
        bool left = false;
        bool barrel = false;
    } hall;;

    struct Power {
        byte battery = 0;
        float voltage = 0;
        float current = 0;
    } power;

} sensorState;

void uartCommand(byte cmd, byte id, byte *data, byte len) {
  if (cmd == uart.mapId.PING) {
    uart.send(uart.mapId.PONG, 0, NULL);
  } else if (cmd == uart.mapId.servo.catcher.ANGLE) {
    servo180(catcherServo, data[0]);
  } else if (cmd == uart.mapId.servo.arm.ANGLE) {
    servo180(armServo, data[0]);
  } else if (cmd == uart.mapId.ULTRASONIC) {
    sensorState.ultrasonic = ultrasonic.readDistanceCM();
    byte ultrasonicBuffer[4];
    memcpy(ultrasonicBuffer, &sensorState.ultrasonic, sizeof(sensorState.ultrasonic));
    uart.send(uart.mapId.ULTRASONIC, 4, ultrasonicBuffer);
  } else if (cmd == uart.mapId.irSensor.CATCHER) {
    byte val = irCather.isDetected() ? 1 : 0;
    uart.send(uart.mapId.irSensor.CATCHER, 1, &val);
  } else if (cmd == uart.mapId.irSensor.DROP_POINT) {
    byte val = irDropPoint.isDetected() ? 1 : 0;
    uart.send(uart.mapId.irSensor.DROP_POINT, 1, &val);
  } else if (cmd == uart.mapId.irSensor.SHOOT) {
    byte val = irShoot.isDetected() ? 1 : 0;
    uart.send(uart.mapId.irSensor.SHOOT, 1, &val);
  } else if (cmd == uart.mapId.irSensor.SPEED_MOTOR_RIGHT) {
    uart.send(uart.mapId.irSensor.SPEED_MOTOR_RIGHT, 4, (byte*)&rightRpm);
  } else if (cmd == uart.mapId.irSensor.SPEED_MOTOR_LEFT) {
    uart.send(uart.mapId.irSensor.SPEED_MOTOR_LEFT, 4, (byte*)&leftRpm);
  }
}

void setup() {
  Serial.begin(9600);
  uart.begin(uartCommand);
  servoAttach(catcherServo, pins.servo.catcher);
  servoAttach(armServo, pins.servo.arm);
  rHallSensor.begin();
  lHallSensor.begin();
  ultrasonic.init(pins.ultrasonicSensor.trig, pins.ultrasonicSensor.echo);

  buzzer.play(melody.startup, 8, 100);
}

void loop() {
  uart.update();
  
  // --- Polling RPM (Paling ringan, tanpa delay/blocking) ---
  unsigned long currentMillis = millis();
  
  bool currentRight = irSpeedMotorRight.isDetected();
  if (currentRight && !lastRightState) rightPulseCount++;
  lastRightState = currentRight;

  bool currentLeft = irSpeedMotorLeft.isDetected();
  if (currentLeft && !lastLeftState) leftPulseCount++;
  lastLeftState = currentLeft;

  // Hitung hasil RPM setiap 100ms
  if (currentMillis - lastRpmTime >= 100) { 
    float multiplier = 60000.0 / (currentMillis - lastRpmTime) / ENCODER_HOLES;
    rightRpm = rightPulseCount * multiplier;
    leftRpm = leftPulseCount * multiplier;
    
    rightPulseCount = 0;
    leftPulseCount = 0;
    lastRpmTime = currentMillis;
  }
  // --------------------------------------------------------

  static bool lastCatcherState = false;
  bool currentCatcherState = irCather.isDetected();
  if (currentCatcherState && !lastCatcherState) {
    uart.send(uart.mapId.irSensor.CATCHER, 0, NULL);
  }
  lastCatcherState = currentCatcherState;
}
