#pragma once
#include <Arduino.h>
using cbyte = const byte;

struct Pins {
    struct Uart {
        cbyte rx = 3;
        cbyte tx = 1;
    } esp32Serial;
    struct L298N_r {
        cbyte IN1 = 0;
        cbyte IN2 = 4;
        cbyte IN3 = 0;
        cbyte IN4 = 0;
        cbyte ENA = 2;
        cbyte ENB = 5;
    } wheelDriver_r;

    struct L298N_l {
        cbyte IN1 = 0;
        cbyte IN2 = 0;
        cbyte IN3 = 18;
        cbyte IN4 = 19;
        cbyte ENA = 0;
        cbyte ENB = 23;
    } wheelDriver_l;

    struct LedDriver {
        struct bumper {
            cbyte left = 6;
            cbyte right = 4;
        } bumper;
        cbyte laser = 8;
    } led;

    struct TCRT5000 {
        cbyte catcher = 19;
        cbyte dropPoint = 18;
        cbyte shoot = 17;
        cbyte speedMotorRight = 16;
        cbyte speedMotorLeft = 15;
    } irSensor;

    struct MPU6050 {
        cbyte sda = 21;
        cbyte scl = 22;
    } accelerometer;

    struct GY271 {
        cbyte sda = 21;
        cbyte scl = 22;
    } compassSensor;

    struct HALL49E {
        cbyte right = 20;
        cbyte left = 21;
    } hallSensor;

    struct Esp32Uart {
        cbyte rx = 16;
        cbyte tx = 17;
    } esp32Uart;

    struct NanoSerial {
        cbyte rx = 0;
        cbyte tx = 1;
    } nanoSerial;

    struct IRF520 {
        struct flyWheel {
            cbyte left = 10;
            cbyte right = 9;
        } flyWheel;
        cbyte fan = 5;
    } flywheelDriver;

    struct servo {
        cbyte catcher = 0;
        cbyte arm = 0;
        cbyte megazine = 0;
        cbyte pusher = 0;
        cbyte barrelPuller = 0;
    } servo;

    struct NRF24l01 {
        cbyte MISO = 0;
        cbyte MOSI = 0;
        cbyte SCK = 0;  
        cbyte CSN = 0;
        cbyte CE = 0;
    } radio24;

    struct PowerMonitor {
        cbyte voltageSensor = 0;
        cbyte currentSensor = 0;
    } powerMonitor;

    struct HCSR04 {
        cbyte trig = 0;
        cbyte echo = 0;
    } ultrasonicSensor;

    struct buzzer {
        cbyte buzzerPin = 0;
    } buzzer;

} pins;

