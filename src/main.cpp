#include <Arduino.h>
#include <ArduinoJson.h>
#include "Config.h"
#include "WiFiManager.h"
#include "NtpSync.h"
#include "MqttClient.h"
#include "IrrigationController.h"
#include "Telemetry.h"
#include "OtaUpdater.h"

uint32_t sensorCalibrationTimer = millis();

static Config config;
static NtpSync ntp;
static OtaUpdater ota;

static WiFiManager *wifi = nullptr;
static MqttClient *mqtt = nullptr;
static IrrigationController *ctrl = nullptr;
static Telemetry *telemetry = nullptr;

// ── MQTT command dispatcher ──────────────────────────────────────────────────

static void parseWindow(const char *str, uint8_t &h, uint8_t &m)
{
    if (!str || strlen(str) < 5)
        return;
    h = (uint8_t)atoi(str);
    m = (uint8_t)atoi(str + 3);
}

static void onMqttCommand(const String & /*topic*/, JsonDocument &doc)
{
    const char *cmd = doc["cmd"];
    if (!cmd)
        return;

    if (strcmp(cmd, "zone_on") == 0)
    {
        ctrl->manualZoneOn(doc["zone"].as<uint8_t>());
    }
    else if (strcmp(cmd, "zone_off") == 0)
    {
        ctrl->manualZoneOff(doc["zone"].as<uint8_t>());
    }
    else if (strcmp(cmd, "set_zone_config") == 0)
    {
        uint8_t z = doc["zone"].as<uint8_t>();
        ZoneConfig cfg = config.getZoneConfig(z);

        if (doc["moisture_start"].is<float>())
            cfg.moistureStartThreshold = doc["moisture_start"].as<float>();
        if (doc["moisture_stop"].is<float>())
            cfg.moistureStopThreshold = doc["moisture_stop"].as<float>();
        if (doc["max_pump_s"].is<uint32_t>())
            cfg.maxPumpDurationMs = doc["max_pump_s"].as<uint32_t>() * 1000UL;
        if (doc["window_start"].is<const char *>())
            parseWindow(doc["window_start"], cfg.windowStartHour, cfg.windowStartMinute);
        if (doc["window_end"].is<const char *>())
            parseWindow(doc["window_end"], cfg.windowEndHour, cfg.windowEndMinute);
        if (doc["window2_enabled"].is<bool>())
            cfg.window2Enabled = doc["window2_enabled"].as<bool>();
        if (doc["window2_start"].is<const char *>()) {
            parseWindow(doc["window2_start"], cfg.window2StartHour, cfg.window2StartMinute);
            cfg.window2Enabled = true;
        }
        if (doc["window2_end"].is<const char *>())
            parseWindow(doc["window2_end"], cfg.window2EndHour, cfg.window2EndMinute);

        config.setZoneConfig(z, cfg);
        ctrl->applyZoneConfig(z, cfg);
    }
    else if (strcmp(cmd, "set_mqtt") == 0)
    {
        config.setMqttBroker(doc["broker"].as<String>(),
                             doc["port"].as<uint16_t>());
    }
    else if (strcmp(cmd, "set_wifi") == 0)
    {
        config.setWifiCredentials(doc["ssid"].as<String>(),
                                  doc["password"].as<String>());
    }
    else if (strcmp(cmd, "publish_metrics") == 0)
    {
        telemetry->publishNow();
    }
    else if (strcmp(cmd, "ota") == 0)
    {
        ota.triggerUpdate(doc["url"].as<String>());
    }
    else if (strcmp(cmd, "restart") == 0)
    {
        ESP.restart();
    }
}

// ── setup() ──────────────────────────────────────────────────────────────────

void setup()
{
    Serial.begin(115200);
    delay(200);
    Serial.println("\n\n=== Auto Irrigation System starting ===");

    if (!config.begin())
    {
        Serial.println("[ERROR] Config init failed");
    }

    wifi = new WiFiManager(config.getWifiSSID(), config.getWifiPassword());
    mqtt = new MqttClient(config.getMqttBroker(), config.getMqttPort(),
                          config.getMqttClientId(),
                          config.getMqttUser(), config.getMqttPassword());
    ctrl = new IrrigationController(config);

    wifi->onConnected([]
                      {
        ntp.begin();
        mqtt->reconnect(); });

    mqtt->onCommand(onMqttCommand);
    mqtt->subscribe("irrigation/commands");
    mqtt->begin();

    telemetry = new Telemetry(
        *ctrl, ntp,
        [](const String &topic, const String &payload) -> bool
        {
            return mqtt->publish(topic, payload);
        });

    ota.onResult([](bool ok, const String &msg)
                 {
        String status = String("{\"ota_ok\":") + (ok ? "true" : "false")
                        + ",\"msg\":\"" + msg + "\"}";
        mqtt->publish("irrigation/status", status); });

    ctrl->begin();
    wifi->begin();

    Serial.println("=== Setup complete ===");
}

// ── loop() ────────────────────────────────────────────────────────────────────

void loop()
{
    wifi->loop();
    mqtt->loop();
    ntp.loop();

    bool timeOk = ntp.isSynced();
    struct tm now = ntp.localTime();

    ctrl->update(timeOk, now);
    telemetry->loop();
    ota.loop(*ctrl);

    if (millis() - sensorCalibrationTimer > 5000)
    {
        sensorCalibrationTimer = millis();
        Serial.print("Sensor reading : ");
        Serial.println(analogRead(34));
    }
}
