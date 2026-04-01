#pragma once
#include <Arduino.h>
using cuint = const uint8_t;

struct Pins {
    struct L298N_r {
        cuint IN1 = 0;
        cuint IN2 = 4;
        cuint IN3 = 26;
        cuint IN4 = 27;
        cuint ENA = 2;
        cuint ENB = 5;
    } wheelDriver_r;

    struct L298N_l {
        cuint IN1 = 12;
        cuint IN2 = 13;
        cuint IN3 = 18;
        cuint IN4 = 19;
        cuint ENA = 14;
        cuint ENB = 23;
    } wheelDriver_l;

    struct LedDriver {
        struct bumper {
            cuint left = 6;
            cuint right = 4;
        } bumper;
        cuint laser = 8;
    } led;

    struct TCRT5000 {
        cuint catcher = 19;
        cuint dropPoint = 18;
        cuint shoot = 17;
        cuint speedMotorRight = 16;
        cuint speedMotorLeft = 15;
    } irSensor;

    struct MPU6050 {
        cuint sda = 21;
        cuint scl = 22;
    } accelerometer;

    struct GY271 {
        cuint sda = 21;
        cuint scl = 22;
    } compassSensor;

    struct HALL49E {
        cuint right = 20;
        cuint left = 21;
    } hallSensor;

    struct esp32Serial {
        cuint rx = 16;
        cuint tx = 17;
    } esp32Serial;

    struct NanoSerial {
        cuint rx = 0;
        cuint tx = 1;
    } nanoSerial;

    struct IRF520 {
        struct flyWheel {
            cuint left = 10;
            cuint right = 9;
        } flyWheel;
        cuint fan = 5;
    } flywheelDriver;

    struct servo {
        cuint catcher = 0;
        cuint arm = 0;
        cuint megazine = 0;
        cuint pusher = 0;
        cuint barrelPuller = 0;
    } servo;

    struct NRF24l01 {
        cuint MISO = 0;
        cuint MOSI = 0;
        cuint SCK = 0;  
        cuint CSN = 0;
        cuint CE = 0;
    } radio24;

    struct PowerMonitor {
        cuint voltageSensor = 0;
        cuint currentSensor = 0;
    } powerMonitor;

    struct HCSR04 {
        cuint trig = 0;
        cuint echo = 0;
    } ultrasonicSensor;

    struct buzzer {
        cuint buzzerPin = 0;
    } buzzer;

};

Pins pins;

