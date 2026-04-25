#pragma once
#include <PubSubClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include <functional>
#include <vector>

class MqttClient {
public:
    using CommandCallback = std::function<void(const String& topic, JsonDocument& doc)>;

    MqttClient(const String& broker,
               uint16_t      port,
               const String& clientId,
               const String& user,
               const String& password);

    void begin();
    void loop();

    bool isConnected();
    bool publish(const String& topic, const String& payload, bool retain = false);
    bool subscribe(const String& topic);
    void onCommand(CommandCallback cb);

    // Called by WiFiManager onConnected to trigger an immediate reconnect attempt
    void reconnect();

private:
    WiFiClient   _wifiClient;
    PubSubClient _pubSub;

    String   _broker;
    uint16_t _port;
    String   _clientId;
    String   _user;
    String   _password;

    uint32_t _lastReconnectMs = 0;
    uint32_t _reconnectInterval = 5000;
    uint8_t  _reconnectCount    = 0;

    static constexpr uint8_t  MAX_FAST_RETRIES       = 5;
    static constexpr uint32_t FAST_RECONNECT_MS      = 5000;
    static constexpr uint32_t SLOW_RECONNECT_MS      = 60000;

    CommandCallback      _commandCb;
    std::vector<String>  _subscriptions;

    static MqttClient* _instance;
    static void        _staticCallback(char* topic, byte* payload, unsigned int length);
    void               _handleMessage(const char* topic, const byte* payload, unsigned int length);

    bool _doConnect();
    void _resubscribeAll();
};
