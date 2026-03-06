#pragma once

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

template <typename T>
class MutexData {
private:
    T data;
    SemaphoreHandle_t mutex;

public:

    // Constructor
    MutexData() {
        mutex = xSemaphoreCreateMutex();
    }

    // Constructor dengan nilai awal
    MutexData(T initialValue) {
        data = initialValue;
        mutex = xSemaphoreCreateMutex();
    }

    // Set data dengan mutex lock
    void set(T value) {
        if (xSemaphoreTake(mutex, portMAX_DELAY)) {
            data = value;
            xSemaphoreGive(mutex);
        }
    }

    // Get data dengan mutex lock
    T get() {
        T temp;
        if (xSemaphoreTake(mutex, portMAX_DELAY)) {
            temp = data;
            xSemaphoreGive(mutex);
        }
        return temp;
    }

    // Akses langsung dengan fungsi callback (lebih aman)
    void access(void (*func)(T&)) {
        if (xSemaphoreTake(mutex, portMAX_DELAY)) {
            func(data);
            xSemaphoreGive(mutex);
        }
    }

};




//
// #include "MutexData.h"

// MutexData<int> sharedNumber(0);

// void taskCore0(void *pvParameters) {
//     while (1) {
//         int value = sharedNumber.get();
//         value++;
//         sharedNumber.set(value);

//         Serial.print("Core0: ");
//         Serial.println(value);

//         vTaskDelay(1000 / portTICK_PERIOD_MS);
//     }
// }

// void taskCore1(void *pvParameters) {
//     while (1) {
//         int value = sharedNumber.get();

//         Serial.print("Core1 read: ");
//         Serial.println(value);

//         vTaskDelay(1500 / portTICK_PERIOD_MS);
//     }
// }

// void setup() {
//     Serial.begin(115200);

//     xTaskCreatePinnedToCore(
//         taskCore0,
//         "Task0",
//         2048,
//         NULL,
//         1,
//         NULL,
//         0
//     );

//     xTaskCreatePinnedToCore(
//         taskCore1,
//         "Task1",
//         2048,
//         NULL,
//         1,
//         NULL,
//         1
//     );
// }

// void loop() {}