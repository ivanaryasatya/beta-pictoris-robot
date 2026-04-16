
#include <Arduino.h> 
//#include "FirebaseHandler.h"
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
//#include "wifiHandler.h"
#include "motorDriver.h"
#include "wheelDrive.h"
#include "gamepadHandler.h"
#include <pgmspace.h>
#include "irSensor.h"


// #define WIFI_SSID secretData.WIFI_SSID
// #define WIFI_PASSWORD secretData.WIFI_PASSWORD
// #define API_KEY secretData.Web_API_KEY
// #define DATABASE_URL secretData.DATABASE_URL
// #define USER_EMAIL secretData.USER_EMAIL
// #define USER_PASSWORD secretData.USER_PASSWORD
using cbyte = const byte;
using cuint = const unsigned int;
using uint = unsigned int;

// Struct wrapper untuk array String[10]
struct StringArray10 {
    String data[10];
};

//FirebaseHandler firebase;
EEPROMManager memory;
MutexData<int> sendNumber(0);

MutexData<String> mutexCommand;
MutexData<String> mutexCmdTarget;
MutexData<StringArray10> mutexCmdValue;
MutexData<byte> mutexCmdValueCount(0);

uint count = 0;
cbyte esp32 = 0;
cbyte nano = 1;
byte gmpDeadZone = 2;
int test = 0;
bool serialLogState = true;
bool gamepadIsConnected = false;
unsigned int nanoPing = 0;

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
Servo barrelServo;
Servo pusherServo;
Servo megazineServo;

bool megazine[11] = {false, false, false, false, false, false, false, false, false, false, false};
int currentMegazineSlot = 1; // Slot 1 dimulai dari drop point

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


void cmdValueError(const byte expected, const byte actual) {
    slog.add(logMes.commandValueLessThanExpected);
    slog.add(String(cmdValueCount));
    slog.println();
}

/*
true = light sleep
false = deep sleep
time in second
*/
void espSleep(const bool isLightmode, const unsigned int time) {
    esp_sleep_enable_timer_wakeup(time * 1000000ULL);
    if (isLightmode == 0) {
        esp_light_sleep_start();
    } else {
        esp_deep_sleep_start();
    }
}

bool checkValue(const byte valueCount, const byte minCount) {
    if (valueCount < minCount) {
        cmdValueError(minCount, cmdValueCount);
        return false;
    }
    return true;
}

struct RobotAction {
    byte upArmAngle = 120;
    byte downArmAngle = 0;
    byte openCatcherAngle = 20;
    byte closeCatherAngle = 0;
    byte pusherPushAngle = 180;
    byte pusherIdleAngle = 0;

    void setDefault() {
        servo180(pusherServo, 0);
    }

    void takeBall() {

        uart.send(uart.mapId.servo.catcher.ANGLE, 1, (byte*)&closeCatherAngle); // tutup cather
        delay(500);
        uart.send(uart.mapId.servo.arm.ANGLE, 1, (byte*)&upArmAngle); //angkat lengan
        delay(500);
        uart.send(uart.mapId.servo.catcher.ANGLE, 1, (byte*)&openCatcherAngle); //buka cather
        delay(500);
        uart.send(uart.mapId.servo.arm.ANGLE, 1, (byte*)&downArmAngle); //turunkan lengan
        delay(500);
        
    }

    void shoot() {
        servo180(pusherServo, pusherPushAngle);
        delay(500);
        servo180(pusherServo, pusherIdleAngle);
        delay(500);
    }

    void rotateMegazine(bool rotateRight, byte targetCount = 1) {
        byte currentCount = 0;
        bool lastState = false;
        
        while (currentCount < targetCount && targetCount <= 11) {
            static bool currentState = sensorState.hall.right && sensorState.hall.left;
            if (rotateRight) {
                servo360Right(megazineServo, 40);
                if (currentState && !lastState) {
                    currentMegazineSlot = (currentMegazineSlot % 11) + 1;
                }
            } else {
                servo360Left(megazineServo, 40);
                if (currentState && !lastState) {
                    currentMegazineSlot = (currentMegazineSlot == 1) ? 11 : currentMegazineSlot - 1;
                }
            }

            if (currentState && !lastState) {
                currentCount++;
                megazine[currentMegazineSlot - 1] = sensorState.ir.dropPoint;
            }
            lastState = currentState;
            delay(10);
        }
        
        servo360Stop(megazineServo);
    }

} robotAction;

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
            strcasecmp(c, "on") == 0 || strcasecmp(c, "enable") == 0 || 
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
    if (uartCmd == uart.mapId.PING) {
        uart.send(uart.mapId.PONG, 0, NULL);
    } else if (uartCmd == uart.mapId.PONG) {
        lastPongReceivedMicros = micros();
        nanoPing = lastPongReceivedMicros - lastPingMicros;
    } else if (uartCmd == uart.mapId.RESTART) {
        espResetTimer = data[0] * 1000;
    } else if (uartCmd == uart.mapId.USER_CMD) {

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
    } else if (uartCmd == uart.mapId.ULTRASONIC) {
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
        }
    }

    else if (target == F("esp32")) {
        if (command == F("ping")) {
            uart.send(uart.mapId.PING, 0, NULL);
        } else if (command == F("memSave")) {
            memory.save(value[0][0], value[1]);
        } else if (command == F("memGetAll")) {
            slog.println(memory.getAll());
        } else if (command == F("wifiBegin")) {
            // if (valueCount >= 2) {
            //     wifi.connect();
            // } else {
            //     slog.add(logMes.commandValueLessThanExpected);
            //     slog.add(String(valueCount));
            //     slog.println();
            // }
        } else if (command == F("wifiStatus")) {
            // slog.println(wifi.isConnected() ? logMes.wifiConnected : logMes.wifiNotConnected);
        } else if (command == F("wifiLocalIP")) {
            // slog.add(logMes.wifiLocalIP);
            // slog.add(wifi.getIP());
            // slog.println();
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
        
        }
    } else if (target == F("gmp")) {
        if (command == F("battery")) {
            slog.add(F("gamepad battery: "));
            slog.add(gamepadState.battery_status);
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
        servoAttach(barrelServo, pins.servo.barrelPuller);
        servoAttach(pusherServo, pins.servo.pusher);
        servoAttach(megazineServo, pins.servo.megazine);

        Serial.begin(115200);
        Serial1.begin(9600, SERIAL_8N1, pins.esp32Uart.rx, pins.esp32Uart.tx);
        uart.begin(uartCommandRun);
        
        memory.begin(512);
        serialLogState = memory.get(epmPtr.logState) == "1";
        slog.enable(serialLogState);

        robotAction.setDefault();

        mainFunctionHasRunOnce = true;
    }





    while (true) {
        uart.update();

        // Ping Nano setiap 500ms
        if (micros() - lastPingMicros >= pingInterval) { // 500000 mikrodetik = 500 ms
            lastPingMicros = micros();
            uart.send(uart.mapId.PING, 0, NULL);
        }

        //Gamepad=========================================
        if (Ps3.isConnected()) {
            gamepadIsConnected = true;
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

                if (abs(current_state.stick_lx - last_state.stick_lx) > 2 || 
                    abs(current_state.stick_ly - last_state.stick_ly) > 2 || 
                    abs(current_state.stick_rx - last_state.stick_rx) > 2 || 
                    abs(current_state.stick_ry - last_state.stick_ry) > 2) {
                    Serial.printf("Joy L [X: %d, Y: %d] | Joy R [X: %d, Y: %d]\n", 
                    current_state.stick_lx, current_state.stick_ly, 
                    current_state.stick_rx, current_state.stick_ry);
                }

                // barrel elevation
                if (current_state.stick_ly < -20) { // Joystick ke Atas
                    int speed = map(current_state.stick_ly, -20, -128, 0, 90);
                    servo360Left(barrelServo, speed);
                } else if (current_state.stick_ly > 20) { // Joystick ke Bawah
                    int speed = map(current_state.stick_ly, 20, 127, 0, 90);
                    servo360Right(barrelServo, speed);
                } else {
                    servo360Stop(barrelServo);
                }
                
                // Pusher action shoot (Triangle/Y button)
                if (current_state.triangle && !last_state.triangle) {
                    robotAction.shoot();
                }

                // Arm action take ball (cross/A button)
                if (current_state.cross && !last_state.cross) {
                    robotAction.takeBall();
                }

                //
                if (current_state.square && !last_state.square) {

                }


                // Mecanum wheel control
                mecanum.drive(current_state.stick_rx, -current_state.stick_ry, current_state.stick_lx);

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
            gamepadIsConnected = false;
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

            slog.println("Received serial input: " + serialInput);
            slog.println("Hasil Parsing -> target: " + target + " | command: " + command + " | value: " + value[0]);
            
            if (target == "") {
                slog.println(logMes.invalidCommandTarget);
            } else if (command == "") {
                slog.println(logMes.invalidCommand);
            } else {
                cmdValueCount = sizeof(value) / sizeof(value[0]);

                mutexCommand.set(command);
                mutexCmdTarget.set(target);
                mutexCmdValueCount.set(cmdMaxValue);
                StringArray10 tempValue;
                for (int i = 0; i < 10; i++) {
                    tempValue.data[i] = value[i];
                }
                mutexCmdValue.set(tempValue);
                commandRun(target, command, value, cmdValueCount);
            }
        }
        //Serial communication end================================

        static unsigned long lastTime = 0;
        static unsigned long loopCount = 0;

        loopCount++;

        if (millis() - lastTime >= 1000) {
            slog.add("Loop rate core1: ");
            slog.add(String(loopCount));
            slog.add(" Hz");
            slog.println();

            loopCount = 0;
            lastTime = millis();
        }
        

        static unsigned long coreLastMillis = 0;
        if (millis() - coreLastMillis >= 2000) {
            slog.println("core 1 is alive");
            coreLastMillis = millis();
        }

        vTaskDelay(5 / portTICK_PERIOD_MS);
        yield();
    }
}

void runtime(void *pvParameters) {
    while (1) {
        
        vTaskDelay(5 / portTICK_PERIOD_MS);
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
        
        // if (wifi.connect()) {
            
        // }
    //     firebase.begin(
    //         secretData.Web_API_KEY,
    //         secretData.DATABASE_URL,
    //         secretData.USER_EMAIL,
    //         secretData.USER_PASS
    //     );
        wlConnectionFunctionHasRunOnce = true;
    }

    while (1) {

        // Backup state background ke Mutex Data secara kontinyu dari PS3 (Core 0)
        if (Ps3.isConnected()) {
            globalGamepadState.set(gamepadState);
        }

        if (millis() - lastMillis >= 400) {
            lastMillis = millis();
            // firebase.sendInt("/test/sensor", number.get());
            // esp_task_wdt_reset();
            // parseCmd(firebase.getString("/test/command"), target, command, value, cmdMaxValue, cmdValueCount);
            // esp_task_wdt_reset();
            // if (command != ""){ slog.println(command); }
            //count ++;
        }

        static unsigned long coreLastMillis = 0;
        if (millis() - coreLastMillis >= 2000) {
            slog.println("core 0 is alive");
            coreLastMillis = millis();
        }


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
        1,
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
        1
    );
}

void loop(){

}


//////////////////////////////////////////////////////////////
