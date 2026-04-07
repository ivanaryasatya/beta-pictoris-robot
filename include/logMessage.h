#pragma once
#include <Arduino.h>

struct LogMessage {
    const char* wifiHasConnected = "wifi is already connected";
    const char* wifiConnecting = "connecting to wifi";
    const char* wifiConnected = "wifi connected";
    const char* wifiNotConnected = "wifi not connected";
    const char* wifiConnectionFailed = "wifi connection failed";
    const char* retryingConnection = "retrying connection...";
    const char* wifiStartingScan = "starting wifi scan...";
    const char* wifiNoNetworksFound = "no wifi networks found";
    const char* wifiNetworksFound = "wifi networks found";
    const char* credentialsUpdated = "wifi credentials updated";
    const char* newSSID = "new SSID: ";
    const char* newPassword = "new Password: ";
    const char* accessPointStarted = "access point started";
    const char* ssidAp = "SSID AP: ";
    const char* ipAp = "IP AP: ";
    const char* accessPointFailed = "failed to start access point";
    const char* accessPointSuccessfullyConfigured = "access point IP configuration successfully changed";
    const char* accessPointConfigurationFailed = "failed to change access point IP configuration";
    const char* accessPointStopped = "access point stopped";
    const char* invalidCommandTarget = "invalid command target";
    const char* invalidCommand = "invalid command";
    const char* esp32Restarting = "ESP32 is restarting...";
    const char* commandValueLessThanExpected = "command value count is less than ";
    const char* wifiLocalIP = "wifi local IP: ";
    const char* eepromSaveSuccess = "successfully saved to EEPROM";
    const char* eepromSaveFailed = "failed to save to EEPROM";
    const char* esp32WillRestartIn = "ESP32 will restart in ";
    const char* seconds = " seconds";

} logMes;


const char s0[] PROGMEM = "OFF";
const char s1[] PROGMEM = "ON";
const char s2[] PROGMEM = "ERROR";

const char* const statusText[] PROGMEM = { s0, s1, s2 };

void printStatus(uint8_t status) {
    char buffer[10];

    const char* ptr = (const char*)pgm_read_ptr(&statusText[status]);
    strcpy_P(buffer, ptr);

    Serial.println(buffer);
}
