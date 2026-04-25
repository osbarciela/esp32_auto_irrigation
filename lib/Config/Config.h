#pragma once
#include <Arduino.h>
#include <Preferences.h>

struct ZoneConfig {
    uint8_t  sensorPin;
    uint8_t  relayPin;
    bool     relayActiveLow;
    String   name;
    float    moistureStartThreshold;
    float    moistureStopThreshold;
    uint8_t  windowStartHour;
    uint8_t  windowStartMinute;
    uint8_t  windowEndHour;
    uint8_t  windowEndMinute;
    bool     window2Enabled;
    uint8_t  window2StartHour;
    uint8_t  window2StartMinute;
    uint8_t  window2EndHour;
    uint8_t  window2EndMinute;
    uint32_t maxPumpDurationMs;
    int      adcDryValue;
    int      adcWetValue;
};

class Config {
public:
    Config();

    bool begin();

    String   getWifiSSID() const;
    String   getWifiPassword() const;
    void     setWifiCredentials(const String& ssid, const String& password);

    String   getMqttBroker() const;
    uint16_t getMqttPort() const;
    String   getMqttUser() const;
    String   getMqttPassword() const;
    String   getMqttClientId() const;
    void     setMqttBroker(const String& broker, uint16_t port);
    void     setMqttCredentials(const String& user, const String& password);

    uint8_t    getZoneCount() const;
    void       setZoneCount(uint8_t count);
    ZoneConfig getZoneConfig(uint8_t zoneIndex) const;
    void       setZoneConfig(uint8_t zoneIndex, const ZoneConfig& cfg);

private:
    mutable Preferences _prefs;
    bool                _open = false;

    static constexpr uint8_t MAX_ZONES = 4;

    String _zk(uint8_t idx, const char* field) const;
};
