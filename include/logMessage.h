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
    const char* commandValueLessThanExpected = "command value count is less than";
    const char* wifiLocalIP = "wifi local IP: ";

} logMes;