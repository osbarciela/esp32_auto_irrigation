#include "IrrigationZone.h"
#include <esp_log.h>

static const char* TAG = "IrrigZone";

IrrigationZone::IrrigationZone(uint8_t id, const String& name,
                               MoistureSensor& sensor, RelayDriver& relay,
                               float startThr, float stopThr, uint32_t maxPumpMs,
                               uint8_t w1StartH, uint8_t w1StartM,
                               uint8_t w1EndH,   uint8_t w1EndM,
                               bool    w2Enabled,
                               uint8_t w2StartH, uint8_t w2StartM,
                               uint8_t w2EndH,   uint8_t w2EndM)
    : _id(id), _name(name), _sensor(sensor), _relay(relay),
      _startThreshold(startThr), _stopThreshold(stopThr),
      _maxPumpDurationMs(maxPumpMs),
      _win1{w1StartH, w1StartM, w1EndH, w1EndM, true},
      _win2{w2StartH, w2StartM, w2EndH, w2EndM, w2Enabled}
{}

void IrrigationZone::update(bool timeValid, const struct tm& now) {
    _sensor.update();
    float pct   = _sensor.moisturePercent();
    bool  inWin = timeValid && _isInsideAnyWindow(now);

    switch (_state) {
    case ZoneState::IDLE:
        if (inWin && _sensor.isValid() && pct < _startThreshold) {
            ESP_LOGI(TAG, "[Zone %d] Starting irrigation (moisture=%.1f%%)", _id, pct);
            _startPump();
            _state = ZoneState::IRRIGATING;
        }
        break;

    case ZoneState::IRRIGATING:
        if (!inWin) {
            ESP_LOGI(TAG, "[Zone %d] Window ended — stopping irrigation", _id);
            _stopPump();
            _state = ZoneState::IDLE;
        } else if (!_sensor.isValid() || pct >= _stopThreshold) {
            ESP_LOGI(TAG, "[Zone %d] Stopping irrigation (moisture=%.1f%%)", _id, pct);
            _stopPump();
            _state = ZoneState::IDLE;
        } else if (millis() - _pumpStartMs > _maxPumpDurationMs) {
            ESP_LOGW(TAG, "[Zone %d] Safety timeout reached", _id);
            _stopPump();
            _state = ZoneState::TIMEOUT_LOCKOUT;
        }
        break;

    case ZoneState::TIMEOUT_LOCKOUT:
        if (!inWin) {
            _state = ZoneState::IDLE;
        }
        break;

    case ZoneState::MANUAL_ON:
        break;
    }
}

void IrrigationZone::manualOn() {
    ESP_LOGI(TAG, "[Zone %d] Manual ON", _id);
    _startPump();
    _state = ZoneState::MANUAL_ON;
}

void IrrigationZone::manualOff() {
    ESP_LOGI(TAG, "[Zone %d] Manual OFF", _id);
    _stopPump();
    _state = ZoneState::IDLE;
}

void IrrigationZone::setThresholds(float startPct, float stopPct) {
    _startThreshold = startPct;
    _stopThreshold  = stopPct;
}

void IrrigationZone::setWindow1(uint8_t startH, uint8_t startM,
                                 uint8_t endH,   uint8_t endM) {
    _win1 = {startH, startM, endH, endM, true};
}

void IrrigationZone::setWindow2(bool enabled,
                                 uint8_t startH, uint8_t startM,
                                 uint8_t endH,   uint8_t endM) {
    _win2 = {startH, startM, endH, endM, enabled};
}

void IrrigationZone::setMaxPumpDuration(uint32_t ms) { _maxPumpDurationMs = ms; }

ZoneStatus IrrigationZone::status() const {
    uint32_t running = (_relay.isOn() && _pumpStartMs > 0)
                       ? millis() - _pumpStartMs : 0;
    return { _id, _name, _state,
             _sensor.moisturePercent(), _relay.isOn(),
             running, _sensor.isValid() };
}

bool IrrigationZone::_isInsideAnyWindow(const struct tm& t) const {
    return _windowContains(_win1, t) || (_win2.enabled && _windowContains(_win2, t));
}

bool IrrigationZone::_windowContains(const TimeWindow& w, const struct tm& t) const {
    int cur   = t.tm_hour * 60 + t.tm_min;
    int start = w.startH  * 60 + w.startM;
    int end   = w.endH    * 60 + w.endM;
    if (start <= end) return cur >= start && cur < end;
    return cur >= start || cur < end;
}

void IrrigationZone::_startPump() {
    _pumpStartMs = millis();
    _relay.on();
}
void IrrigationZone::_stopPump() {
    _relay.off();
}
