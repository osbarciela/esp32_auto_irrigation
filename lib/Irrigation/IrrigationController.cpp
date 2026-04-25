#include "IrrigationController.h"
#include <esp_log.h>

static const char* TAG = "IrrigCtrl";

IrrigationController::IrrigationController(Config& config) : _config(config) {}

void IrrigationController::begin() {
    _sensors.clear();
    _relays.clear();
    _zones.clear();

    uint8_t count = _config.getZoneCount();
    ESP_LOGI(TAG, "Initializing %d zone(s)", count);
    for (uint8_t i = 0; i < count; i++) {
        _buildZone(i);
    }
}

void IrrigationController::update(bool timeValid, const struct tm& localNow) {
    for (auto& zone : _zones) {
        zone->update(timeValid, localNow);
    }
}

void IrrigationController::manualZoneOn(uint8_t zoneId) {
    for (auto& zone : _zones) {
        if (zone->id() == zoneId) { zone->manualOn(); return; }
    }
    ESP_LOGW(TAG, "manualZoneOn: zone %d not found", zoneId);
}

void IrrigationController::manualZoneOff(uint8_t zoneId) {
    for (auto& zone : _zones) {
        if (zone->id() == zoneId) { zone->manualOff(); return; }
    }
    ESP_LOGW(TAG, "manualZoneOff: zone %d not found", zoneId);
}

void IrrigationController::applyZoneConfig(uint8_t zoneId, const ZoneConfig& cfg) {
    for (auto& zone : _zones) {
        if (zone->id() == zoneId) {
            zone->setThresholds(cfg.moistureStartThreshold, cfg.moistureStopThreshold);
            zone->setWindow1(cfg.windowStartHour, cfg.windowStartMinute,
                             cfg.windowEndHour,   cfg.windowEndMinute);
            zone->setWindow2(cfg.window2Enabled,
                             cfg.window2StartHour, cfg.window2StartMinute,
                             cfg.window2EndHour,   cfg.window2EndMinute);
            zone->setMaxPumpDuration(cfg.maxPumpDurationMs);
            return;
        }
    }
    ESP_LOGW(TAG, "applyZoneConfig: zone %d not found", zoneId);
}

void IrrigationController::stopAllZones() {
    for (auto& zone : _zones) {
        zone->manualOff();
    }
}

std::vector<ZoneStatus> IrrigationController::allZoneStatuses() const {
    std::vector<ZoneStatus> result;
    result.reserve(_zones.size());
    for (const auto& zone : _zones) {
        result.push_back(zone->status());
    }
    return result;
}

size_t IrrigationController::zoneCount() const { return _zones.size(); }

void IrrigationController::_buildZone(uint8_t idx) {
    ZoneConfig cfg = _config.getZoneConfig(idx);

    auto sensor = std::make_unique<MoistureSensor>(
        cfg.sensorPin, cfg.adcDryValue, cfg.adcWetValue);
    auto relay  = std::make_unique<RelayDriver>(cfg.relayPin, cfg.relayActiveLow);

    sensor->begin();
    relay->begin();

    auto zone = std::make_unique<IrrigationZone>(
        idx,
        cfg.name,
        *sensor,
        *relay,
        cfg.moistureStartThreshold,
        cfg.moistureStopThreshold,
        cfg.maxPumpDurationMs,
        cfg.windowStartHour,   cfg.windowStartMinute,
        cfg.windowEndHour,     cfg.windowEndMinute,
        cfg.window2Enabled,
        cfg.window2StartHour,  cfg.window2StartMinute,
        cfg.window2EndHour,    cfg.window2EndMinute
    );

    _sensors.push_back(std::move(sensor));
    _relays.push_back(std::move(relay));
    _zones.push_back(std::move(zone));

    ESP_LOGI(TAG, "Zone %d (%s) — sensor GPIO%d, relay GPIO%d",
             idx, cfg.name.c_str(), cfg.sensorPin, cfg.relayPin);
}
