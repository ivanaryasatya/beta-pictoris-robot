#pragma once

#include <Arduino.h>

#define UART_HEADER  0xAA
#define UART_FOOTER  0x55
#define UART_MAX_DATA 32

// format: header | cmd | id | len | data | checksum | footer
// size:   1      | 1   | 1  | 1   | 32   | 1        | 1

typedef void (*CommandHandler)(byte cmd, byte id, byte *data, byte len);
using cbyte = const byte;

class UARTProtocol {
public:
  UARTProtocol(Stream &serial) : _serial(serial) {}

  // =============================
  // INIT
  // =============================
  void begin(CommandHandler handler) {
    _handler = handler;
  }

  struct MapId {
    cbyte CMD_GET_NUMBER = 0x10;
    cbyte CMD_NUMBER     = 0x11;
    cbyte RESTART        = 0x12;
    cbyte USER_CMD       = 0x13;
    cbyte PING           = 0x14;
    cbyte PONG           = 0x15;
  } mapId;

  // =============================
  // SEND
  // =============================
  void send(byte cmd, byte len, byte *data) {

    byte checksum = calcChecksum(cmd, _packetID, len, data);

    _serial.write(UART_HEADER);
    _serial.write(cmd);
    _serial.write(_packetID);
    _serial.write(len);

    for (byte i = 0; i < len; i++) {
      _serial.write(data[i]);
    }

    _serial.write(checksum);
    _serial.write(UART_FOOTER);

    _lastID = _packetID;
    _packetID++;
  }

  // =============================
  // UPDATE (WAJIB DI LOOP)
  // =============================
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

  // =============================
  // CHECKSUM
  // =============================
  byte calcChecksum(byte cmd, byte id, byte len, byte *data) {
    byte sum = cmd ^ id ^ len;
    for (byte i = 0; i < len; i++) sum ^= data[i];
    return sum;
  }

  // =============================
  // PARSER
  // =============================
  void parse(byte b) {

    switch (_state) {

      case WAIT_HEADER:
        if (b == UART_HEADER) _state = READ_CMD;
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
        if (b == UART_FOOTER) {
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