#pragma once
#include <Arduino.h>

const byte buzzerPin = 5;
const byte shooterMotorEnPin = 6;
const byte shooterMotorPwm2Pin = 8;
const byte builtinLedPin = 2;

struct ServoPins {
    const byte ballPusher;
    const byte ballCollector;
    const byte barrelLifter;
    const byte ballCollectorClamper;
    const byte magazineRotation;
};
ServoPins servoPins = {
    18,  // ballPusher
    19,  // ballCollector
    21,  // barrelLifter
    22,  // ballCollectorClamper
    23   // magazineRotation
};



struct MotorPins {
    const byte en;
    const byte in1;
    const byte in2;
};

MotorPins motors[4] = {
//   en, in1, in2
    {32, 33, 25}, // Motor1 - front right
    {26, 27, 14}, // Motor2 - front left
    {12, 13, 15}, // Motor3 - back left
    {2, 4, 16}    // Motor4 - back right
};


struct IRSensorPins {
    const byte ballCollectorDropper;
    const byte ballLauncher;
    const byte ballCollector;
};
IRSensorPins irPins = {
    34, // ballCollectorDropper
    35, // ballLauncher
    32  // ballCollector
};