#include "WiFiManager.h"
#include <esp_log.h>

static const char* TAG = "WiFiMgr";

WiFiManager::WiFiManager(const String& ssid, const String& password)
    : _ssid(ssid), _password(password) {}

void WiFiManager::begin() {
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(false);
    _startConnect();
}

void WiFiManager::loop() {
    switch (_state) {
    case State::CONNECTING:
        if (WiFi.status() == WL_CONNECTED) {
            _retryCount = 0;
            _state = State::CONNECTED;
            ESP_LOGI(TAG, "Connected — IP: %s", WiFi.localIP().toString().c_str());
            if (_onConnected) _onConnected();
        } else if (millis() - _connectStartMs > CONNECT_TIMEOUT_MS) {
            ESP_LOGW(TAG, "Connect timeout (attempt %d)", _retryCount + 1);
            WiFi.disconnect(true);
            _state = State::DISCONNECTED;
            _lastAttemptMs = millis();
            if (_onDisconnected) _onDisconnected();
        }
        break;

    case State::CONNECTED:
        if (WiFi.status() != WL_CONNECTED) {
            ESP_LOGW(TAG, "Lost connection");
            WiFi.disconnect(true);
            _state = State::DISCONNECTED;
            _lastAttemptMs = millis();
            if (_onDisconnected) _onDisconnected();
        }
        break;

    case State::DISCONNECTED:
        if (millis() - _lastAttemptMs >= RECONNECT_INTERVAL_MS) {
            _retryCount++;
            if (_retryCount > MAX_RETRIES_BEFORE_REBOOT) {
                ESP_LOGE(TAG, "Too many retries — restarting");
                ESP.restart();
            }
            _startConnect();
        }
        break;
    }
}

bool   WiFiManager::isConnected() const { return WiFi.status() == WL_CONNECTED; }
String WiFiManager::localIP()     const { return WiFi.localIP().toString(); }
int8_t WiFiManager::rssi()        const { return WiFi.RSSI(); }

void WiFiManager::onConnected(ConnectedCallback cb)       { _onConnected = cb; }
void WiFiManager::onDisconnected(DisconnectedCallback cb) { _onDisconnected = cb; }

void WiFiManager::_startConnect() {
    ESP_LOGI(TAG, "Connecting to %s…", _ssid.c_str());
    WiFi.begin(_ssid.c_str(), _password.c_str());
    _connectStartMs = millis();
    _state = State::CONNECTING;
}
