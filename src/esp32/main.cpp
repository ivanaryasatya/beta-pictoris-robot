
#include <Arduino.h> 
#include "eepromManager.h"
#include "secretData.h"
#include "protocolMap.h"
#include "taskManager.h"
#include "kalmanFilter.h"
#include <esp_task_wdt.h>
#include <eepromPointer.h>
#include "servoController.h"
#include "commandParser.h"
#include "uartProtocol.h"
#include "pins.h"
#include "serialLogger.h" 
#include "eepromPointer.h"
#include "logMessage.h"
#include "motorDriver.h"
#include "wheelDrive.h"
#include "gamepadHandler.h"
#include <pgmspace.h>
#include "irSensor.h"
#include "buzzerMelody.h"
#include "sequence.h"

#define ON_STR "ON"
#define OFF_STR "OFF"
#define TRUE_INT 1
#define FALSE_INT 0

using byte = uint8_t;

using cbyte = const byte;
using cuint = const uint;

// Struct wrapper untuk array String[10]
struct StringArray10 {
    String data[10];
};

byte RIGHT = 1;
byte LEFT = 0;

//FirebaseHandler firebase;
EEPROMManager memory;
MutexData<byte> mutexCmdValueCount(0);

uint count = 0;
cbyte esp32 = 0;
cbyte nano = 1;
byte gmpDeadZone = 2;
int test = 0;
bool serialLogState = true;
bool gamepadIsConnected = false;
uint nanoPing = 0;
bool isNanoConnected = false;
MutexData<unsigned long> loopCountCore0(0);


// Command parsing
String target, command, value[16];
int cmdMaxValue, cmdValueCount;

String serialInput;
UARTProtocol uart(Serial1);

MutexData<GamepadState> globalGamepadState;
GamepadState last_state;

unsigned long last_rumble_burst = 0;
bool is_rumble_burst_active = false;
const unsigned long RUMBLE_BURST_DURATION = 200;
const char* spaceStr = " ";
int espResetTimer = -1;
unsigned long lastPingMicros = 0;
unsigned long lastPongReceivedMicros = 0;
const unsigned long pingInterval = 500000; // us

MotorDriver motor(pins.wheelDriver_r.ENA, pins.wheelDriver_r.IN1, pins.wheelDriver_r.IN2, pins.wheelDriver_r.IN3, pins.wheelDriver_r.IN4, pins.wheelDriver_r.ENB);
MotorDriver motor2(pins.wheelDriver_l.ENA, pins.wheelDriver_l.IN1, pins.wheelDriver_l.IN2, pins.wheelDriver_l.IN3, pins.wheelDriver_l.IN4, pins.wheelDriver_l.ENB);

MecanumDrive mecanum(&motor2, &motor);
Servo catcherServo;
Servo pusherServo;
Servo armServo;


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

// Fungsi filter untuk menghindari spam output ke serial monitor
bool isGamepadChanged(const GamepadState& current, const GamepadState& last) {
    if (abs(current.stick_lx - last.stick_lx) > gmpDeadZone) return true;
    if (abs(current.stick_ly - last.stick_ly) > gmpDeadZone) return true;
    if (abs(current.stick_rx - last.stick_rx) > gmpDeadZone) return true;
    if (abs(current.stick_ry - last.stick_ry) > gmpDeadZone) return true;

    if (abs(current.analog_l2 - last.analog_l2) > gmpDeadZone) return true;
    if (abs(current.analog_r2 - last.analog_r2) > gmpDeadZone) return true;

    if (current.cross != last.cross) return true;
    if (current.square != last.square) return true;
    if (current.triangle != last.triangle) return true;
    if (current.circle != last.circle) return true;
    
    if (current.up != last.up) return true;
    if (current.down != last.down) return true;
    if (current.left != last.left) return true;
    if (current.right != last.right) return true;
    
    if (current.l1 != last.l1) return true;
    if (current.r1 != last.r1) return true;
    if (current.l3 != last.l3) return true;
    if (current.r3 != last.r3) return true;
    
    if (current.start != last.start) return true;
    if (current.select != last.select) return true;
    if (current.ps != last.ps) return true;

    return false;
}



/*
true = light sleep
false = deep sleep
time in second
*/
void espSleep(const bool isLightmode, cuint time) {
    esp_sleep_enable_timer_wakeup(time * 1000000ULL);
    if (isLightmode == 0) {
        esp_light_sleep_start();
    } else {
        esp_deep_sleep_start();
    }
}

bool checkValue(const byte valueCount, const byte minCount) {
    if (valueCount < minCount) {
        slog.add(logMes.commandValueLessThanExpected);
        slog.add(String(cmdValueCount));
        slog.println();
        return false;
    }
    return true;
}

class RobotAction {
    public:
    enum MagAlignTarget {
        ALIGN_SHOOT,
        ALIGN_DROP_POINT
    };

    bool magazineSlots[11] = {false, false, false, false, false, false, false, false, false, false, false};
    int currentSlotAtShoot = 0; // 0 to 10
    
    private:
    bool doTakeBall = false;
    bool doShoot = false;
    
    // State variables untuk Magazine Rotation
    bool doRotateMag = false;
    bool magRotateDirection = LEFT; 
    MagAlignTarget magAlignTargetMode = ALIGN_SHOOT;
    byte magTargetCount = 1;
    byte currentMagRotationCount = 0;
    bool lastHallRight = false;
    bool lastHallLeft = false;
    
    // State variables untuk TakeBall
    byte takeBallStep = 0;
    unsigned long lastTakeBallTime = 0;
    uint takeBallDelay = 0;

    // State variables untuk Shoot
    byte shootStep = 0;
    unsigned long lastShootTime = 0;
    uint shootDelay = 0;

    public:
        byte upArmAngle = 120;
        byte downArmAngle = 0;
        byte openCatcherAngle = 20;
        byte closeCatherAngle = 0;
        byte pusherPushAngle = 180;
        byte pusherIdleAngle = 0;

        bool emergencyState = false;
        bool isEmergencyFeatureEnabled = true;
        bool ledBumperState = false;
        byte flywheelSpeed = 0;

        void update() {
            // Jalankan logika TakeBall secara independen
            if (doTakeBall && (millis() - lastTakeBallTime >= takeBallDelay)) {
                lastTakeBallTime = millis();
                switch (takeBallStep) {
                    case 0: 
                        servo180(catcherServo, closeCatherAngle); 
                        takeBallDelay = 500; 
                        takeBallStep++; 
                        break;
                    case 1: 
                        servo180(armServo, upArmAngle); 
                        takeBallDelay = 500; 
                        takeBallStep++; 
                        break;
                    case 2: 
                        servo180(catcherServo, openCatcherAngle); 
                        takeBallDelay = 500; 
                        takeBallStep++; 
                        break;
                    case 3: 
                        servo180(armServo, downArmAngle); 
                        doTakeBall = false; 
                        takeBallStep = 0; 
                        break;
                }
            }

            // Jalankan logika Shoot secara independen
            if (doShoot && (millis() - lastShootTime >= shootDelay)) {
                lastShootTime = millis();
                switch (shootStep) {
                    case 0: 
                        servo180(pusherServo, pusherPushAngle); 
                        shootDelay = 500; 
                        shootStep++; 
                        break;
                    case 1: 
                        servo180(pusherServo, pusherIdleAngle); 
                        doShoot = false; 
                        shootStep = 0; 
                        break;
                }
            }
            //solve
            if (doRotateMag) {
                bool currentHallRight = sensorState.hall.right;
                bool currentHallLeft = sensorState.hall.left;

                // Cek Trigger Shoot Aligned
                if (currentHallRight && !lastHallRight) {
                    if (magRotateDirection == LEFT) {
                        currentSlotAtShoot = (currentSlotAtShoot + 1) % 11;
                    } else {
                        currentSlotAtShoot = (currentSlotAtShoot == 0) ? 10 : currentSlotAtShoot - 1;
                    }
                    
                    // Baca status IR saat terisi/tidak
                    magazineSlots[currentSlotAtShoot] = sensorState.ir.shoot;
                    
                    if (magAlignTargetMode == ALIGN_SHOOT) {
                        currentMagRotationCount++;
                        if (currentMagRotationCount >= magTargetCount) {
                            uart.send(uart.mapId.servo.megazine.STOP, 0, NULL);
                            doRotateMag = false;
                        }
                    }
                }

                // Cek Trigger DropPoint Aligned
                if (currentHallLeft && !lastHallLeft) {
                    int dropPointIndex = (currentSlotAtShoot + (magRotateDirection == LEFT ? 5 : 6)) % 11;
                    
                    // Baca status IR drop point
                    magazineSlots[dropPointIndex] = sensorState.ir.dropPoint;

                    if (magAlignTargetMode == ALIGN_DROP_POINT) {
                        currentMagRotationCount++;
                        if (currentMagRotationCount >= magTargetCount) {
                            uart.send(uart.mapId.servo.megazine.STOP, 0, NULL);
                            doRotateMag = false;
                        }
                    }
                }

                lastHallRight = currentHallRight;
                lastHallLeft = currentHallLeft;
            }
        }

        void takeBall() { 
            if (!doTakeBall) { 
                doTakeBall = true; 
                takeBallStep = 0; 
                takeBallDelay = 0; 
            } 
        }

        void rotateMegazine(bool isRotateLeft = LEFT, MagAlignTarget targetAlign = ALIGN_SHOOT, byte targetCount = 1) {
            if (!doRotateMag && targetCount > 0) {
                doRotateMag = true;
                magRotateDirection = isRotateLeft;
                magAlignTargetMode = targetAlign;
                magTargetCount = targetCount;
                currentMagRotationCount = 0;
                lastHallRight = sensorState.hall.right;
                lastHallLeft = sensorState.hall.left;
                
                byte speed = 40;
                if (magRotateDirection == LEFT) {
                    uart.send(uart.mapId.servo.megazine.ROTATE_L, 1, &speed);
                } else {
                    uart.send(uart.mapId.servo.megazine.ROTATE_R, 1, &speed);
                }
            }
        }

        void shoot() { 
            if (!doShoot) { 
                doShoot = true; 
                shootStep = 0; 
                shootDelay = 0; 
            } 
        }

        void setFlywheelSpeed(bool rightSide, byte speed) {
            if (rightSide) {
                uart.send(uart.mapId.motor.right.SPEED, 1, &speed);
            } else {
                uart.send(uart.mapId.motor.left.SPEED, 1, &speed);
            }
        }

        void setFanSpeed(byte speed) {
            uart.send(uart.mapId.motor.fan.SPEED, 1, &speed);
        }

        void laser(bool state) {
            byte val = state ? 1 : 0;
            uart.send(uart.mapId.led.LASER, 1, &val);
        }

        void barrel(bool moveUp, byte speed) {
            if (moveUp) {
                uart.send(uart.mapId.servo.barrel.UP, 1, &speed);
            } else {
                uart.send(uart.mapId.servo.barrel.DOWN, 1, &speed);
            }
        }

        void setDefault() {
            uart.send(uart.mapId.SET_DEFAULT, 0, NULL);
            servo180(pusherServo, 0);
        }

        void setEmergencyFeature(bool enable) {
            isEmergencyFeatureEnabled = enable;
            if (!enable && emergencyState) {
                emergencyState = false;
                byte val = 0;
                uart.send(uart.mapId.EMERGENCY_MODE, 1, &val);
            }
        }

        void toggleEmergency() {
            if (!isEmergencyFeatureEnabled) {
                slog.println(F("Emergency feature is disabled!"));
                return;
            }
            emergencyState = !emergencyState;
            byte val = emergencyState ? 1 : 0;
            uart.send(uart.mapId.EMERGENCY_MODE, 1, &val);
            if (emergencyState) {
                // Hentikan semua proses beruntun dan pergerakan seketika
                doTakeBall = false;
                doShoot = false;
                doRotateMag = false;
                flywheelSpeed = 0;
                setFlywheelSpeed(true, 0);
                setFlywheelSpeed(false, 0);
                setFanSpeed(0);
                uart.send(uart.mapId.servo.megazine.STOP, 0, NULL);
                mecanum.stop();
            }
        }

        void toggleLedBumper() {
            ledBumperState = !ledBumperState;
            byte val = ledBumperState ? 1 : 0;
            uart.send(uart.mapId.led.bumper.LEFT, 1, &val);
            uart.send(uart.mapId.led.bumper.RIGHT, 1, &val);
        }

        void changeFlywheelSpeed(bool increase) {
            if (increase) {
                if (flywheelSpeed <= 230) flywheelSpeed += 25;
                else flywheelSpeed = 255;
            } else {
                if (flywheelSpeed >= 25) flywheelSpeed -= 25;
                else flywheelSpeed = 0;
            }
            setFlywheelSpeed(true, flywheelSpeed);
            setFlywheelSpeed(false, flywheelSpeed);
        }

} robotAction;

// ==================== AUTO SEQUENCE CONFIG ====================
int selectedSequenceIndex = -1; // -1 Berarti tidak ada sequence yang dipilih
bool isSequenceRunning = false;
byte currentSequenceStep = 0;
unsigned long sequenceStepStartTime = 0;

// ==============================================================

// Fungsi cek boolean dari string
//
// str:
// true: "1", "true", "on", "enable", "yes"
// false: "0", "false", "off", "disable", "no"
// number:
// true: >= 1
// false: <= 0
struct State {
    bool str(const String &s) {
        const char* c = s.c_str();

        if (strcasecmp(c, "1") == 0 || strcasecmp(c, "true") == 0 || 
            strcasecmp(c, ON_STR) == 0 || strcasecmp(c, "enable") == 0 || 
            strcasecmp(c, "yes") == 0) {
            return true;
        }

        if (strcasecmp(c, "0") == 0 || strcasecmp(c, "false") == 0 || 
            strcasecmp(c, "off") == 0 || strcasecmp(c, "disable") == 0 || 
            strcasecmp(c, "no") == 0) {
            return false;
        } 

        slog.println(logMes.invalidCommand);
        return false;
    }

    bool num(const int number) {
        if (number >= 1) return true;
        if (number == 0) return false;
        return false;
    }

    byte numReturn(const bool value) {
        if (value) return 1;
        return 0;
    }
} state;

void uartCommandRun(byte uartCmd, byte id, byte *data, byte len) {
    // Ping
    if (uartCmd == uart.mapId.PING) {
        uart.send(uart.mapId.PONG, 0, NULL);
    } else if (uartCmd == uart.mapId.PONG) {
        lastPongReceivedMicros = micros();
        isNanoConnected = true;
        nanoPing = lastPongReceivedMicros - lastPingMicros;

    // Restart
    } else if (uartCmd == uart.mapId.RESTART) {
        espResetTimer = data[0] * 1000;
        
    // Cmd
    } else if (uartCmd == uart.mapId.USER_CMD) {
        
    // Hall Sensors
    } else if (uartCmd == uart.mapId.hall.RIGHT) {
        sensorState.hall.right = state.num(data[0]);
    } else if (uartCmd == uart.mapId.hall.LEFT) {
        sensorState.hall.left = state.num(data[0]);

    // IR Sensors
    } else if (uartCmd == uart.mapId.irSensor.CATCHER) {
        sensorState.ir.catcher = state.num(data[0]);
    } else if (uartCmd == uart.mapId.irSensor.DROP_POINT) {
        sensorState.ir.dropPoint = state.num(data[0]);
    } else if (uartCmd == uart.mapId.irSensor.SHOOT) {
        sensorState.ir.shoot = state.num(data[0]);
    } else if (uartCmd == uart.mapId.irSensor.SPEED_MOTOR_RIGHT) {
        sensorState.ir.speedMotorRight = *(float*)&data[0];
    } else if (uartCmd == uart.mapId.irSensor.SPEED_MOTOR_LEFT) {
        sensorState.ir.speedMotorLeft = *(float*)&data[0];

    // Ultrasonic sensor
    } else if (uartCmd == uart.mapId.ultrasonic.DISTANCE) {
        sensorState.ultrasonic = *(unsigned long*)&data[0];
    } else {
        slog.println(logMes.invalidCommand);
    }
}

bool commandRun(const String &target, const String &command, const String value[], const byte valueCount) {

    // format: target.command.value1.value2;
    // contoh: esp32.memSave.q.MyWiFi; atau nano.led.on;

    if (target == F("rbt")) {
        if (command == F("restart")) {
            uart.send(uart.mapId.RESTART, 0, NULL);
            delay(100);
            ESP.restart();
            return true;
        } else if (command == F("move")) {
            if (checkValue(valueCount, 4)) {
                return false;
            }
            mecanum.drive(value[0].toInt(), value[1].toInt(), value[2].toInt(), value[3].toInt());
        } else if (command == F("enableEmergency")) {
            bool stateVal = state.str(value[0]);
            robotAction.setEmergencyFeature(stateVal);
            if (!memory.save(epmPtr.emergencyState, stateVal ? "1" : "0")) {
                slog.println(logMes.eepromSaveFailed);
            }
            slog.add(F("Emergency Feature: "));
            slog.add(stateVal ? ON_STR : OFF_STR);
            slog.println();
        } else if (command == F("emergency")) {
            bool stateVal = state.str(value[0]);
            if (robotAction.emergencyState != stateVal) {
                robotAction.toggleEmergency();
                slog.add(F("Emergency mode: "));
                slog.add(stateVal ? ON_STR : OFF_STR);
                slog.println();
            }
        } else if (command == F("isEmergency")) {
            slog.add(F("Emergency mode: "));
            slog.println(robotAction.emergencyState ? ON_STR : OFF_STR);
        } else {
            slog.println(logMes.invalidCommand);
            return false;
        }
    }

    else if (target == F("esp32")) {
        if (command == F("ping")) {
            uart.send(uart.mapId.PING, 0, NULL);
        } else if (command == F("memSave")) {
            memory.save(value[0][0], value[1]);
        } else if (command == F("memGetAll")) {
            slog.println(memory.getAll());
        } else if (command == F("freeHeap")) {
            slog.add(F("Free heap: "));
            slog.add(String(ESP.getFreeHeap()));
            slog.println();
        } else if (command == F("minFreeHeap")) {
            slog.add("Minimum free heap: ");
            slog.add(String(ESP.getMinFreeHeap()));
            slog.println();
        } else if (command == F("enableLog")) {
            serialLogState = state.str(value[0]);
            slog.enable(serialLogState);
            if (!memory.save(epmPtr.logState, serialLogState ? "1" : "0")) {
                slog.println(logMes.eepromSaveFailed);
            }
            slog.add(logMes.logStatus);
            slog.add(serialLogState ? logMes.ON : logMes.OFF);
            slog.println();
        } else if (command == F("restart")) {
            slog.println(logMes.esp32Restarting);
            ESP.restart();
        } else if (command == F("maxAllocHeap")) {
            slog.add(F("Maximum alloc heap: "));
            slog.add(String(ESP.getMaxAllocHeap()));
            slog.println();
        } else if (command == F("sleep")) {
            if (value[0] == F("light")) {
                espSleep(true, value[1].toInt());
            } else if (value[0] == F("deep")) {
                espSleep(false, value[1].toInt());
            }
        } else if (command == F("bzrTone")) {
            unsigned int buzzerValue = value[0].toInt();
            uart.send(uart.mapId.buzzer.TONE, sizeof(buzzerValue), (byte*)&buzzerValue);
        } else if (command == F("bzrMelody")) {
            byte melodyValue = (byte)value[0].toInt();
            uart.send(uart.mapId.buzzer.MELODY, 1, &melodyValue);
        } else if (command == F("power")) {
            slog.add(F("Battery: "));
            slog.add(String(sensorState.power.battery));
            slog.add(F("% | Voltage: "));
            slog.add(String(sensorState.power.voltage, 2));
            slog.add(F("V | Current: "));
            slog.add(String(sensorState.power.current, 2));
            slog.add(F("A"));
            slog.println();
        } else {
            slog.println(logMes.invalidCommand);
            return false;
        }
    } else if (target == F("nano")) {
        if (command == F("sendCommand") && valueCount >= 1) {
            uart.send(uart.mapId.USER_CMD, 0, (byte*)value[0].c_str());
        } else if (command == F("ping")) {
            slog.add(F("NANO ping: "));
            slog.add(String(nanoPing));
            slog.add(F(" us"));
            slog.println();
        } else if (command == F("restart")) {
            slog.println(logMes.esp32Restarting);
            ESP.restart();
        }
    } else if (target == F("gmp")) {
        if (command == F("batt")) {
            slog.add(F("gamepad battery: "));
            slog.add(globalGamepadState.get().battery_status);
            slog.println();
        }
    } else {
        slog.add(logMes.invalidCommandTarget);
        slog.add(spaceStr);
        slog.add(target);
        slog.println();
        return false;
    }
    return false;
}










/*
=======================================
=============== Core 1 ================
=======================================
*/
void mainFunction(void *pvParameters) {
    static bool mainFunctionHasRunOnce = false;
    if (!mainFunctionHasRunOnce) {
        slog.println("ESP32 is starting up!");

        motor.begin();
        motor2.begin();
        mecanum.stop();
        servoAttach(catcherServo, pins.servo.catcher);
        servoAttach(armServo, pins.servo.arm);
        servoAttach(pusherServo, pins.servo.pusher);

        Serial.begin(115200);
        Serial1.begin(115200, SERIAL_8N1, pins.esp32Uart.rx, pins.esp32Uart.tx);
        uart.begin(uartCommandRun);
        
        memory.begin(512);
        serialLogState = memory.get(epmPtr.logState) == "1";
        slog.enable(serialLogState);

        String emgFeatureState = memory.get(epmPtr.emergencyState);
        if (emgFeatureState == "0") {
            robotAction.setEmergencyFeature(false);
        } else {
            robotAction.setEmergencyFeature(true);
        }

        uart.send(uart.mapId.buzzer.MELODY, 1, (byte*)&buzzerMel.STARTUP);

        robotAction.setDefault();

        // Catatan: Pin 34 ESP32 BUKAN pull-up internal. Pastikan menggunakan Pull-Up Resistor eksternal di hardware
        pinMode(pins.EMERGENCY_BTN, INPUT); 

        mainFunctionHasRunOnce = true;
    }





    while (true) {
        uart.update();
        mecanum.update();
        robotAction.update();

        // Ping Nano setiap 500ms
        if (micros() - lastPingMicros >= pingInterval) { // 500000 mikrodetik = 500 ms
            lastPingMicros = micros();
            uart.send(uart.mapId.PING, 0, NULL);
        }

        // Cek timeout komunikasi dengan Nano (2 detik)
        if (micros() - lastPongReceivedMicros > 2000000) {
            if (isNanoConnected) {
                isNanoConnected = false;
                slog.println(F("Nano is disconnected!"));
            }
        }

        // Cek tombol fisik Emergency
        static bool lastEmergencyBtn = HIGH;
        bool currentEmergencyBtn = digitalRead(pins.EMERGENCY_BTN);
        if (currentEmergencyBtn == LOW && lastEmergencyBtn == HIGH) {
            slog.println(F("[BTN] Toggle Emergency Mode"));
            robotAction.toggleEmergency();
        }
        lastEmergencyBtn = currentEmergencyBtn;

        
        //Gamepad=========================================
        if (Ps3.isConnected()) {
            if (!gamepadIsConnected) {
                gamepadIsConnected = true;
                slog.println(F("Gamepad connected!"));
                uart.send(uart.mapId.buzzer.MELODY, 1, (byte*)&buzzerMel.CTRL_CONNECTED);
            }
            GamepadState current_state = globalGamepadState.get();
            unsigned long current_millis = millis();

            if (is_rumble_burst_active && (current_millis - last_rumble_burst >= RUMBLE_BURST_DURATION)) {
                is_rumble_burst_active = false;
                if (current_state.analog_r2 > 128) {
                    Ps3.setRumble(current_state.analog_r2, current_state.analog_r2);
                } else if (current_state.r1) {
                    Ps3.setRumble(50, 0);
                } else {
                    Ps3.setRumble(0, 0);
                }
            }

            //Gamepad action=============================
            if (isGamepadChanged(current_state, last_state)) {

                // Cek pembatalan sequence jika tombol/analog lain ditekan
                bool otherInputActive = false;
                if (abs(current_state.stick_lx - last_state.stick_lx) > gmpDeadZone) otherInputActive = true;
                if (abs(current_state.stick_ly - last_state.stick_ly) > gmpDeadZone) otherInputActive = true;
                if (abs(current_state.stick_rx - last_state.stick_rx) > gmpDeadZone) otherInputActive = true;
                if (abs(current_state.stick_ry - last_state.stick_ry) > gmpDeadZone) otherInputActive = true;
                if (current_state.cross && !last_state.cross) otherInputActive = true;
                if (current_state.square && !last_state.square) otherInputActive = true;
                if (current_state.triangle && !last_state.triangle) otherInputActive = true;
                if (current_state.circle && !last_state.circle) otherInputActive = true;
                if (current_state.up && !last_state.up) otherInputActive = true;
                if (current_state.down && !last_state.down) otherInputActive = true;
                if (current_state.left && !last_state.left) otherInputActive = true;
                if (current_state.right && !last_state.right) otherInputActive = true;
                if (current_state.l1 && !last_state.l1) otherInputActive = true;
                if (current_state.r1 && !last_state.r1) otherInputActive = true;
                if (current_state.ps && !last_state.ps) otherInputActive = true;

                if (otherInputActive && selectedSequenceIndex != -1 && !isSequenceRunning) {
                    slog.println(F("[GMP] Sequence selection CANCELLED"));
                    selectedSequenceIndex = -1;
                }

                // PS Button = Toggle Emergency Mode (Dipindah dari Cross)
                if (current_state.ps && !last_state.ps) {
                    slog.println(F("[GMP] PS: Toggle Emergency Mode"));
                    robotAction.toggleEmergency();
                }

                // Blokir kontrol mekanik jika masuk mode darurat
                if (!robotAction.emergencyState) {
                    // barrel elevation
                    if (current_state.stick_ly < -20) { // Joystick ke Atas
                        byte speed = map(current_state.stick_ly, -20, -128, 0, 90);
                        robotAction.barrel(true, speed);
                    } else if (current_state.stick_ly > 20) { // Joystick ke Bawah
                        byte speed = map(current_state.stick_ly, 20, 127, 0, 90);
                        robotAction.barrel(false, speed);
                    } else if (abs(last_state.stick_ly) > 20) {
                        robotAction.barrel(true, 0); // Kirim speed 0 untuk stop
                    }
                    
                    // Button 0: X (Cross) = Shoot
                    if (current_state.cross && !last_state.cross) {
                        slog.println(F("[GMP] Cross: Shoot"));
                        robotAction.shoot();
                    }

                    // Button 1: A (Square) = Buzzer (Klakson)
                    if (current_state.square && !last_state.square) {
                        slog.println(F("[GMP] Square: Buzzer ON"));
                        byte bzData[2] = { 200, 200 }; // Frekuensi & durasi dari klakson (Buzzer)
                        uart.send(uart.mapId.buzzer.TONE, 2, bzData);
                    }

                    // Button 2: B (Circle) = Reload Megazine
                    if (current_state.circle && !last_state.circle) {
                        slog.println(F("[GMP] Circle: Reload Magazine"));
                        robotAction.rotateMegazine(LEFT, RobotAction::ALIGN_DROP_POINT, 1);
                    }

                    // Button 3: Y (Triangle) = Predicted shoot
                    if (current_state.triangle && !last_state.triangle) {
                        slog.println(F("[GMP] Triangle: Predicted Shoot"));
                        unsigned long dist = sensorState.ultrasonic;
                        if (dist > 0 && dist < 500) { // Safety check jarak
                            // Hitung kecepatan motor (50-255) berdasarkan jarak (10-300cm)
                            byte calcSpeed = map(constrain(dist, 10, 300), 10, 300, 50, 255);
                            slog.add(F("Calculated Flywheel Speed: ")); slog.println(String(calcSpeed));
                            robotAction.setFlywheelSpeed(true, calcSpeed);
                            robotAction.setFlywheelSpeed(false, calcSpeed);
                            robotAction.shoot();
                        } else {
                            slog.println(F("Target out of range or sensor error!"));
                        }
                    }

                    // Button 4: Left Bumper (L1) = Laser (ON/OFF)
                    static bool laserState = false;
                    if (current_state.l1 && !last_state.l1) {
                        laserState = !laserState;
                        slog.println(F("[GMP] L1: Toggle Laser"));
                        robotAction.laser(laserState);
                    }

                    // Button 5: Right Bumper (R1) = Led bumper Right & Left
                    if (current_state.r1 && !last_state.r1) {
                        slog.println(F("[GMP] R1: Toggle LED Bumper"));
                        robotAction.toggleLedBumper();
                    }

                    // Button 8: Select = select sequence
                    if (current_state.select && !last_state.select) {
                        if (!isSequenceRunning) {
                            selectedSequenceIndex++;
                            if (selectedSequenceIndex >= totalSequences) {
                                selectedSequenceIndex = 0; // Kembali ke sequence 1 jika terlewat
                            }
                            slog.add(F("[GMP] Select: Sequence "));
                            slog.add(String(selectedSequenceIndex + 1));
                            slog.println(F(" Selected (Ready to run)"));
                            
                            // Bunyikan buzzer sebentar (frekuensi 150, durasi 50ms)
                            byte bzData[2] = { 150, 50 }; 
                            uart.send(uart.mapId.buzzer.TONE, 2, bzData);
                        }
                    }

                    // Button 9: Start = start selected sequence
                    if (current_state.start && !last_state.start) {
                        if (isSequenceRunning) {
                            isSequenceRunning = false;
                            selectedSequenceIndex = -1; // Batalkan pilihan agar bisa pilih baru nanti
                            slog.println(F("[GMP] Start: Auto Sequence STOPPED"));
                            mecanum.stop();
                        } else {
                            if (selectedSequenceIndex != -1) {
                                isSequenceRunning = true;
                                slog.add(F("[GMP] Start: Auto Sequence "));
                                slog.add(String(selectedSequenceIndex + 1));
                                slog.println(F(" STARTED"));
                                
                                currentSequenceStep = 0;
                                sequenceStepStartTime = millis();
                                
                                String t, c, v[16]; int vCount = 0;
                                parseCmd(String(availableSequences[selectedSequenceIndex].steps[0].command), t, c, v, 16, vCount);
                                commandRun(t, c, v, vCount);
                            } else {
                                slog.println(F("[GMP] Start: No sequence selected! Press SELECT first."));
                            }
                        }
                    }

                    // D-Pad X (Kiri/Kanan) = Rotate Magazine
                    if (current_state.left && !last_state.left) {
                        slog.println(F("[GMP] D-Pad Left: Rotate Magazine Left"));
                        robotAction.rotateMegazine(LEFT, RobotAction::ALIGN_SHOOT, 1);
                    }
                    if (current_state.right && !last_state.right) {
                        slog.println(F("[GMP] D-Pad Right: Rotate Magazine Right"));
                        robotAction.rotateMegazine(RIGHT, RobotAction::ALIGN_SHOOT, 1);
                    }

                    // D-Pad Y (Atas/Bawah) = Flywheel speed
                    if (current_state.up && !last_state.up) {
                        slog.println(F("[GMP] D-Pad Up: Flywheel Speed +"));
                        robotAction.changeFlywheelSpeed(true);
                    }
                    if (current_state.down && !last_state.down) {
                        slog.println(F("[GMP] D-Pad Down: Flywheel Speed -"));
                        robotAction.changeFlywheelSpeed(false);
                    }

                    // Joystick L info
                    static int last_joy_lx = 0, last_joy_ly = 0;
                    if (current_state.stick_lx != last_joy_lx || current_state.stick_ly != last_joy_ly) {
                        slog.add(F("[JOY L] X: "));
                        slog.add(String(current_state.stick_lx));
                        slog.add(F(" | Y: "));
                        slog.add(String(current_state.stick_ly));
                        slog.println();

                        last_joy_lx = current_state.stick_lx;
                        last_joy_ly = current_state.stick_ly;
                    }

                    // Joystick R info
                    static int last_joy_rx = 0, last_joy_ry = 0;
                    if (current_state.stick_rx != last_joy_rx || current_state.stick_ry != last_joy_ry) {
                        slog.add(F("[JOY R] X: "));
                        slog.add(String(current_state.stick_rx));
                        slog.add(F(" | Y: "));
                        slog.add(String(current_state.stick_ry));
                        slog.println();

                        last_joy_rx = current_state.stick_rx;
                        last_joy_ry = current_state.stick_ry;
                    }

                    // Mecanum wheel control
                    int mec_x = current_state.stick_rx;
                    int mec_y = -current_state.stick_ry;
                    int mec_turn = current_state.stick_lx;

                    static int last_mec_x = 0, last_mec_y = 0, last_mec_turn = 0;
                    if (mec_x != last_mec_x || mec_y != last_mec_y || mec_turn != last_mec_turn) {
                        slog.add(F("[MECANUM] X: "));
                        slog.add(String(mec_x));
                        slog.add(F(" | Y: "));
                        slog.add(String(mec_y));
                        slog.add(F(" | Turn: "));
                        slog.add(String(mec_turn));
                        slog.println();

                        last_mec_x = mec_x;
                        last_mec_y = mec_y;
                        last_mec_turn = mec_turn;
                    }

                    mecanum.drive(mec_x, mec_y, mec_turn);
                } else {
                    // Pastikan mecanum benar-benar tidak bergerak
                    mecanum.stop();
                }

                // Trigger Getaran Ringan
                if (!is_rumble_burst_active) {
                    if (current_state.analog_r2 > 128) { 
                        Ps3.setRumble(current_state.analog_r2, current_state.analog_r2); 
                    } else if (current_state.r1) {
                        Ps3.setRumble(50, 0); 
                    } else if (!current_state.analog_r2 && !current_state.r1 && (last_state.analog_r2 > 0 || last_state.r1 == true)) {
                        Ps3.setRumble(0, 0);
                    }
                }

                // Eksekusi Getaran Kasar (Burst)
                if (current_state.square && !last_state.square) {
                    Ps3.setRumble(200, 200);
                    last_rumble_burst = current_millis;
                    is_rumble_burst_active = true; 
                }
                last_state = current_state;
            }
            //Gamepad action end=========================
        } else {
            if (gamepadIsConnected) {
                gamepadIsConnected = false;
                slog.println(F("Gamepad disconnected!"));
                uart.send(uart.mapId.buzzer.MELODY, 1, (byte*)&buzzerMel.CTRL_DISCONNECTED);
            }
        }

        //Gamepad end================================


        //Serial communication================================
        if (Serial.available()) {
            Serial.println(F("serial detected"));
            serialInput = Serial.readStringUntil('\n');
            serialInput.trim();
            // Bersihkan sisa variabel lama
            target = ""; command = "";
            for (int i = 0; i < 16; i++) value[i] = "";
            cmdMaxValue = sizeof(value) / sizeof(value[0]);
            
            parseCmd(serialInput, target, command, value, cmdMaxValue, cmdValueCount);

            slog.add(F("Received serial input: "));
            slog.println(serialInput);
            slog.add(F("Hasil Parsing -> target: "));
            slog.add(target);
            slog.add(F(" | command: "));
            slog.add(command);
            slog.add(F(" | value: "));
            slog.println(value[0]);
            
            if (target == "") {
                slog.println(logMes.invalidCommandTarget);
            } else if (command == "") {
                slog.println(logMes.invalidCommand);
            } else {
                cmdValueCount = sizeof(value) / sizeof(value[0]);
                commandRun(target, command, value, cmdValueCount);
            }
        }
        //Serial communication end================================

        //Auto sequence runner====================================
        if (isSequenceRunning && !robotAction.emergencyState && selectedSequenceIndex != -1) {
            const SequenceDef& currentSeq = availableSequences[selectedSequenceIndex];
            if (millis() - sequenceStepStartTime >= currentSeq.steps[currentSequenceStep].duration) {
                currentSequenceStep++;
                if (currentSequenceStep >= currentSeq.length) {
                    isSequenceRunning = false;
                    selectedSequenceIndex = -1; // Reset pilihan ke default jika telah selesai dieksekusi
                    slog.println(F("[AUTO] Sequence FINISHED"));
                    mecanum.stop();
                } else {
                    sequenceStepStartTime = millis();
                    // Local temporary var (membebaskan global scope memory cache)
                    String t, c, v[16]; int vCount = 0;
                    parseCmd(String(currentSeq.steps[currentSequenceStep].command), t, c, v, 16, vCount);
                    commandRun(t, c, v, vCount);
                }
            }
        }
        //Auto sequence runner end================================
        
        static unsigned long lastTime = 0;
        static unsigned long loopCountCore1 = 0;

        loopCountCore1++;

        if (millis() - lastTime >= 1000) {
            slog.add(F("Loop rate -> Core 0: "));
            slog.add(String(loopCountCore0.get()));
            slog.add(F(" Hz | Core 1: "));
            slog.add(String(loopCountCore1));
            slog.add(F(" Hz"));
            slog.println();

            loopCountCore0.set(0);
            loopCountCore1 = 0;
            lastTime = millis();
        }

        vTaskDelay(5 / portTICK_PERIOD_MS);
        yield();
    }
}

void runtime(void *pvParameters) {
    esp_task_wdt_add(NULL);

    unsigned long lastDiagnosticTime = 0;

    // Inisialisasi pin untuk sensor tegangan dan arus
    // Pastikan di pins.h Anda mengubah pin 0 menjadi pin ADC ESP32 yang valid (misal: 34, 35, 36, 39)
    pinMode(pins.powerMonitor.voltageSensor, INPUT);
    pinMode(pins.powerMonitor.currentSensor, INPUT);

    while (1) {
        esp_task_wdt_reset();
        
        // Power Monitor (Tegangan, Arus, dan Baterai)
        int rawV = analogRead(pins.powerMonitor.voltageSensor);
        int rawI = analogRead(pins.powerMonitor.currentSensor);
        
        // Konversi pembacaan raw ADC (0-4095) ke Tegangan Pin (0-3.3V)
        float vPin = (rawV / 4095.0) * 3.3;
        float iPin = (rawI / 4095.0) * 3.3;

        // Konstanta Kalibrasi (Ubah sesuai dengan komponen hardware sensor yang Anda pakai)
        const float VOLTAGE_MULTIPLIER = 5.0;    // Rasio Voltage Divider
        const float CURRENT_SENSITIVITY = 0.185; // Sensitivitas modul ACS712-5A (185mV/A)
        const float CURRENT_ZERO_POINT = 1.65;   // Nilai tegangan tengah saat 0 Ampere (VCC/2)

        sensorState.power.voltage = vPin * VOLTAGE_MULTIPLIER;
        sensorState.power.current = (iPin - CURRENT_ZERO_POINT) / CURRENT_SENSITIVITY;
        if (sensorState.power.current < 0.05) sensorState.power.current = 0.0; // Filter out noise sensor arus

        // Persentase Baterai (Diasumsikan menggunakan 3S LiPo: Min 9.0V, Max 12.6V)
        const float BATTERY_MIN = 9.0;
        const float BATTERY_MAX = 12.6;
        float percent = ((sensorState.power.voltage - BATTERY_MIN) / (BATTERY_MAX - BATTERY_MIN)) * 100.0;
        sensorState.power.battery = constrain((int)percent, 0, 100);
        // --------------------------------------------------

        if (millis() - lastDiagnosticTime >= 10000) {
            lastDiagnosticTime = millis();

            uint32_t freeHeap = ESP.getFreeHeap();
            UBaseType_t stackWaterMark = uxTaskGetStackHighWaterMark(NULL);

            slog.add(F("[RUNTIME] Free Heap: "));
            slog.add(String(freeHeap));
            slog.add(F(" | Stack Watermark: "));
            slog.add(String(stackWaterMark));
            slog.println();

            if (freeHeap < 10000) {
                slog.println(F("[CRITICAL] Low Memory! System Restarting..."));
                delay(1000);
                ESP.restart();
            }

            if (!isNanoConnected && !robotAction.emergencyState) {
                slog.println(F("[CRITICAL] Nano Offline! Forcing Emergency Mode..."));
                robotAction.toggleEmergency();
            }
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS);
        yield();
    }
}










/*
=======================================
                Core 0
=======================================
*/
void wlConnection(void *pvParameters) {
    static long lastMillis = 0;
    static bool wlConnectionFunctionHasRunOnce = false;

    if (!wlConnectionFunctionHasRunOnce) {
        initGamepad(secretData.PS3_MAC_ADDRESS);
        
        wlConnectionFunctionHasRunOnce = true;
    }

    while (1) {

        if (Ps3.isConnected()) {
            globalGamepadState.set(gamepadState);
        }

        loopCountCore0.set(loopCountCore0.get() + 1);

        vTaskDelay(15 / portTICK_PERIOD_MS);
        yield();
    }
}

void setup(){
    esp_task_wdt_init(5, true);

    // CORE 0
    xTaskCreatePinnedToCore(
        wlConnection,
        "wlConnection task",
        50000,
        NULL,
        1,
        NULL,
        0
    );

    // CORE 1
    xTaskCreatePinnedToCore(
        mainFunction,
        "main task",
        8192,
        NULL,
        2,      // Prioritas dinaikkan menjadi 2 (Lebih tinggi dari task background)
        NULL,
        1
    );

    xTaskCreatePinnedToCore(
        runtime,
        "runtime task",
        2048,
        NULL,
        1,
        NULL,
        0       // Dipindah ke Core 0 agar tidak mengganggu kecepatan kalkulasi mecanum di Core 1
    );
}

void loop(){

}


//////////////////////////////////////////////////////////////
