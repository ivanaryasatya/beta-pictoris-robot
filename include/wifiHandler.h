#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include "secretData.h"

class WifiHandler {
private:
    String ssid;
    String password;

public:
    WifiHandler() {
        ssid = secretData.WIFI_SSID;
        password = secretData.WIFI_PASSWORD;
    }

    void connect() {
        if (isConnected()) {
            Serial.println("WiFi sudah terhubung.");
            return;
        }

        WiFi.begin(ssid.c_str(), password.c_str());
        Serial.print("Menghubungkan ke WiFi");
        int attempts = 0;
        
        while (WiFi.status() != WL_CONNECTED && attempts < 20) {
            Serial.print(".");
            delay(500);
            attempts++;
        }
        
        if (isConnected()) {
            Serial.println("\nWiFi terhubung.");
            Serial.print("IP Address: ");
            Serial.println(WiFi.localIP());
        } else {
            Serial.println("\nGagal terhubung ke WiFi.");
        }
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
            Serial.println("Mencoba menghubungkan kembali...");
            connect();
        }
    }

    void scanNetworks() {
        Serial.println("Memulai scan WiFi...");
        int n = WiFi.scanNetworks();
        if (n == 0) {
            Serial.println("Tidak ada jaringan WiFi yang ditemukan.");
        } else {
            Serial.print(n);
            Serial.println(" jaringan ditemukan:");
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
        Serial.println("Kredensial WiFi diperbarui.");
        Serial.print("SSID Baru: ");
        Serial.println(ssid);
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
            Serial.println("Access Point berhasil dimulai.");
            Serial.print("SSID AP: ");
            Serial.println(apSsid);
            Serial.print("IP AP: ");
            Serial.println(WiFi.softAPIP());
        } else {
            Serial.println("Gagal memulai Access Point.");
        }
        return result;
    }

    bool configAP(IPAddress local_ip, IPAddress gateway, IPAddress subnet) {
        bool result = WiFi.softAPConfig(local_ip, gateway, subnet);
        if(result) {
            Serial.println("Konfigurasi IP AP berhasil diubah.");
        } else {
            Serial.println("Gagal mengubah konfigurasi IP AP.");
        }
        return result;
    }

    void stopAP() {
        WiFi.softAPdisconnect(true);
        Serial.println("Access Point dihentikan.");
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