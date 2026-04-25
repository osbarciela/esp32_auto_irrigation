#pragma once
#include <Arduino.h>

class RelayDriver {
public:
    RelayDriver(uint8_t pin, bool activeLow = true);

    void begin();
    void on();
    void off();
    void toggle();
    bool isOn() const;

private:
    uint8_t _pin;
    bool    _activeLow;
    bool    _on = false;

    void _write(bool activate);
};
