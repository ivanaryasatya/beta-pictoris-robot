#pragma once

#include <Arduino.h>
#include "serialLogger.h"
using cbyte = const byte;
cbyte UART_MAX_DATA = 32;
cbyte v = 'a';

// format: header | cmd | id | len | data | checksum | footer
// size:   1      | 1   | 1  | 1   | 32   | 1        | 1

typedef void (*CommandHandler)(byte cmd, byte id, byte *data, byte len);
using cbyte = const byte;

class UARTProtocol {
public:
  UARTProtocol(Stream &serial) : _serial(serial) {}
  void begin(CommandHandler handler) {
    _handler = handler;
  }
  

struct MapId {
  cbyte UART_HEADER        = 0xAA;
  cbyte UART_FOOTER        = 0x55;

  cbyte RESTART            = 0x02;
  cbyte USER_CMD           = 0x03;
  cbyte PING               = 0x04;
  cbyte PONG               = 0x05;
  cbyte TIME               = 0x06;
  cbyte EMERGENCY_MODE      = 0x07;
  cbyte SET_DEFAULT        = 0x08;
  cbyte VALUE              = 0x09;

  struct Buzzer{
    cbyte TONE = 0xD2;
    cbyte MELODY = 0xFA;
  } buzzer;
  
  struct DHT11 {
    cbyte TEMPERATURE      = 0x0A;
    cbyte HUMIDITY         = 0x0B;
  } temSensor;

  struct Ultrasonic {
    cbyte DISTANCE         = 0x0C;
    cbyte SET_THRESHOLD    = 0x0D;
  } ultrasonic;

  struct HallSensors {
    cbyte RIGHT            = 0x0E;
    cbyte LEFT             = 0x0F;
  } hall;

  struct IrSensors {
    cbyte CATCHER          = 0x10;
    cbyte DROP_POINT       = 0x11;
    cbyte SHOOT            = 0x12;
    cbyte SPEED_MOTOR_RIGHT= 0x13;
    cbyte SPEED_MOTOR_LEFT = 0x14;
  } irSensor;

  struct Servos {
    struct CATCHER {
      cbyte ANGLE          = 0x15;
      cbyte SPEED          = 0x16;
    } catcher;

    struct ARM {
      cbyte ANGLE          = 0x17;
      cbyte SPEED          = 0x18;
    } arm;

    struct MEGAZINE {
      cbyte ROTATE_R       = 0x19;
      cbyte ROTATE_L       = 0x1A;
      cbyte STOP           = 0x1B;
    } megazine;

    struct SHOOTER {
      cbyte ANGLE          = 0x1C;
      cbyte SPEED          = 0x1D;
    } shooter;

    struct BARREL {
      cbyte UP             = 0x1E;
      cbyte DOWN           = 0x1F;
    } barrel;
  } servo;

  struct Motors {
    struct Right {
      cbyte SPEED          = 0x20;
    } right;

    struct Left {
      cbyte SPEED          = 0x21;
    } left;

    struct Fan {
      cbyte SPEED          = 0x22;
    } fan;
  } motor;

  struct LedDriver {
    struct bumper {
      cbyte LEFT           = 0x30;
      cbyte RIGHT          = 0x31;
    } bumper;
    cbyte LASER            = 0x32;
  } led;

} mapId;

  void send(byte cmd, byte len, byte *data) {

    byte checksum = calcChecksum(cmd, _packetID, len, data);

    _serial.write(mapId.UART_HEADER);
    _serial.write(cmd);
    _serial.write(_packetID);
    _serial.write(len);

    for (byte i = 0; i < len; i++) {
      _serial.write(data[i]);
    }

    _serial.write(checksum);
    _serial.write(mapId.UART_FOOTER);

    _lastID = _packetID;
    _packetID++;
    
    // --- LOGGING DATA UART ---
    slog.add(F("UART sent CMD: 0x"));
    if (cmd < 0x10) slog.add(F("0")); // Tambahkan leading zero jika nilai hex di bawah 10 (contoh: 0x0A)
    slog.add(String(cmd, HEX));
    
    slog.add(F(" | DATA: "));
    if (len > 0 && data != NULL) {
      for (byte i = 0; i < len; i++) {
        slog.add(F("0x"));
        if (data[i] < 0x10) slog.add(F("0"));
        slog.add(String(data[i], HEX));
        slog.add(F(" "));
      }
    } else {
      slog.add(F("None"));
    }
    slog.println();
    // -------------------------
  }

  void update() {
    while (_serial.available()) {
      parse(_serial.read());
    }
  }

private:
  Stream &_serial;
  CommandHandler _handler;

  byte _buffer[UART_MAX_DATA];
  byte _index = 0, _len = 0, _cmd, _id, _checksum;

  byte _packetID = 0;
  byte _lastID = 0;

  enum State {
    WAIT_HEADER,
    READ_CMD,
    READ_ID,
    READ_LEN,
    READ_DATA,
    READ_CHECKSUM,
    WAIT_FOOTER
  };

  State _state = WAIT_HEADER;

  byte calcChecksum(byte cmd, byte id, byte len, byte *data) {
    byte sum = cmd ^ id ^ len;
    for (byte i = 0; i < len; i++) sum ^= data[i];
    return sum;
  }

  void parse(byte b) {

    switch (_state) {

      case WAIT_HEADER:
        if (b == mapId.UART_HEADER) _state = READ_CMD;
        break;

      case READ_CMD:
        _cmd = b;
        _state = READ_ID;
        break;

      case READ_ID:
        _id = b;
        _state = READ_LEN;
        break;

      case READ_LEN:
        _len = b;
        _index = 0;
        _state = (_len > 0) ? READ_DATA : READ_CHECKSUM;
        break;

      case READ_DATA:
        _buffer[_index++] = b;
        if (_index >= _len) _state = READ_CHECKSUM;
        break;

      case READ_CHECKSUM:
        _checksum = b;
        _state = WAIT_FOOTER;
        break;

      case WAIT_FOOTER:
        if (b == mapId.UART_FOOTER) {
          byte calc = calcChecksum(_cmd, _id, _len, _buffer);

          if (calc == _checksum && _handler) {
            _handler(_cmd, _id, _buffer, _len);
          }
        }
        _state = WAIT_HEADER;
        break;
    }
  }
};