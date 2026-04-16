#pragma once

#include <Arduino.h>
//#include "firebaseHandler.h"

class Logger {
  private:
    String buffer;
    bool enableLog = false;
    bool rtdbSyncLog = false;
    bool BTlog = false;
    void startText() {
#if defined(ESP32)
      Serial.print(F("E"));
#else
      Serial.print(F("N"));
#endif
      Serial.print(millis());
      Serial.println(F("-"));
    }

  public:
    void add(String text) {
      buffer += text;
    }

    void addLine(String text) {
      buffer += text;
      buffer += "\n";
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
      startText();


      Serial.print(buffer);
      clear();
    }

    void println(const String &text = "") {
      // E1200:
      if (!enableLog) return;
      if (text != "") {
        buffer = "";
        addLine(text);
      }

#if defined(ESP32)
      Serial.print(F("E"));
#else
      Serial.print(F("N"));
#endif
      Serial.print(millis());
      Serial.print(F("-"));


      Serial.println(buffer);
      clear();
    }

    void enable(const bool state) {
      enableLog = state;
    }

    void enableRtdbSyncLog(const bool state) {
      rtdbSyncLog = state;
    }

    void enableBTLog(const bool state) {
      BTlog = state;
    }
};

Logger slog;