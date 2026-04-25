#include "MqttClient.h"
#include <WiFi.h>
#include <esp_log.h>

static const char* TAG = "MqttClient";

MqttClient* MqttClient::_instance = nullptr;

MqttClient::MqttClient(const String& broker, uint16_t port,
                       const String& clientId, const String& user,
                       const String& password)
    : _pubSub(_wifiClient),
      _broker(broker), _port(port),
      _clientId(clientId), _user(user), _password(password)
{
    _instance = this;
}

void MqttClient::begin() {
    _pubSub.setServer(_broker.c_str(), _port);
    _pubSub.setCallback(_staticCallback);
    _pubSub.setBufferSize(1024);
}

void MqttClient::loop() {
    if (!_pubSub.connected()) {
        uint32_t now = millis();
        if (now - _lastReconnectMs >= _reconnectInterval) {
            _lastReconnectMs = now;
            if (_doConnect()) {
                _reconnectCount    = 0;
                _reconnectInterval = FAST_RECONNECT_MS;
                _resubscribeAll();
                String ip = WiFi.localIP().toString();
                String msg = "{\"online\":true,\"ip\":\"" + ip + "\",\"firmware\":\"1.0.0\"}";
                _pubSub.publish("irrigation/status", msg.c_str(), true);
                ESP_LOGI(TAG, "Connected to %s:%d", _broker.c_str(), _port);
            } else {
                _reconnectCount++;
                _reconnectInterval = (_reconnectCount < MAX_FAST_RETRIES)
                                     ? FAST_RECONNECT_MS
                                     : SLOW_RECONNECT_MS;
                ESP_LOGW(TAG, "MQTT connect failed (attempt %d)", _reconnectCount);
            }
        }
    } else {
        _pubSub.loop();
    }
}

bool MqttClient::isConnected() { return _pubSub.connected(); }

bool MqttClient::publish(const String& topic, const String& payload, bool retain) {
    if (!_pubSub.connected()) return false;
    return _pubSub.publish(topic.c_str(), payload.c_str(), retain);
}

bool MqttClient::subscribe(const String& topic) {
    // Track for resubscription after reconnect
    for (const auto& t : _subscriptions) {
        if (t == topic) return true; // already tracked
    }
    _subscriptions.push_back(topic);
    if (_pubSub.connected()) {
        return _pubSub.subscribe(topic.c_str());
    }
    return true;
}

void MqttClient::onCommand(CommandCallback cb) { _commandCb = cb; }

void MqttClient::reconnect() {
    _lastReconnectMs   = 0; // force immediate attempt on next loop()
    _reconnectInterval = FAST_RECONNECT_MS;
}

bool MqttClient::_doConnect() {
    static const char* lwt_topic   = "irrigation/status";
    static const char* lwt_payload = "{\"online\":false}";
    return _pubSub.connect(_clientId.c_str(),
                           _user.c_str(), _password.c_str(),
                           lwt_topic, 0, true, lwt_payload);
}

void MqttClient::_resubscribeAll() {
    for (const auto& t : _subscriptions) {
        _pubSub.subscribe(t.c_str());
        ESP_LOGI(TAG, "Subscribed to %s", t.c_str());
    }
}

void MqttClient::_staticCallback(char* topic, byte* payload, unsigned int length) {
    if (_instance) {
        _instance->_handleMessage(topic, payload, length);
    }
}

void MqttClient::_handleMessage(const char* topic, const byte* payload, unsigned int length) {
    if (!_commandCb) return;

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload, length);
    if (err) {
        ESP_LOGW(TAG, "JSON parse error: %s", err.c_str());
        return;
    }
    _commandCb(String(topic), doc);
}
