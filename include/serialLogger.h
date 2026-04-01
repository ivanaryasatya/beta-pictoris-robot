#pragma once

#include <Arduino.h>

#if defined(ESP32)
  #define DEVICE_NAME "ESP32"
#elif defined(ARDUINO_AVR_NANO)
  #define DEVICE_NAME "NANO"
#elif defined(ARDUINO_UNO)
  #define DEVICE_NAME "UNO"
#else
  #define DEVICE_NAME "UNKNOWN"
#endif

class Logger {
  private:
    String buffer;
    bool enableLog = false;

  public:
    void add(String text) {
      buffer += text;
    }

    void addLine(String text) {
      buffer += text + "\n";
    }

    void clear() {
      buffer = "";
    }

    void print(const String &text = "") {
        if (!enableLog) return;
        if (text != "") {
            buffer = "";
            add(text);
          }
      Serial.print(">> ");
      Serial.print(DEVICE_NAME);
      Serial.print(" [");
      Serial.print(millis());
      Serial.print(" ms]: ");

      Serial.print(buffer);
      clear();
    }

    void println(const String &text = "") {
        if (!enableLog) return;
        if (text != "") {
            buffer = "";
            addLine(text);
        }
        Serial.print(">> ");
        Serial.print(DEVICE_NAME);
        Serial.print(" [");
        Serial.print(millis());
        Serial.print(" ms]: ");

        Serial.println(buffer);
        clear();
    }

    void enable(const bool state) {
        enableLog = state;
    }
};

Logger slog;