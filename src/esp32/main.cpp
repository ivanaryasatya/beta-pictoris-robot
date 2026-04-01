
#include <Arduino.h> 
#include "FirebaseHandler.h"
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
#include "wifiHandler.h"
#include "motorDriver.h"

// #define WIFI_SSID secretData.WIFI_SSID
// #define WIFI_PASSWORD secretData.WIFI_PASSWORD
// #define API_KEY secretData.Web_API_KEY
// #define DATABASE_URL secretData.DATABASE_URL
// #define USER_EMAIL secretData.USER_EMAIL
// #define USER_PASSWORD secretData.USER_PASSWORD

#define RXD2 16
#define TXD2 17

using cbyte = const byte;
using uint = unsigned int;

FirebaseHandler firebase;
EEPROMManager memory;
MutexData<int> sendNumber(0);
MutexData<String> interCoreCmdCommand("");  
MutexData<String> interCoreCmdTarget("");
MutexData<String[10]> interCoreCmdValue;

uint count = 0;
cbyte esp32 = 0;
cbyte nano = 1;
int test = 0;
bool serialLogState = false;

// Command parsing
String target, command, value[10];
int cmdMaxValue, cmdValueCount;

String serialInput;
UARTProtocol uart(Serial2);

MotorDriver motor(pins.wheelDriver_r.ENA, pins.wheelDriver_r.IN1, pins.wheelDriver_r.IN2, pins.wheelDriver_r.IN3, pins.wheelDriver_r.IN4, pins.wheelDriver_r.ENB);
MotorDriver motor2(pins.wheelDriver_l.ENA, pins.wheelDriver_l.IN1, pins.wheelDriver_l.IN2, pins.wheelDriver_l.IN3, pins.wheelDriver_l.IN4, pins.wheelDriver_l.ENB);

void handleCommand(byte cmd, byte id, byte *data, byte len) {
}

bool commandRun(const String &target, const String &command, const String value[], const byte valueCount) {

    // format: target.command.value1.value2;
    // contoh: esp32.memSave.q.MyWiFi; atau nano.led.on;
    if (target == "rbt") {
        if (command == "restart") {
            uart.send(uart.mapId.USER_CMD, 0, NULL);
            delay(100);
            ESP.restart();
            return true;
        }
    }

    else if (target == "esp32") {
        if (command == "restart") {
            
        } else if (command == "memSave") {
            memory.save(value[0][0], value[1]);
        } else if (command == "memGetAll") {
            slog.println(memory.getAll());
        } else if (command == "wifiBegin") {
            if (valueCount >= 2) {
                wifi.connect();
            } else {
                slog.add(logMes.commandValueLessThanExpected);
                slog.add(String(valueCount));
                slog.println();
            }
        } else if (command == "wifiStatus") {
            slog.println(wifi.isConnected() ? logMes.wifiConnected : logMes.wifiNotConnected);
        } else if (command == "wifiLocalIP") {
            slog.add(logMes.wifiLocalIP);
            slog.add(wifi.getIP());
            slog.println();
        } else if (command == "wifiScan") {
        } else if (command == "wifiList") {
        } else if (command == "sendNumber") {
            int num = value[0].toInt();
            sendNumber.set(num);
        } else if (command == "freeHeap") {
            slog.add("Free heap: ");
            slog.add(String(ESP.getFreeHeap()));
            slog.println();
        } else if (command == "minFreeHeap") {
            slog.add("Minimum free heap: ");
            slog.add(String(ESP.getMinFreeHeap()));
            slog.println();
        } else {
            slog.println(logMes.invalidCommand);
            return false;
        }
    } else if (target == "nano") {
        if (command == "sendCommand" && valueCount >= 1) {
            uart.send(uart.mapId.USER_CMD, 0, (byte*)value[0].c_str());
        }
    } else {
        slog.println(logMes.invalidCommandTarget);
        return false;
    }
    return false;
}













/*
=======================================
                Core 1
=======================================
*/
void mainFunction(void *pvParameters) {
    static bool mainFunctionHasRunOnce = false;
    if (!mainFunctionHasRunOnce) {
        slog.println("ESP32 is starting up");
        
        slog.enable(true);
        Serial.begin(115200);
        Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
        uart.begin(handleCommand);
        
        memory.begin(512);
        serialLogState = memory.get(epmPtr.logState) == "1";
        slog.enable(serialLogState);
        mainFunctionHasRunOnce = true;
    }



    while (true) {
        uart.update();

        if (Serial.available()) {
            serialInput = Serial.readStringUntil('\n');
            serialInput.trim();
            parseCmd(serialInput, target, command, value, cmdMaxValue, cmdValueCount);
            
            if (serialInput == "s") {
                slog.enable(true);
            }

            if (target == "") {
                slog.println(logMes.invalidCommandTarget);
            } else if (command == "") {
                slog.println(logMes.invalidCommand);
            } else {
                cmdValueCount = sizeof(value) / sizeof(value[0]);
                interCoreCmdCommand.set(command);
                interCoreCmdTarget.set(target);
                interCoreCmdValue.set(value);

                commandRun(target, command, value, cmdValueCount);
            }
        }

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
        if (millis() - coreLastMillis >= 1000) {
            Serial.println("core one is alive");
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
void internet(void *pvParameters) {
    static long lastMillis = 0;
    static bool internetFunctionHasRunOnce = false;

    if (!internetFunctionHasRunOnce) {
         wifi.connect();
    //     firebase.begin(
    //         secretData.Web_API_KEY,
    //         secretData.DATABASE_URL,
    //         secretData.USER_EMAIL,
    //         secretData.USER_PASS
    //     );
        internetFunctionHasRunOnce = true;
    }

    while (1) {

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
        if (millis() - coreLastMillis >= 1000) {
            Serial.println("core zero is alive");
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
        internet,
        "internet task",
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





