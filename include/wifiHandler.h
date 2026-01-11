#pragma once
#define ENABLE_USER_AUTH
#define ENABLE_DATABASE

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <FirebaseClient.h>
#include "secretData.h"

// Kredensial WiFi
String WIFI_SSID = secretData.getWifiSSID();
String WIFI_PASSWORD = secretData.getWifiPassword();
#define Web_API_KEY    secretData.getWebApiKey()
#define DATABASE_URL   secretData.getDatabaseUrl()
#define USER_EMAIL     secretData.getUserEmail()
#define USER_PASS      secretData.getUserPass()

// Pin dan path
#define LED_PIN     2           // Contoh: LED terhubung ke GPIO2
#define SENSOR_PATH "/sensorValue"
#define LED_PATH    "/ledStatus"

// Objek Firebase dan client
UserAuth user_auth(Web_API_KEY, USER_EMAIL, USER_PASS);
FirebaseApp app;
WiFiClientSecure ssl_client;
using AsyncClient = AsyncClientClass;
AsyncClient aClient(ssl_client);
RealtimeDatabase db;

// Variabel timer dan data
unsigned long lastSendTime = 0;
const unsigned long sendInterval = 10000; // 10 detik
int sensorValue = 0;

// Callback untuk menangani hasil Firebase
void processData(AsyncResult &aResult) {
  if (!aResult.isResult()) return;  // abaikan jika bukan hasil
  if (aResult.isError()) {
    Serial.printf("Firebase Error: %s\n", aResult.error().message().c_str());
    return;
  }
  if (aResult.available()) {
    String task = aResult.uid();
    String payload = aResult.c_str();
    // Cek apakah ini data LED yang diterima
    if (task == "RTDB_GetLED") {
      int ledState = payload.toInt();
      digitalWrite(LED_PIN, ledState);
      Serial.printf("LED diatur ke: %d\n", ledState);
    }
  }
}

void setup() {
  Serial.begin(115200);
  // Koneksi ke WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Menghubungkan ke WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println("\nWiFi terhubung.");

  // Konfigurasi SSL/TLS
  ssl_client.setInsecure();
  //ssl_client.setConnectionTimeout(1000);
  ssl_client.setHandshakeTimeout(5);

  // Inisialisasi Firebase
  initializeApp(aClient, app, getAuth(user_auth), processData, " authTask");
  app.getApp<RealtimeDatabase>(db);
  db.url(DATABASE_URL);

  pinMode(LED_PIN, OUTPUT);
}

void loop() {
  app.loop();  // Menjaga autentikasi dan koneksi tetap aktif
  if (app.ready()) {
    unsigned long currentTime = millis();
    if (currentTime - lastSendTime >= sendInterval) {
      lastSendTime = currentTime;
      // Kirim data sensor (dummy) ke Firebase
      String data = String(sensorValue++);
      db.set<String>(aClient, SENSOR_PATH, data, processData, "RTDB_SetSensor");
      Serial.println("Sensor terkirim: " + data);
      // Baca status LED dari Firebase
      db.get(aClient, LED_PATH, processData, false, "RTDB_GetLED");
    }
  }
}

