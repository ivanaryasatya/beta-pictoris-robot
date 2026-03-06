#pragma once
#include <Arduino.h>

class CommandParser {
  private:
    String _buffer;

  public:
    CommandParser() {}
    void input(String data) {
      _buffer += data;
    }

    bool parse(String &outCommand, String &outValue) {
      int separatorIndex = _buffer.indexOf(';');
      
      if (separatorIndex == -1) {
      }

      String rawCommand = _buffer.substring(0, separatorIndex);
      
      _buffer = _buffer.substring(separatorIndex + 1);
      int equalIndex = rawCommand.indexOf('=');

      if (equalIndex != -1) {
        outCommand = rawCommand.substring(0, equalIndex);
        outValue = rawCommand.substring(equalIndex + 1);
      } else {
        outCommand = rawCommand;
        outValue = "";
      }

      return true;
    }

    void clear() {
      _buffer = "";
    }
};

// #include <Arduino.h>
// #include "commandParser.h"

// CommandParser parser;

// void setup() {
//   Serial.begin(115200);

//   // Contoh input berantai yang panjang
//   String inputSistem = "9O=255;fQ=0;p2=12;";
  
//   // Masukkan string tersebut ke parser
//   parser.input(inputSistem);
// }

// void loop() {
//   String cmd, val;
  
//   // Ambil setiap potongan command yang ada di memori parser
//   // Looping ini akan terus berjalan selama ada format command;
//   while(parser.parse(cmd, val)) {
//     Serial.print("Command: ");
//     Serial.print(cmd);
//     Serial.print(" | Value: ");
//     Serial.println(val);

//     // Di sini kamu bisa gunakan if/else untuk aksi pada servo/aktuator:
//     if (cmd == "9O") {
//       int pwmValue = val.toInt();
//       // contoh eksekusi analogWrite(pin, pwmValue);
//     } else if (cmd == "fQ") {
//       // dst...
//     }
//   }

//   // Jika suatu saat ada data baru dari Serial:
//   if (Serial.available()) {
//     String incoming = Serial.readStringUntil('\n');
//     parser.input(incoming);
//   }
// }
