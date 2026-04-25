#include "RelayDriver.h"

RelayDriver::RelayDriver(uint8_t pin, bool activeLow)
    : _pin(pin), _activeLow(activeLow) {}

void RelayDriver::begin() {
    pinMode(_pin, OUTPUT);
    _write(false);
}

void RelayDriver::on()     { _write(true);  }
void RelayDriver::off()    { _write(false); }
void RelayDriver::toggle() { _write(!_on);  }
bool RelayDriver::isOn() const { return _on; }

void RelayDriver::_write(bool activate) {
    _on = activate;
    digitalWrite(_pin, _activeLow ? !activate : activate);
}
