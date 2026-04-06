
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

struct InputCommand {
    String target;
    String command;
    String value[16];
    byte valueCount;
};

//FirebaseHandler firebase;
EEPROMManager memory;
MutexData<int> sendNumber(0);
MutexData<InputCommand> InputCommand;


uint count = 0;
cbyte esp32 = 0;
cbyte nano = 1;
int test = 0;
bool serialLogState = false;

// Command parsing
String target, command, value[16];
int cmdMaxValue, cmdValueCount;

String serialInput;
UARTProtocol uart(Serial2);

// Shared Data Gamepad (Lintas Core Bebas-Crash)
MutexData<GamepadState> globalGamepadState;
GamepadState last_state;

unsigned long last_rumble_burst = 0;
bool is_rumble_burst_active = false;
const unsigned long RUMBLE_BURST_DURATION = 200;
const char* spaceStr = " ";
const char* NANO_STR = "NANO";
const char* ESP32_STR = "ESP32";
const char* RBT_STR = "RBT";
int espResetTimer = -1;

MotorDriver motor(pins.wheelDriver_r.ENA, pins.wheelDriver_r.IN1, pins.wheelDriver_r.IN2, pins.wheelDriver_r.IN3, pins.wheelDriver_r.IN4, pins.wheelDriver_r.ENB);
MotorDriver motor2(pins.wheelDriver_l.ENA, pins.wheelDriver_l.IN1, pins.wheelDriver_l.IN2, pins.wheelDriver_l.IN3, pins.wheelDriver_l.IN4, pins.wheelDriver_l.ENB);

// Instansiasi Eksekutor Kendaraan Mecanum (motor2 = Kiri, motor = Kanan)
MecanumDrive mecanum(&motor2, &motor);


// Fungsi filter untuk menghindari spam output ke serial monitor
bool isGamepadChanged(const GamepadState& current, const GamepadState& last) {
    if (abs(current.stick_lx - last.stick_lx) > 2) return true;
    if (abs(current.stick_ly - last.stick_ly) > 2) return true;
    if (abs(current.stick_rx - last.stick_rx) > 2) return true;
    if (abs(current.stick_ry - last.stick_ry) > 2) return true;

    if (abs(current.analog_l2 - last.analog_l2) > 2) return true;
    if (abs(current.analog_r2 - last.analog_r2) > 2) return true;

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

void uartCommand(byte cmd, byte id, byte *data, byte len) {

}


void cmdValueError(const byte expected, const byte actual) {
    slog.add(logMes.commandValueLessThanExpected);
    slog.add(String(cmdValueCount));
    slog.println();
}

// Fungsi cek boolean dari string
//
// str:
// true: "1", "true", "on", "enable", "yes"
// false: "0", "false", "off", "disable", "no"
// number:
// true: >= 1
// false: <= 0
struct State {
    // Menggunakan const String &s tetap menjaga kompatibilitas, 
    // tapi kita akses sebagai c_str() agar lebih ringan saat pembandingan.
    bool str(const String &s) {
        const char* c = s.c_str();

        if (strcmp(c, "1") == 0 || strcmp(c, "true") == 0 || strcmp(c, "on") == 0 || 
            strcmp(c, "enable") == 0 || strcmp(c, "yes") == 0) {
            return true;
        }

        if (strcmp(c, "0") == 0 || strcmp(c, "false") == 0 || strcmp(c, "off") == 0 || 
            strcmp(c, "disable") == 0 || strcmp(c, "no") == 0) {
            return false;
        } 
        slog.println(logMes.invalidCommand);
        return false;
    }

    // Fungsi num diperbaiki return type-nya menjadi bool agar konsisten
    bool num(const int number) {
        if (number >= 1) return true;
        if (number <= 0) return false;
        
        slog.println(logMes.invalidCommand);
        return false;
    }
} state;


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

    else if (target == "esp32" || target == ESP32_STR) {
        if (command == "restart") {
            slog.println(logMes.esp32Restarting);
            ESP.restart();
        } else if (command == "memSave") {
            memory.save(value[0][0], value[1]);
        } else if (command == "memGetAll") {
            slog.println(memory.getAll());
        } else if (command == "wifiBegin") {
            // if (valueCount >= 2) {
            //     wifi.connect();
            // } else {
            //     slog.add(logMes.commandValueLessThanExpected);
            //     slog.add(String(valueCount));
            //     slog.println();
            // }
        } else if (command == "wifiStatus") {
            // slog.println(wifi.isConnected() ? logMes.wifiConnected : logMes.wifiNotConnected);
        } else if (command == "wifiLocalIP") {
            // slog.add(logMes.wifiLocalIP);
            // slog.add(wifi.getIP());
            // slog.println();
        } else if (command == "freeHeap") {
            slog.add("Free heap: ");
            slog.add(String(ESP.getFreeHeap()));
            slog.println();
        } else if (command == "minFreeHeap") {
            slog.add("Minimum free heap: ");
            slog.add(String(ESP.getMinFreeHeap()));
            slog.println();
        } else if (command == "enableSerialLog") {
            if (state.str(value[0])) {
                serialLogState = true;
                slog.enable(true);
                if (!memory.save(epmPtr.logState, "1")) {
                    slog.println(logMes.eepromSaveFailed);
                }
            } else {
                serialLogState = false;
                slog.enable(false);
                slog.println("Serial log disabled");
            }
        } else if (command == "restart") {
            slog.println(logMes.esp32Restarting);
            ESP.restart();
        } else {
            slog.println(logMes.invalidCommand);
            return false;
        }
    } else if (target == "nano" ||target == NANO_STR) {
        if (command == "sendCommand" && valueCount >= 1) {
            uart.send(uart.mapId.USER_CMD, 0, (byte*)value[0].c_str());
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
                Core 1
=======================================
*/
void mainFunction(void *pvParameters) {
    static bool mainFunctionHasRunOnce = false;
    if (!mainFunctionHasRunOnce) {
        slog.println("ESP32 is starting up");
        
        // Inisialisasi awal hardware L298N
        motor.begin();
        motor2.begin();
        mecanum.stop();

        slog.enable(true);
        Serial.begin(115200);
        Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
        uart.begin(uartCommand);
        
        memory.begin(512);
        serialLogState = memory.get(epmPtr.logState) == "1";
        slog.enable(serialLogState);
        mainFunctionHasRunOnce = true;
    }





    while (true) {
        uart.update();

        //Gamepad=========================================
        if (Ps3.isConnected()) {
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
                
                if (current_state.cross && !last_state.cross) {
                    slog.println("Tombol Silang (X) Ditekan! - Aksi: Melompat");
                }
                if (current_state.square && !last_state.square) {
                    slog.println("Tombol Kotak Ditekan! - Aksi: Serang / Reload");
                }
                if (current_state.triangle && !last_state.triangle) {
                    slog.println("Tombol Segitiga Ditekan! - Aksi: Interaksi");
                }
                if (current_state.circle && !last_state.circle) {
                    slog.println("Tombol Lingkaran Ditekan! - Aksi: Batal / Menghindar");
                }

                if (current_state.up && !last_state.up) {
                    slog.println("D-Pad Atas Ditekan!");
                }
                if (current_state.down && !last_state.down) {
                    slog.println("D-Pad Bawah Ditekan!");
                }
                if (current_state.left && !last_state.left) {
                    slog.println("D-Pad Kiri Ditekan!");
                }
                if (current_state.right && !last_state.right) {
                    slog.println("D-Pad Kanan Ditekan!");
                }

                if (current_state.l1 && !last_state.l1) {
                    slog.println("L1 Ditekan!");
                }
                if (current_state.r1 && !last_state.r1) {
                    slog.println("R1 Ditekan!");
                }
                
                if (current_state.l3 && !last_state.l3) {
                    slog.println("L3 (Joystick Kiri Masuk) Ditekan!");
                }
                if (current_state.r3 && !last_state.r3) {
                    slog.println("R3 (Joystick Kanan Masuk) Ditekan!");
                }

                if (current_state.start && !last_state.start) {
                    slog.println("Start Ditekan!");
                }
                if (current_state.ps && !last_state.ps) {
                    slog.println("PS Button Ditekan!");
                }

                // Sensor dengan log framework
                if (current_state.select && !last_state.select) {
                    slog.println("\n--- [INFO SENSOR TERKINI] ---");
                    slog.add("  - Baterai      : "); slog.println(current_state.battery_status);
                    slog.add("  - Accelerometer: X: "); slog.add(String(current_state.accel_x)); slog.add(" Y: "); slog.add(String(current_state.accel_y)); slog.add(" Z: "); slog.println(String(current_state.accel_z));
                    slog.add("  - Gyroscope    : Z: "); slog.println(String(current_state.gyro_z));
                }

                if (abs(current_state.stick_lx) > 10 || abs(current_state.stick_ly) > 10) {
                    slog.add("Joy L Berjalan -> X: "); slog.add(String(current_state.stick_lx)); slog.add(" | Y: "); slog.println(String(current_state.stick_ly));
                }
                if (abs(current_state.stick_rx) > 10 || abs(current_state.stick_ry) > 10) {
                    slog.add("Joy R Berjalan -> X: "); slog.add(String(current_state.stick_rx)); slog.add(" | Y: "); slog.println(String(current_state.stick_ry));
                }

                // ==========================================
                // KONTROL MECANUM WHEELS (Kirim PWM ke Driver L298N)
                // ==========================================
                // Mapping: Joy Kanan = Maju/Strafe, Joy Kiri = Rotasi/Putar Kiri
                // Nilai Y harus di-inverse karena pada PS3 ditekuk ke atas = bernilai negatif
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
        }
        //Gamepad end================================


        //Serial communication================================
        if (Serial.available()) {
            serialInput = Serial.readStringUntil('\n');
            serialInput.trim();
            // Bersihkan sisa variabel lama
            target = ""; command = "";
            for (int i = 0; i < 16; i++) value[i] = ""; // Harus dibongkar juga untuk mencegah "ghost data"

            // Eksekusi Parsing (Pastikan ngirim cmdMaxValue yang BUKAN 0)
            cmdMaxValue = sizeof(value) / sizeof(value[0]);
            parseCmd(serialInput, target, command, value, cmdMaxValue, cmdValueCount);
            
            // Lakukan print DEBUGGING SETELAH fungsi parsing!
            slog.println("Received serial input: " + serialInput);
            slog.println("Hasil Parsing -> target: " + target + " | command: " + command + " | value: " + value[0]);
            
            if (serialInput == "s") {
                static bool isSerialInputEnabled = false; 
                isSerialInputEnabled = !isSerialInputEnabled; 
                slog.enable(isSerialInputEnabled);
                slog.add("Log status: ");
                slog.add(isSerialInputEnabled ? "ON" : "OFF");
                slog.println();
            }

            // --- PROTEKSI TARGET ---
            // Hanya eksekusi commandRun() jika var target tidak dibiarkan kosong
            if (target == "") {
                // Biarkan saja (Tidak usah lempar error karena ini bisa saja cuma ketikan tes 's')
            } else if (command == "") {
                slog.println(logMes.invalidCommand);
            } else {
                cmdValueCount = sizeof(value) / sizeof(value[0]);
                interCoreCmdCommand.set(command);
                interCoreCmdTarget.set(target);
                StringArray10 tempValue;
                for (int i = 0; i < 10; i++) {
                    tempValue.data[i] = value[i];
                }
                interCoreCmdValue.set(tempValue);

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
        
        //ESP32 restart timer============================
        static unsigned long resetTimerLastMillis = 0;
        static bool isTimerActive = false;

        if (espResetTimer > 0 && !isTimerActive) {
            resetTimerLastMillis = millis();
            isTimerActive = true;

            slog.print(logMes.esp32WillRestartIn);
            slog.print(String(espResetTimer / 1000));
            slog.println(logMes.seconds);
        } 

        else if (espResetTimer <= 0 && isTimerActive) {
            isTimerActive = false;
            slog.println("Restart dibatalkan.");
        }

        if (isTimerActive) {
            if (millis() - resetTimerLastMillis >= (unsigned long)espResetTimer) {
                slog.println(logMes.esp32Restarting);
                isTimerActive = false;
                espResetTimer = 0; 

                delay(500);
                ESP.restart();
            }
        }
        //ESP32 restart timer endl=========================
        

        static unsigned long coreLastMillis = 0;
        if (millis() - coreLastMillis >= 2000) {
            slog.println("core one is alive");
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
            globalGamepadState.set(gamepad_state);
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
            slog.println("core zero is alive");
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





