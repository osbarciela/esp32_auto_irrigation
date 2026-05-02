#include "Config.h"
#include "credentials.h"

Config::Config() {}

bool Config::begin() {
    _open = _prefs.begin("irrigation", false);
    if (_open && _prefs.getUChar("initialized", 0) == 0) {
        _prefs.putUChar("initialized", 1);
        _prefs.putString("wifi_ssid",  WIFI_SSID);
        _prefs.putString("wifi_pass",  WIFI_PASSWORD);
        _prefs.putString("mqtt_host",  MQTT_HOST);
        _prefs.putUShort("mqtt_port",  MQTT_PORT);
        _prefs.putString("mqtt_user",  MQTT_USER);
        _prefs.putString("mqtt_pass",  MQTT_PASSWORD);
        _prefs.putUChar("zone_count",  1);

        // Default zone 0
        ZoneConfig z0{};
        z0.sensorPin               = 34;
        z0.relayPin                = 26;
        z0.relayActiveLow          = false;
        z0.name                    = "Zona 0";
        z0.moistureStartThreshold  = 40.0f;
        z0.moistureStopThreshold   = 65.0f;
        z0.windowStartHour         = 6;
        z0.windowStartMinute       = 0;
        z0.windowEndHour           = 8;
        z0.windowEndMinute         = 0;
        z0.window2Enabled          = false;
        z0.window2StartHour        = 20;
        z0.window2StartMinute      = 0;
        z0.window2EndHour          = 21;
        z0.window2EndMinute        = 0;
        z0.maxPumpDurationMs       = 300000;
        z0.adcDryValue             = 3330;
        z0.adcWetValue             = 1040;
        setZoneConfig(0, z0);
    }
    return _open;
}

String Config::getWifiSSID() const     { return _prefs.getString("wifi_ssid", ""); }
String Config::getWifiPassword() const { return _prefs.getString("wifi_pass", ""); }
void   Config::setWifiCredentials(const String& ssid, const String& password) {
    _prefs.putString("wifi_ssid", ssid);
    _prefs.putString("wifi_pass", password);
}

String   Config::getMqttBroker()   const { return _prefs.getString("mqtt_host", "192.168.1.100"); }
uint16_t Config::getMqttPort()     const { return _prefs.getUShort("mqtt_port", 1883); }
String   Config::getMqttUser()     const { return _prefs.getString("mqtt_user", ""); }
String   Config::getMqttPassword() const { return _prefs.getString("mqtt_pass", ""); }
String   Config::getMqttClientId() const {
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    char id[24];
    snprintf(id, sizeof(id), "irrigation-%02X%02X%02X", mac[3], mac[4], mac[5]);
    return String(id);
}
void Config::setMqttBroker(const String& broker, uint16_t port) {
    _prefs.putString("mqtt_host", broker);
    _prefs.putUShort("mqtt_port", port);
}
void Config::setMqttCredentials(const String& user, const String& password) {
    _prefs.putString("mqtt_user", user);
    _prefs.putString("mqtt_pass", password);
}

uint8_t Config::getZoneCount() const { return _prefs.getUChar("zone_count", 1); }
void    Config::setZoneCount(uint8_t count) { _prefs.putUChar("zone_count", count); }

ZoneConfig Config::getZoneConfig(uint8_t idx) const {
    ZoneConfig z{};
    z.sensorPin              = _prefs.getUChar(_zk(idx,"sp").c_str(),  34);
    z.relayPin               = _prefs.getUChar(_zk(idx,"rp").c_str(),  26);
    z.relayActiveLow         = _prefs.getBool (_zk(idx,"ral").c_str(), true);
    z.name                   = _prefs.getString(_zk(idx,"nm").c_str(), "Zona " + String(idx));
    z.moistureStartThreshold = _prefs.getFloat (_zk(idx,"mst").c_str(), 40.0f);
    z.moistureStopThreshold  = _prefs.getFloat (_zk(idx,"msp").c_str(), 65.0f);
    z.windowStartHour        = _prefs.getUChar (_zk(idx,"wsh").c_str(),  6);
    z.windowStartMinute      = _prefs.getUChar (_zk(idx,"wsm").c_str(),  0);
    z.windowEndHour          = _prefs.getUChar (_zk(idx,"weh").c_str(),  8);
    z.windowEndMinute        = _prefs.getUChar (_zk(idx,"wem").c_str(),  0);
    z.window2Enabled         = _prefs.getBool  (_zk(idx,"w2e").c_str(),  false);
    z.window2StartHour       = _prefs.getUChar (_zk(idx,"w2sh").c_str(), 20);
    z.window2StartMinute     = _prefs.getUChar (_zk(idx,"w2sm").c_str(), 0);
    z.window2EndHour         = _prefs.getUChar (_zk(idx,"w2eh").c_str(), 21);
    z.window2EndMinute       = _prefs.getUChar (_zk(idx,"w2em").c_str(), 0);
    z.maxPumpDurationMs      = _prefs.getULong (_zk(idx,"mpd").c_str(),  300000);
    z.adcDryValue            = _prefs.getInt   (_zk(idx,"dry").c_str(), 3200);
    z.adcWetValue            = _prefs.getInt   (_zk(idx,"wet").c_str(), 1490);
    return z;
}

void Config::setZoneConfig(uint8_t idx, const ZoneConfig& z) {
    _prefs.putUChar (_zk(idx,"sp").c_str(),  z.sensorPin);
    _prefs.putUChar (_zk(idx,"rp").c_str(),  z.relayPin);
    _prefs.putBool  (_zk(idx,"ral").c_str(), z.relayActiveLow);
    _prefs.putString(_zk(idx,"nm").c_str(),  z.name);
    _prefs.putFloat (_zk(idx,"mst").c_str(), z.moistureStartThreshold);
    _prefs.putFloat (_zk(idx,"msp").c_str(), z.moistureStopThreshold);
    _prefs.putUChar (_zk(idx,"wsh").c_str(),  z.windowStartHour);
    _prefs.putUChar (_zk(idx,"wsm").c_str(),  z.windowStartMinute);
    _prefs.putUChar (_zk(idx,"weh").c_str(),  z.windowEndHour);
    _prefs.putUChar (_zk(idx,"wem").c_str(),  z.windowEndMinute);
    _prefs.putBool  (_zk(idx,"w2e").c_str(),  z.window2Enabled);
    _prefs.putUChar (_zk(idx,"w2sh").c_str(), z.window2StartHour);
    _prefs.putUChar (_zk(idx,"w2sm").c_str(), z.window2StartMinute);
    _prefs.putUChar (_zk(idx,"w2eh").c_str(), z.window2EndHour);
    _prefs.putUChar (_zk(idx,"w2em").c_str(), z.window2EndMinute);
    _prefs.putULong (_zk(idx,"mpd").c_str(),  z.maxPumpDurationMs);
    _prefs.putInt   (_zk(idx,"dry").c_str(), z.adcDryValue);
    _prefs.putInt   (_zk(idx,"wet").c_str(), z.adcWetValue);
}

String Config::_zk(uint8_t idx, const char* field) const {
    return String("z") + idx + "_" + field;
}
