#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <functional>
#include "IrrigationController.h"
#include "NtpSync.h"

class Telemetry {
public:
    using PublishFn = std::function<bool(const String& topic, const String& payload)>;

    Telemetry(IrrigationController& controller,
              NtpSync&              ntp,
              PublishFn             publishFn,
              uint32_t              intervalMs = 30000);

    void loop();
    void publishNow();

private:
    IrrigationController& _controller;
    NtpSync&              _ntp;
    PublishFn             _publish;
    uint32_t              _intervalMs;
    uint32_t              _lastPublishMs = 0;

    String _buildPayload();

    static const char* _stateName(ZoneState state);
};
