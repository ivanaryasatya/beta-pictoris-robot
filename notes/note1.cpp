#include <Arduino.h>
#include "uartProtocol.h"

// Menggunakan Serial2 untuk ESP32 (contoh PIN RX=16, TX=17)
UARTProtocol myUart(Serial2);

// Callback / Handler saat instruksi atau pesan diterima
void onCommandReceived(byte cmd, byte id, byte *data, byte len) {
    // Misalkan cmd 0x01 adalah "Terima Teks"
    if (cmd == 0x01) { 
        // Karena array byte yang diterima belum tentu memiliki null-terminator (\0),
        // kita perlu mengubahnya menjadi String dengan aman
        char buffer[UART_MAX_DATA + 1]; // +1 untuk batas akhir teks (null terminator)
        memcpy(buffer, data, len);      // salin data ke buffer
        buffer[len] = '\0';             // tambahkan null terminator
        
        Serial.print("Pesan (ID Paket ");
        Serial.print(id);
        Serial.print(") diterima: ");
        Serial.println(buffer);
    }
}

void setup() {
    Serial.begin(115200);   // Serial monitor
    Serial2.begin(9600);    // Inisiasi serial rute UART Protocol
    
    // Hubungkan instance myUart dengan fungsi Handler Anda
    myUart.begin(onCommandReceived); 
}

void loop() {
    // WAJIB diletakkan di loop agar paket terus di-decode
    myUart.update(); 
    
    // --------- CONTOH MENGIRIM DATA SETIAP 2 DETIK ---------
    static unsigned long lastSend = 0;
    if (millis() - lastSend > 2000) {
        lastSend = millis();

        // -----------------------------------------------------
        // CARA 1: Menggunakan objek String (Arduino)
        // -----------------------------------------------------
        String pesan1 = "Hello World!";
        byte len1 = pesan1.length(); 
        
        // Batasi maksimal 16 byte karakter (sesuai config di uartProtocol.h)
        if (len1 > UART_MAX_DATA) len1 = UART_MAX_DATA; 
        
        // Parameter send: (cmd, panjang data, pointer array of byte)
        myUart.send(0x01, len1, (byte*)pesan1.c_str());
        // -----------------------------------------------------


        // -----------------------------------------------------
        // CARA 2: Menggunakan C-String (const char*)
        // -----------------------------------------------------
        const char* pesan2 = "Testing 123";
        byte len2 = strlen(pesan2);
        
        if (len2 > UART_MAX_DATA) len2 = UART_MAX_DATA;
        
        // Kirim dengan Command ID yang berbeda misal: 0x02
        myUart.send(0x02, len2, (byte*)pesan2);
        // -----------------------------------------------------

        Serial.println("Pesan barusan dikirim!");
    }
}
