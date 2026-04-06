#pragma once
#include <Arduino.h>

const byte esp32_restart        = 0xAA;
const byte esp32_memSave        = 0xAB;
const byte esp32_memGetAll      = 0xAC;
const byte esp32_memGet         = 0xAD;
const byte esp32_memDelete      = 0xAE;
const byte esp32_memDeleteAll   = 0xAF;

const byte esp32_wifiConnect    = 0xBA;
const byte esp32_wifiDisconnect = 0xBC;
const byte esp32_wifiStatus     = 0xBD;
const byte esp32_wifiScan       = 0xBF;

const byte esp32_wifiGetConfig  = 0xCA;
const byte esp32_wifiSetConfig  = 0xCB;
const byte esp32_wifiGetIP      = 0xCD;
const byte esp32_wifiGetMAC     = 0xCE;

const byte esp32_getInfo        = 0xCF;
const byte esp32_getUptime      = 0xDA;
const byte esp32_getFreeHeap    = 0xDB;
const byte esp32_getChipID      = 0xDC;
const byte esp32_getFlashSize   = 0xDD;
const byte esp32_minFreeHeap    = 0xDE;
const byte esp32_getTemperature = 0xDF;

const byte esp32_enableLog      = 0xEA;
const byte esp32_sendUart       = 0xEB;