#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include "secretData.h"
#include "serialLogger.h"
#include "logMessage.h"

class WifiHandler {
private:
    String ssid;
    String password;
    byte attempts;

public:
    WifiHandler() {
        ssid = secretData.WIFI_SSID;
        password = secretData.WIFI_PASSWORD;
    }

    // 1 = connected, 0 = not connected, 2 = already connected
    byte connect() {
        if (isConnected()) {
            return 2;
        }

        WiFi.begin(ssid.c_str(), password.c_str());
        slog.println(logMes.wifiConnecting);
        
        while (WiFi.status() != WL_CONNECTED && attempts < 20) {
            slog.print(logMes.wifiConnecting);
            Serial.print(".");
            delay(500);
            attempts++;
        }
        
        if (isConnected()) {
            slog.addLine(logMes.wifiConnected);
            slog.add(logMes.wifiLocalIP);
            slog.addLine(WiFi.localIP().toString());
            slog.print();
        } else {
            slog.println(logMes.wifiConnectionFailed);
        }
        return isConnected() ? 1 : 0;
    }

    void setAttempts(const byte newAttempts) {
        attempts = newAttempts;
    }
    
    void disconnect() {
        WiFi.disconnect(true);
        Serial.println("WiFi terputus.");
    }

    bool isConnected() {
        return WiFi.status() == WL_CONNECTED;
    }

    String getIP() {
        if (isConnected()) {
            return WiFi.localIP().toString();
        }
        return "Not Connected";
    }

    String getMacAddress() {
        return WiFi.macAddress();
    }

    int32_t getRSSI() {
        if (isConnected()) {
            return WiFi.RSSI();
        }
        return 0;
    }

    void reconnect() {
        if (!isConnected()) {
            slog.println(logMes.retryingConnection);
            connect();
        }
    }

    void scanNetworks() {
        slog.println(logMes.wifiStartingScan);
        int n = WiFi.scanNetworks();
        if (n == 0) {
            slog.println(logMes.wifiNoNetworksFound);
        } else {
            slog.add(String(n));
            slog.addLine(logMes.wifiNetworksFound);
            for (int i = 0; i < n; ++i) {
                Serial.print(i + 1);
                Serial.print(": ");
                Serial.print(WiFi.SSID(i));
                Serial.print(" (");
                Serial.print(WiFi.RSSI(i));
                Serial.println(" dBm)");
                delay(10);
            }
        }
    }

    void setCredentials(String newSSID, String newPassword) {
        ssid = newSSID;
        password = newPassword;
        slog.addLine(logMes.credentialsUpdated);
        slog.add(logMes.newSSID);
        slog.addLine(ssid);
        slog.print();
    }


    wifi_mode_t getMode() {
        return WiFi.getMode();
    }

    // --- Access Point (AP) Tools ---

    bool startAP(String apSsid, String apPassword = "", int channel = 1, int hidden = 0, int max_connection = 4) {
        Serial.println("Memulai Access Point...");
        
        bool result = false;
        if(apPassword == "" || apPassword.length() == 0) {
            result = WiFi.softAP(apSsid.c_str(), NULL, channel, hidden, max_connection);
        } else {
            result = WiFi.softAP(apSsid.c_str(), apPassword.c_str(), channel, hidden, max_connection);
        }

        if(result) {
            slog.addLine(logMes.accessPointStarted);
            slog.add(logMes.ssidAp);
            slog.addLine(apSsid);
            slog.add(logMes.ipAp);
            slog.addLine(WiFi.softAPIP().toString());
            slog.print();
        } else {
            slog.println(logMes.accessPointFailed);
        }
        return result;
    }

    bool configAP(IPAddress local_ip, IPAddress gateway, IPAddress subnet) {
        bool result = WiFi.softAPConfig(local_ip, gateway, subnet);
        if(result) {
            slog.println(logMes.accessPointSuccessfullyConfigured);
        } else {
            slog.println(logMes.accessPointConfigurationFailed);
        }
        return result;
    }

    void stopAP() {
        WiFi.softAPdisconnect(true);
        slog.println(logMes.accessPointStopped);
    }

    String getAPIP() {
        return WiFi.softAPIP().toString();
    }

    String getAPMacAddress() {
        return WiFi.softAPmacAddress();
    }

    uint8_t getAPStationNum() {
        return WiFi.softAPgetStationNum();
    }

    String getAPSSID() {
        return WiFi.softAPSSID();
    }
};

WifiHandler wifi;