#pragma once
#include <Arduino.h>

class MoistureSensor {
public:
    MoistureSensor(uint8_t pin,
                   int     adcDry     = 3200,
                   int     adcWet     = 1490,
                   uint8_t numSamples = 10);

    void  begin();
    void  update();

    int   rawValue() const;
    float moisturePercent() const;
    bool  isValid() const;

    void  setCalibration(int adcDry, int adcWet);

private:
    uint8_t _pin;
    int     _adcDry;
    int     _adcWet;
    uint8_t _numSamples;
    int     _rawValue = 0;
    bool    _valid    = false;

    int _readAveraged();
};
