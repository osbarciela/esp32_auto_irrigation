#pragma once
#include <vector>
#include <memory>
#include "IrrigationZone.h"
#include "MoistureSensor.h"
#include "RelayDriver.h"
#include "Config.h"

class IrrigationController {
public:
    explicit IrrigationController(Config& config);

    void begin();
    void update(bool timeValid, const struct tm& localNow);

    void manualZoneOn(uint8_t zoneId);
    void manualZoneOff(uint8_t zoneId);
    void applyZoneConfig(uint8_t zoneId, const ZoneConfig& cfg);
    void stopAllZones();

    std::vector<ZoneStatus> allZoneStatuses() const;
    size_t                  zoneCount() const;

private:
    Config& _config;

    std::vector<std::unique_ptr<MoistureSensor>> _sensors;
    std::vector<std::unique_ptr<RelayDriver>>    _relays;
    std::vector<std::unique_ptr<IrrigationZone>> _zones;

    void _buildZone(uint8_t idx);
};
