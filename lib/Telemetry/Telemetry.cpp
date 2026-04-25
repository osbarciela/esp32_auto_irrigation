#include "Telemetry.h"
#include <WiFi.h>

Telemetry::Telemetry(IrrigationController& controller, NtpSync& ntp,
                     PublishFn publishFn, uint32_t intervalMs)
    : _controller(controller), _ntp(ntp),
      _publish(publishFn), _intervalMs(intervalMs) {}

void Telemetry::loop() {
    if (millis() - _lastPublishMs >= _intervalMs) {
        publishNow();
    }
}

void Telemetry::publishNow() {
    _lastPublishMs = millis();
    String payload = _buildPayload();
    _publish("irrigation/metrics", payload);
}

String Telemetry::_buildPayload() {
    JsonDocument doc;

    doc["device"]   = "irrigation-esp32";
    doc["uptime_s"] = millis() / 1000;
    doc["timestamp"] = _ntp.isSynced() ? _ntp.formattedLocalTime() : "unsynced";
    doc["wifi_rssi"] = WiFi.RSSI();

    JsonArray zones = doc["zones"].to<JsonArray>();
    for (const ZoneStatus& s : _controller.allZoneStatuses()) {
        JsonObject z        = zones.add<JsonObject>();
        z["id"]             = s.id;
        z["name"]           = s.name;
        z["state"]          = _stateName(s.state);
        z["moisture_pct"]   = serialized(String(s.moisturePercent, 1));
        z["pump_on"]        = s.pumpOn;
        z["pump_running_ms"] = s.pumpRunningForMs;
        z["sensor_valid"]   = s.sensorValid;
    }

    String out;
    serializeJson(doc, out);
    return out;
}

const char* Telemetry::_stateName(ZoneState state) {
    switch (state) {
    case ZoneState::IDLE:            return "IDLE";
    case ZoneState::IRRIGATING:      return "IRRIGATING";
    case ZoneState::MANUAL_ON:       return "MANUAL_ON";
    case ZoneState::TIMEOUT_LOCKOUT: return "TIMEOUT_LOCKOUT";
    default:                         return "UNKNOWN";
    }
}
