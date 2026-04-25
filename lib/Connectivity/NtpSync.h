#pragma once
#include <Arduino.h>
#include <time.h>

class NtpSync {
public:
    NtpSync(const char* ntpServer = "pool.ntp.org",
            const char* timezone  = "CET-1CEST,M3.5.0,M10.5.0/3");

    void begin();
    void loop();

    bool      isSynced() const;
    time_t    now() const;
    struct tm localTime() const;

    bool   isInsideWindow(uint8_t startH, uint8_t startM,
                          uint8_t endH,   uint8_t endM) const;
    String formattedLocalTime() const;

private:
    const char* _ntpServer;
    const char* _timezone;
    uint32_t    _lastSyncAttemptMs = 0;
    bool        _synced            = false;

    static constexpr uint32_t RESYNC_INTERVAL_MS = 86400000UL;
    static constexpr uint32_t SYNC_TIMEOUT_MS    = 10000;
};
