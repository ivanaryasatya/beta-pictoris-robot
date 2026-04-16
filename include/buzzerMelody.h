#pragma once
#include <Arduino.h>
using cuint = const unsigned int;


struct Melody {
    unsigned int startup[8] = {
        523,  8,
        659,  8,
        784,  8,
        1047, 4
    };

    cuint controllerConnected[10] = {
        1047, 16,
        0,    16,
        1047, 16, 
        0,    8,
        1319, 4 
    };

    cuint melodyDisconnected[6] = {
        1047, 8,
        784, 8,
        523, 2
    };
} melody;
