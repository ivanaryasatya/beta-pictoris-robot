#pragma once
#include <Arduino.h>

class HallSensor {
  private:
    const uint8_t _pin; 
    uint16_t _threshold; 

  public:    
    HallSensor(uint8_t pin, uint16_t threshold) : _pin(pin), _threshold(threshold) {}
    inline void begin() const {
        pinMode(_pin, INPUT);
    }
    inline uint16_t readAnalog() const {
        return analogRead(_pin);
    }   
    
    inline bool isTriggered() const {
        return analogRead(_pin) >= _threshold; 
    }
    
    inline void setThreshold(uint16_t threshold) {
        _threshold = threshold;
    }
};