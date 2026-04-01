#pragma once

#include <Arduino.h>
#include <SPI.h>
#include <RF24.h>

/**
 * ⚠️ PERINGATAN (WARNING): 
 * Penggunaan Jammer (pengacau sinyal) dilarang di berbagai negara, 
 * termasuk di Indonesia oleh Kominfo jika digunakan untuk mengganggu 
 * hak cipta/fasilitas umum. Kode ini disediakan HANYA UNTUK TUJUAN 
 * EDUKASI / PENGUJIAN KEAMANAN di lingkungan tertutup.
 */

// Konfigurasi pin default untuk ESP32 (DevKitC v4)
// Anda dapat mengubah pin CE dan CSN sesuai wiring fisik Anda
#ifndef CE_PIN
#define CE_PIN 4
#endif

#ifndef CSN_PIN
#define CSN_PIN 5
#endif

class RadioJammer {
private:
    RF24 radio;
    uint8_t channels[80]; // Bluetooth menggunakan 79 channel (2402 - 2480 MHz)
    int num_channels;
    bool isOutputCarrier;

public:
    // Constructor dengan parameter pin CE dan CSN
    RadioJammer(uint8_t ce = CE_PIN, uint8_t csn = CSN_PIN) : radio(ce, csn) {
        num_channels = 0;
        isOutputCarrier = false;
        
        // Mengisi array dengan channel Bluetooth (2..80) yang merepresentasikan 
        // 2402 MHz sampai 2480 MHz
        for (int i = 2; i <= 80; i++) {
            channels[num_channels++] = i;
        }
    }

    // Inisialisasi NRF24L01 untuk mode pengacau (Jammer)
    bool begin(bool useCarrier = false) {
        isOutputCarrier = useCarrier;
        
        if (!radio.begin()) {
            Serial.println("Error: NRF24L01 tidak terdeteksi!");
            return false;
        }

        // Konfigurasi NRF24L01 untuk jamming
        radio.setAutoAck(false);           // Matikan Auto-ACK (No acknowledgment needed)
        radio.stopListening();             // NRF24 sebagai Transmitter
        radio.setRetries(0, 0);            // Matikan Auto-Retry
        radio.setPALevel(RF24_PA_MAX);     // Power maksimal untuk sinyal terkuat (butuh power supply kuat)
        radio.setDataRate(RF24_2MBPS);     // Data rate tercepat (2 Mbps) untuk memenuhi bandwith
        radio.setCRCLength(RF24_CRC_DISABLED); // Matikan CRC agar bisa ngirim raw noise lebih cepat

        Serial.println("Jammer NRF24L01 Berhasil Diinisialisasi.");
        return true;
    }

    // Fungsi utama untuk melakukan jamming / sweep
    // Panggil fungsi ini di dalam void loop()
    void sweep() {
        if (isOutputCarrier) {
            // Metode 1: Continuous Carrier Wave (butuh library RF24 versi terbaru)
            // Memancarkan gelombang konstan secara berurutan di setiap channel
            for (int i = 0; i < num_channels; i++) {
                radio.setChannel(channels[i]);
                radio.startConstCarrier(RF24_PA_MAX, channels[i]);
                delayMicroseconds(500); // Pancarkan selama 0.5 milidetik di channel ini
                radio.stopConstCarrier();
            }
        } else {
            // Metode 2: Data Flooding (Broadcasting Dummy Data)
            // Mengirim data sampah secara terus-menerus gila-gilaan
            const char dummyPayload[32] = "1010101010101010101010101010101"; // 31 byte + \0
            
            for (int i = 0; i < num_channels; i++) {
                radio.setChannel(channels[i]);
                // Transmit data secara cepat
                radio.write(&dummyPayload, sizeof(dummyPayload));
            }
        }
    }

    // Method tambahan jika hanya ingin mem-block channel spesifik (misal WiFi/BLE)
    void jamSpecificChannel(uint8_t channel) {
        radio.setChannel(channel);
        const char dummyPayload[32] = "1010101010101010101010101010101";
        radio.writeFast(&dummyPayload, sizeof(dummyPayload));
    }
};
