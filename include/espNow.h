#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>

class ESPNowTools {
private:
    static void (*userReceiveCallback)(const uint8_t*, const uint8_t*, int);
    // Callback internal receive
    static void onReceive(const uint8_t *mac, const uint8_t *incomingData, int len) {
        if (userReceiveCallback != nullptr) {
            userReceiveCallback(mac, incomingData, len);
        }
    }
    // Callback status kirim
    static void onSend(const uint8_t *mac_addr, esp_now_send_status_t status) {
        Serial.print("ESPNow Send Status: ");
        if (status == ESP_NOW_SEND_SUCCESS)
            Serial.println("Success");
        else
            Serial.println("Fail");
    }

public:

    // ==========================
    // INIT
    // ==========================
    static bool begin(int channel = 1) {
        WiFi.mode(WIFI_STA);
        WiFi.disconnect();
        if (esp_now_init() != ESP_OK) {
            Serial.println("ESPNow init failed");
            return false;
        }
        esp_now_register_send_cb(onSend);
        esp_now_register_recv_cb(onReceive);
        esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
        Serial.println("ESPNow Ready");
        return true;
    }
    // ==========================
    // PEER MANAGEMENT
    // ==========================
    static bool addPeer(uint8_t *peerMAC, int channel = 1) {
        esp_now_peer_info_t peerInfo = {};
        memcpy(peerInfo.peer_addr, peerMAC, 6);
        peerInfo.channel = channel;
        peerInfo.encrypt = false;
        if (esp_now_add_peer(&peerInfo) != ESP_OK) {
            Serial.println("Peer add failed");
            return false;
        }
        Serial.println("Peer added");
        return true;
    }
    static void removePeer(uint8_t *peerMAC) {
        esp_now_del_peer(peerMAC);
        Serial.println("Peer removed");
    }
    // ==========================
    // SEND DATA
    // ==========================
    template<typename T>
    static bool send(uint8_t *peerMAC, T &data) {
        esp_err_t result = esp_now_send(peerMAC, (uint8_t*) &data, sizeof(T));
        return (result == ESP_OK);
    }
    static bool sendBytes(uint8_t *peerMAC, uint8_t *data, size_t len) {
        esp_err_t result = esp_now_send(peerMAC, data, len);
        return (result == ESP_OK);
    }

    static bool sendString(uint8_t *peerMAC, String msg) {
        return sendBytes(peerMAC, (uint8_t*)msg.c_str(), msg.length()+1);
    }
    // ==========================
    // RECEIVE CALLBACK
    // ==========================
    static void onData(void (*callback)(const uint8_t*, const uint8_t*, int)) {
        userReceiveCallback = callback;
    }
    // ==========================
    // UTILITIES
    // ==========================
    static void printMAC(const uint8_t *mac) {
        for (int i = 0; i < 6; i++) {
            Serial.print(mac[i], HEX);
            if (i < 5) Serial.print(":");
        }
        Serial.println();
    }
    static void getMyMAC() {
        uint8_t mac[6];
        WiFi.macAddress(mac);
        Serial.print("My MAC: ");
        printMAC(mac);
    }
};

void (*ESPNowTools::userReceiveCallback)(const uint8_t*, const uint8_t*, int) = nullptr;


/* transmitter example ===============

#include "ESPNowTools.h"

uint8_t receiverMAC[] = {0x24,0x6F,0x28,0xAA,0xBB,0xCC};

struct DataPacket {

  int value;
  float temperature;

};

DataPacket data;

void setup() {

  Serial.begin(115200);

  ESPNowTools::begin(1);

  ESPNowTools::addPeer(receiverMAC);

}

void loop() {

  data.value++;
  data.temperature = random(200,300)/10.0;

  ESPNowTools::send(receiverMAC, data);

  delay(1000);

}



receiver example ===============

#include "ESPNowTools.h"

struct DataPacket {

  int value;
  float temperature;

};

void onReceive(const uint8_t *mac, const uint8_t *data, int len) {

  DataPacket packet;

  memcpy(&packet, data, sizeof(packet));

  Serial.print("From: ");
  ESPNowTools::printMAC(mac);

  Serial.print("Value: ");
  Serial.println(packet.value);

  Serial.print("Temp: ");
  Serial.println(packet.temperature);

}

void setup() {

  Serial.begin(115200);

  ESPNowTools::begin();

  ESPNowTools::onData(onReceive);

}

void loop() {}

*/