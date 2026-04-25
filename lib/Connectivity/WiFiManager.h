#pragma once
#include <WiFi.h>
#include <functional>

class WiFiManager {
public:
    using ConnectedCallback    = std::function<void()>;
    using DisconnectedCallback = std::function<void()>;

    WiFiManager(const String& ssid, const String& password);

    void begin();
    void loop();

    bool   isConnected() const;
    String localIP() const;
    int8_t rssi() const;

    void onConnected(ConnectedCallback cb);
    void onDisconnected(DisconnectedCallback cb);

private:
    String _ssid;
    String _password;

    enum class State { DISCONNECTED, CONNECTING, CONNECTED };
    State    _state           = State::DISCONNECTED;
    uint32_t _lastAttemptMs   = 0;
    uint32_t _connectStartMs  = 0;
    uint8_t  _retryCount      = 0;

    static constexpr uint32_t CONNECT_TIMEOUT_MS        = 15000;
    static constexpr uint32_t RECONNECT_INTERVAL_MS     = 30000;
    static constexpr uint8_t  MAX_RETRIES_BEFORE_REBOOT = 10;

    ConnectedCallback    _onConnected;
    DisconnectedCallback _onDisconnected;

    void _startConnect();
};
