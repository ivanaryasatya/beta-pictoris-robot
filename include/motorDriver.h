#pragma once
#include <Arduino.h>

class MotorDriver {
private:
    uint8_t _enA;
    uint8_t _in1;
    uint8_t _in2;
    uint8_t _in3;
    uint8_t _in4;
    uint8_t _enB;

public:
    /**
     * @brief Inisialisasi pin driver L298N.
     * @param enA Pin ENA (PWM Motor A)
     * @param in1 Pin IN1 (Arah 1 Motor A)
     * @param in2 Pin IN2 (Arah 2 Motor A)
     * @param in3 Pin IN3 (Arah 1 Motor B)
     * @param in4 Pin IN4 (Arah 2 Motor B)
     * @param enB Pin ENB (PWM Motor B)
     */
    MotorDriver(uint8_t enA, uint8_t in1, uint8_t in2, uint8_t in3, uint8_t in4, uint8_t enB) {
        _enA = enA;
        _in1 = in1;
        _in2 = in2;
        _in3 = in3;
        _in4 = in4;
        _enB = enB;
    }

    /**
     * @brief Setup pin mode (panggil fungsi ini di setup() Arduino).
     */
    void begin() {
        pinMode(_enA, OUTPUT);
        pinMode(_in1, OUTPUT);
        pinMode(_in2, OUTPUT);
        pinMode(_in3, OUTPUT);
        pinMode(_in4, OUTPUT);
        pinMode(_enB, OUTPUT);
        
        // Memastikan motor dalam keadaan mati saat awal
        stop();
    }

    /**
     * @brief Kontrol Motor A (kiri).
     * @param speed Kecepatan motor, nilai berkisar -255 hingga 255.
     *              (Positif = maju, Negatif = mundur, 0 = berhenti)
     */
    void setMotorA(int speed) {
        // Membatasi nilai agar tidak di luar rentang -255 sampai 255
        speed = constrain(speed, -255, 255);
        
        if (speed > 0) {
            // Maju
            digitalWrite(_in1, HIGH);
            digitalWrite(_in2, LOW);
            analogWrite(_enA, speed);
        } else if (speed < 0) {
            // Mundur
            digitalWrite(_in1, LOW);
            digitalWrite(_in2, HIGH);
            analogWrite(_enA, -speed); // Konversi ke positif untuk PWM
        } else {
            // Berhenti
            digitalWrite(_in1, LOW);
            digitalWrite(_in2, LOW);
            analogWrite(_enA, 0);
        }
    }

    /**
     * @brief Kontrol Motor B (kanan).
     * @param speed Kecepatan motor, nilai berkisar -255 hingga 255.
     *              (Positif = maju, Negatif = mundur, 0 = berhenti)
     */
    void setMotorB(int speed) {
        // Membatasi nilai agar tidak di luar rentang -255 sampai 255
        speed = constrain(speed, -255, 255);
        
        if (speed > 0) {
            // Maju
            digitalWrite(_in3, HIGH);
            digitalWrite(_in4, LOW);
            analogWrite(_enB, speed);
        } else if (speed < 0) {
            // Mundur
            digitalWrite(_in3, LOW);
            digitalWrite(_in4, HIGH);
            analogWrite(_enB, -speed); // Konversi ke positif untuk PWM
        } else {
            // Berhenti
            digitalWrite(_in3, LOW);
            digitalWrite(_in4, LOW);
            analogWrite(_enB, 0);
        }
    }

    /**
     * @brief Mengontrol kedua motor sekaligus.
     * @param speedA Kecepatan Motor A (-255 s/d 255)
     * @param speedB Kecepatan Motor B (-255 s/d 255)
     */
    void setMotors(int speedA, int speedB) {
        setMotorA(speedA);
        setMotorB(speedB);
    }

    /**
     * @brief Menghentikan kedua motor.
     */
    void stop() {
        setMotorA(0);
        setMotorB(0);
    }
};