#pragma once
#include <Arduino.h>
#include "MoistureSensor.h"
#include "RelayDriver.h"

enum class ZoneState {
    IDLE,
    IRRIGATING,
    MANUAL_ON,
    TIMEOUT_LOCKOUT
};

struct ZoneStatus {
    uint8_t   id;
    String    name;
    ZoneState state;
    float     moisturePercent;
    bool      pumpOn;
    uint32_t  pumpRunningForMs;
    bool      sensorValid;
};

class IrrigationZone {
public:
    IrrigationZone(uint8_t         id,
                   const String&   name,
                   MoistureSensor& sensor,
                   RelayDriver&    relay,
                   float           moistureStartThreshold,
                   float           moistureStopThreshold,
                   uint32_t        maxPumpDurationMs,
                   uint8_t         w1StartH, uint8_t w1StartM,
                   uint8_t         w1EndH,   uint8_t w1EndM,
                   bool            w2Enabled   = false,
                   uint8_t         w2StartH    = 20, uint8_t w2StartM = 0,
                   uint8_t         w2EndH      = 21, uint8_t w2EndM   = 0);

    void update(bool timeValid, const struct tm& localNow);

    void manualOn();
    void manualOff();

    void setThresholds(float startPct, float stopPct);
    void setWindow1(uint8_t startH, uint8_t startM, uint8_t endH, uint8_t endM);
    void setWindow2(bool enabled, uint8_t startH, uint8_t startM,
                                  uint8_t endH,   uint8_t endM);
    void setMaxPumpDuration(uint32_t ms);

    ZoneStatus status() const;
    uint8_t    id() const { return _id; }

private:
    struct TimeWindow {
        uint8_t startH, startM, endH, endM;
        bool    enabled;
    };

    uint8_t         _id;
    String          _name;
    MoistureSensor& _sensor;
    RelayDriver&    _relay;
    ZoneState       _state = ZoneState::IDLE;

    float      _startThreshold;
    float      _stopThreshold;
    uint32_t   _maxPumpDurationMs;
    TimeWindow _win1;
    TimeWindow _win2;

    uint32_t _pumpStartMs = 0;

    bool _isInsideAnyWindow(const struct tm& t) const;
    bool _windowContains(const TimeWindow& w, const struct tm& t) const;
    void _startPump();
    void _stopPump();
};
