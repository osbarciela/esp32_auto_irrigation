#include "NtpSync.h"
#include <esp_sntp.h>
#include <esp_log.h>

static const char* TAG = "NtpSync";

NtpSync::NtpSync(const char* ntpServer, const char* timezone)
    : _ntpServer(ntpServer), _timezone(timezone) {}

void NtpSync::begin() {
    configTime(0, 0, _ntpServer);
    setenv("TZ", _timezone, 1);
    tzset();
    _lastSyncAttemptMs = millis();
    ESP_LOGI(TAG, "NTP sync started — server: %s", _ntpServer);
}

void NtpSync::loop() {
    if (!_synced) {
        if (sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED) {
            _synced = true;
            ESP_LOGI(TAG, "Time synced: %s", formattedLocalTime().c_str());
        } else if (millis() - _lastSyncAttemptMs > SYNC_TIMEOUT_MS) {
            ESP_LOGW(TAG, "Sync timeout, retrying…");
            sntp_restart();
            _lastSyncAttemptMs = millis();
        }
    } else if (millis() - _lastSyncAttemptMs > RESYNC_INTERVAL_MS) {
        _synced = false;
        sntp_restart();
        _lastSyncAttemptMs = millis();
        ESP_LOGI(TAG, "Periodic NTP resync");
    }
}

bool NtpSync::isSynced() const { return _synced; }

time_t NtpSync::now() const { return time(nullptr); }

struct tm NtpSync::localTime() const {
    time_t    t = now();
    struct tm result{};
    localtime_r(&t, &result);
    return result;
}

bool NtpSync::isInsideWindow(uint8_t startH, uint8_t startM,
                              uint8_t endH,   uint8_t endM) const {
    if (!_synced) return false;
    struct tm t = localTime();
    int currentMins = t.tm_hour * 60 + t.tm_min;
    int startMins   = startH   * 60 + startM;
    int endMins     = endH     * 60 + endM;

    if (startMins <= endMins) {
        return currentMins >= startMins && currentMins < endMins;
    } else {
        // Window crosses midnight (e.g. 22:00 → 06:00)
        return currentMins >= startMins || currentMins < endMins;
    }
}

String NtpSync::formattedLocalTime() const {
    struct tm t = localTime();
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S%z", &t);
    return String(buf);
}
