#include <Arduino.h>
#include "uartProtocol.h"

UARTProtocol uart(Serial);

int counter = 1;

void(* resetFunc) (void) = 0;

void handleCommand(byte cmd, byte id, byte *data, byte len) {
}

void setup() {
  Serial.begin(9600);
  uart.begin(handleCommand);
}

void loop() {
  uart.update();
}
