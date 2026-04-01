#pragma once
#include <Arduino.h>
using cchar = const char;
class EEPROMPointer {
  public:
    cchar wifiSSID ='a';
    cchar wifiPassword ='b';
    cchar apiKey ='c';
    cchar userEmail ='d';
    cchar userPassword ='e';
    cchar databaseURL ='f';
    cchar logState ='L';
};
EEPROMPointer epmPtr;