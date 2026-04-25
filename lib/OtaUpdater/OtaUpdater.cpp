#include "OtaUpdater.h"
#include <HTTPUpdate.h>
#include <WiFiClient.h>
#include <esp_log.h>

static const char* TAG = "OtaUpdater";

OtaUpdater::OtaUpdater() {}

void OtaUpdater::triggerUpdate(const String& url) {
    if (_pending) return;
    _pendingUrl = url;
    _pending    = true;
    ESP_LOGI(TAG, "OTA queued: %s", url.c_str());
}

void OtaUpdater::loop(IrrigationController& controller) {
    if (!_pending) return;
    _pending = false;

    ESP_LOGI(TAG, "Starting OTA from: %s", _pendingUrl.c_str());
    controller.stopAllZones();

    WiFiClient client;
    httpUpdate.setLedPin(2, LOW);
    t_httpUpdate_return ret = httpUpdate.update(client, _pendingUrl);

    switch (ret) {
    case HTTP_UPDATE_OK:
        // HTTPUpdate reboots automatically — this line is unreachable
        break;
    case HTTP_UPDATE_FAILED:
        ESP_LOGE(TAG, "OTA failed: %s", httpUpdate.getLastErrorString().c_str());
        if (_resultCb) _resultCb(false, httpUpdate.getLastErrorString());
        break;
    case HTTP_UPDATE_NO_UPDATES:
        ESP_LOGI(TAG, "OTA: no updates available");
        if (_resultCb) _resultCb(false, "No update available");
        break;
    }
}

void OtaUpdater::onResult(ResultCallback cb) { _resultCb = cb; }
bool OtaUpdater::isPending() const           { return _pending; }
