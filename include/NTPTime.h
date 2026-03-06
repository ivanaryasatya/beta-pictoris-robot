#pragma once

#include <WiFi.h>
#include "time.h"

class NTPClock {
  private:
    const char* ntpServer;
    long gmtOffset_sec;
    int daylightOffset_sec;

    struct tm timeinfo;

  public:
    // Constructor default
    NTPClock(const char* server = "pool.ntp.org", long gmtOffset = 0, int daylightOffset = 3600) {
      ntpServer = server;
      gmtOffset_sec = gmtOffset;
      daylightOffset_sec = daylightOffset;
    }

    // Inisialisasi NTP
    void init() {
      configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
      updateTime();
    }

    // Update waktu sekarang
    bool updateTime() {
      if (!getLocalTime(&timeinfo)) {
        return false;
      }
      return true;
    }

    // Getter untuk setiap komponen waktu
    int getHour24()    { return timeinfo.tm_hour; }
    int getHour12()    { return timeinfo.tm_hour % 12 == 0 ? 12 : timeinfo.tm_hour % 12; }
    int getMinute()    { return timeinfo.tm_min; }
    int getSecond()    { return timeinfo.tm_sec; }
    int getDay()       { return timeinfo.tm_mday; }
    int getMonth()     { return timeinfo.tm_mon + 1; } // tm_mon 0-11
    int getYear()      { return timeinfo.tm_year + 1900; } // tm_year sejak 1900
    int getWeekDay()   { return timeinfo.tm_wday; } // 0=Sunday
    int getDayOfYear() { return timeinfo.tm_yday; } // 0-365

    // Format string waktu
    String getTimeString() {
      char buf[20];
      strftime(buf, sizeof(buf), "%H:%M:%S", &timeinfo);
      return String(buf);
    }

    String getDateString() {
      char buf[20];
      strftime(buf, sizeof(buf), "%Y-%m-%d", &timeinfo);
      return String(buf);
    }

    String getDateTimeString() {
      char buf[30];
      strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
      return String(buf);
    }

    String getWeekDayName() {
      char buf[10];
      strftime(buf, sizeof(buf), "%A", &timeinfo);
      return String(buf);
    }

    String getMonthName() {
      char buf[10];
      strftime(buf, sizeof(buf), "%B", &timeinfo);
      return String(buf);
    }
};







// #include <WiFi.h>
// #include "NTPTime.h"

// const char* ssid = "YOUR_WIFI";
// const char* password = "YOUR_PASSWORD";

// NTPClock clock("pool.ntp.org", 7*3600, 3600); // GMT+7

// void setup() {
//   Serial.begin(115200);
//   WiFi.begin(ssid, password);
//   while (WiFi.status() != WL_CONNECTED) {
//     delay(500);
//   }

//   clock.init();
// }

// void loop() {
//   if (clock.updateTime()) {
//     Serial.print("Jam: "); Serial.println(clock.getHour24());
//     Serial.print("Menit: "); Serial.println(clock.getMinute());
//     Serial.print("Detik: "); Serial.println(clock.getSecond());
//     Serial.print("Tanggal: "); Serial.println(clock.getDateString());
//     Serial.print("Hari: "); Serial.println(clock.getWeekDayName());
//   } else {
//     Serial.println("Gagal mendapatkan waktu");
//   }

//   delay(1000);
// }