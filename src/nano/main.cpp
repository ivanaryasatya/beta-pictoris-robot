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
#include "DHT.h"

UARTProtocol uart(Serial);

Servo megazineServo;
Servo barrelServo;
DHT dht(pins.temSensor, DHT11);

HallSensor rHallSensor(pins.hallSensor.right, 500);
HallSensor lHallSensor(pins.hallSensor.left, 500);
HallSensor barrelSensor(pins.hallSensor.barrel, 500);

TCRT5000Analog irCather(pins.irSensor.catcher, 800);
TCRT5000Analog irDropPoint(pins.irSensor.dropPoint, 800);
TCRT5000Analog irShoot(pins.irSensor.shoot, 800);
TCRT5000Analog irSpeedMotorRight(pins.irSensor.speedMotorRight, 800);
TCRT5000Analog irSpeedMotorLeft(pins.irSensor.speedMotorLeft, 800);

BuzzerMusic buzzer(pins.buzzer.buzzerPin); // Akses variabel angka pin di dalam struct buzzer

UltrasonicSensor ultrasonic;
unsigned int ultraSonicThreshold = 10; // satuan cm

const byte NUM_RIGHT = 1;
const byte NUM_LEFT = 0;

byte numReturn(const bool value) {
  if (value) return 1;
  return 0;
}
void(* NanoRestart) (void) = 0;
int counter = 1;

// --- Konstanta Kalkulasi RPM ---
const int ENCODER_HOLES = 1;

bool emergencyState = false;

struct SensorState {
  unsigned long ultrasonic = 0;

  struct Tem {
    float temperature = 0;
    float humidity = 0;
  } tem;

  struct Ir {
    bool catcher = false;
    bool dropPoint = false;
    bool shoot = false;
    float speedMotorRight = 0;
    float speedMotorLeft = 0;
    float rightRpm = 0.0;
    float leftRpm = 0.0;
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

void mosfetDriver(const byte pin, const unsigned int speed) {
  static int currentSpeed = 0;
  if (speed > 0) {
    currentSpeed = constrain(speed, 0, 255);
    analogWrite(pin, currentSpeed);
    analogWrite(pin, constrain(speed, 0, 255));
  } else {
    analogWrite(pin, 0);
  }
}

float readMotorRPM(TCRT5000Analog& sensor) {
  unsigned long startRpmTime = millis();
  byte samples = 0;
  unsigned long dt = 0;
  bool lastState = sensor.isDetected();

  while (millis() - startRpmTime < 200) {
    bool currState = sensor.isDetected();
    if (currState && !lastState) {
      samples++;
      if (samples == 5) {
        dt = millis() - startRpmTime;
        break; // Keluar seketika jika sudah 5 sample
      }
    }
    lastState = currState;
  }

  if (samples < 5) dt = millis() - startRpmTime;

  if (samples < 1 || dt == 0) return 0.0;
  return (float)samples * 3000.0 / dt;
}

/**
 * * @param isTemperature Tentukan jenis data: 
 * - true  : Membaca Suhu (Celsius) = Default
 * - false : Membaca Kelembapan (Humidity)
 * * @return Nilai sensor dalam tipe unsigned int. Mengembalikan 0 jika pembacaan gagal (NaN).
 */
float temSensorRead(const bool isTemperature = true) {
  static float data = 0;
  if (isTemperature) {
    data = dht.readTemperature();
  } else {
    data = dht.readHumidity();
  }

  if (isnan(data)) {
    slog.println(F("DHT Failure"));
    return 0;
  } else {
    return data;
  }
}

void uartCommand(byte cmd, byte id, byte *data, byte len) {
  float buffer = 0;

  //ping
  if (cmd == uart.mapId.PING) {
    uart.send(uart.mapId.PONG, 0, NULL);

  //emergency mode
  } else if (cmd == uart.mapId.EMERGENCY_MODE) {
    emergencyState = (data[0] == 1);
    if (emergencyState) {
      // Hentikan total semua penggerak
      mosfetDriver(pins.motor.flywheelRight, 0);
      mosfetDriver(pins.motor.flywheelLeft, 0);
      mosfetDriver(pins.motor.fan, 0);
      servo360Stop(megazineServo);
      servo360Stop(barrelServo);
      digitalWrite(pins.led.laser, LOW);
      digitalWrite(pins.led.bumper.left, LOW);
      digitalWrite(pins.led.bumper.right, LOW);
    }

  // Restart
  } else if (cmd == uart.mapId.RESTART) {
    NanoRestart();

  // Buzzer
  } else if (cmd == uart.mapId.buzzer.TONE) {
    buzzer.playTone(data[0], data[1]);
  } else if (cmd == uart.mapId.buzzer.MELODY) {
    buzzer.play(data[0]);

  // servo
  } else if (cmd == uart.mapId.servo.megazine.ROTATE_R) {
    if (!emergencyState) servo360Right(megazineServo, data[0]);
  } else if (cmd == uart.mapId.servo.megazine.ROTATE_L) {
    if (!emergencyState) servo360Left(megazineServo, data[0]);
  } else if (cmd == uart.mapId.servo.barrel.UP) {
    if (!emergencyState) servo360Right(barrelServo, data[0]);
  } else if (cmd == uart.mapId.servo.barrel.DOWN) {
    if (!emergencyState) servo360Left(barrelServo, data[0]);


  // IR sensor
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
    sensorState.ir.rightRpm = readMotorRPM(irSpeedMotorRight);
    uart.send(uart.mapId.irSensor.SPEED_MOTOR_RIGHT, 4, (byte*)&sensorState.ir.rightRpm);
  } else if (cmd == uart.mapId.irSensor.SPEED_MOTOR_LEFT) {
    sensorState.ir.leftRpm = readMotorRPM(irSpeedMotorLeft);
    uart.send(uart.mapId.irSensor.SPEED_MOTOR_LEFT, 4, (byte*)&sensorState.ir.leftRpm);

  // Hall sensor
  } else if (cmd == uart.mapId.hall.RIGHT) {
    byte val = rHallSensor.isTriggered() ? 1 : 0;
    uart.send(uart.mapId.hall.RIGHT, 1, &val);
  } else if (cmd == uart.mapId.hall.LEFT) {
    byte val = lHallSensor.isTriggered() ? 1 : 0;
    uart.send(uart.mapId.hall.LEFT, 1, &val);

  // ultrasonic
  } else if (cmd == uart.mapId.ultrasonic.DISTANCE){
    sensorState.ultrasonic = ultrasonic.readDistanceCM();
    uart.send(uart.mapId.ultrasonic.DISTANCE, 4, (byte*)&sensorState.ultrasonic);
  } else if (cmd == uart.mapId.ultrasonic.SET_THRESHOLD) {
    ultraSonicThreshold = data[0];

  // LED
  } else if (cmd == uart.mapId.led.LASER) {
    if (!emergencyState) digitalWrite(pins.led.laser, (data[0] == 1) ? HIGH : LOW);
  } else if (cmd == uart.mapId.led.bumper.LEFT) {
    if (!emergencyState) digitalWrite(pins.led.bumper.left, (data[0] == 1) ? HIGH : LOW);
  } else if (cmd == uart.mapId.led.bumper.RIGHT) {
    if (!emergencyState) digitalWrite(pins.led.bumper.right, (data[0] == 1) ? HIGH : LOW);

  // Motor
  } else if (cmd == uart.mapId.motor.right.SPEED) {
    if (!emergencyState) mosfetDriver(pins.motor.flywheelRight, data[0]);
  } else if (cmd == uart.mapId.motor.left.SPEED) {
    if (!emergencyState) mosfetDriver(pins.motor.flywheelLeft, data[0]);
  } else if (cmd == uart.mapId.motor.fan.SPEED) {
    if (!emergencyState) mosfetDriver(pins.motor.fan, data[0]);

  // Temperature sensor
  } else if (cmd == uart.mapId.temSensor.TEMPERATURE) {
    buffer = temSensorRead(true);
    uart.send(uart.mapId.temSensor.TEMPERATURE, 4, (byte*)&buffer);
  } else if (cmd == uart.mapId.temSensor.HUMIDITY) {
    buffer = temSensorRead(false);
    uart.send(uart.mapId.temSensor.HUMIDITY, 4, (byte*)&buffer);
  }
}

void setup() {
  Serial.begin(115200);
  uart.begin(uartCommand);
  servoAttach(megazineServo, pins.servo.catcher);
  servoAttach(barrelServo, pins.servo.arm);
  rHallSensor.begin();
  lHallSensor.begin();
  barrelSensor.begin();
  pinMode(pins.led.laser, OUTPUT);
  pinMode(pins.led.bumper.left, OUTPUT);
  pinMode(pins.led.bumper.right, OUTPUT);
  pinMode(pins.motor.flywheelRight, OUTPUT);
  pinMode(pins.motor.flywheelLeft, OUTPUT);
  pinMode(pins.motor.fan, OUTPUT);
  dht.begin();

  ultrasonic.init(pins.ultrasonicSensor.trig, pins.ultrasonicSensor.echo);
}

void loop() {
  uart.update();
  buzzer.update();


  // Sensors ==============================================
  // IR Catcher
  static bool lastCatcherState = false;
  bool currentCatcherState = irCather.isDetected();
  if (currentCatcherState && !lastCatcherState) {
    uart.send(uart.mapId.irSensor.CATCHER, 1, (byte*)&currentCatcherState);
  }
  lastCatcherState = currentCatcherState;

  // IR Drop point
  static bool lastDropPointState = false;
  bool currentDropPointState = irDropPoint.isDetected();
  if (currentDropPointState && !lastDropPointState) {
    uart.send(uart.mapId.irSensor.DROP_POINT, 1, (byte*)&currentDropPointState);
  }
  lastDropPointState = currentDropPointState;

  // IR Shoot
  static bool lastShootState = false;
  bool currentShootState = irShoot.isDetected();
  if (currentShootState && !lastShootState) {
    uart.send(uart.mapId.irSensor.SHOOT, 1, (byte*)&currentShootState);
  }
  lastShootState = currentShootState;

  // Hall Right 
  static bool lastHallRightState = false;
  bool currentHallRightState = rHallSensor.isTriggered();
  if (currentHallRightState && !lastHallRightState) {
    byte val = currentHallRightState ? 1 : 0;
    uart.send(uart.mapId.hall.RIGHT, 1, &val);
  }
  lastHallRightState = currentHallRightState;

  // Hall Left
  static bool lastHallLeftState = false;
  bool currentHallLeftState = lHallSensor.isTriggered();
  if (currentHallLeftState && !lastHallLeftState) {
    byte val = currentHallLeftState ? 1 : 0;
    uart.send(uart.mapId.hall.LEFT, 1, &val);
  }
  lastHallLeftState = currentHallLeftState;
  
}
