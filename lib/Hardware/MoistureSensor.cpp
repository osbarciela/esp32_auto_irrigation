#include "MoistureSensor.h"

MoistureSensor::MoistureSensor(uint8_t pin, int adcDry, int adcWet, uint8_t numSamples)
    : _pin(pin), _adcDry(adcDry), _adcWet(adcWet), _numSamples(numSamples) {}

void MoistureSensor::begin() {
    pinMode(_pin, INPUT);
}

void MoistureSensor::update() {
    _rawValue = _readAveraged();
    // Allow 200 counts of margin beyond calibration range before marking invalid
    _valid = (_rawValue >= (_adcWet - 200)) && (_rawValue <= (_adcDry + 200));
}

int MoistureSensor::rawValue() const { return _rawValue; }

float MoistureSensor::moisturePercent() const {
    if (_adcDry == _adcWet) return 0.0f;
    float pct = (float)(_adcDry - _rawValue) / (float)(_adcDry - _adcWet) * 100.0f;
    if (pct < 0.0f)   pct = 0.0f;
    if (pct > 100.0f) pct = 100.0f;
    return pct;
}

bool MoistureSensor::isValid() const { return _valid; }

void MoistureSensor::setCalibration(int adcDry, int adcWet) {
    _adcDry = adcDry;
    _adcWet = adcWet;
}

int MoistureSensor::_readAveraged() {
    long sum = 0;
    for (uint8_t i = 0; i < _numSamples; i++) {
        sum += analogRead(_pin);
    }
    return (int)(sum / _numSamples);
}
