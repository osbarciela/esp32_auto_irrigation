#pragma once
#include <Arduino.h>
#include <functional>
#include "IrrigationController.h"

class OtaUpdater {
public:
    using ResultCallback = std::function<void(bool success, const String& msg)>;

    OtaUpdater();

    void triggerUpdate(const String& url);
    void loop(IrrigationController& controller);
    void onResult(ResultCallback cb);
    bool isPending() const;

private:
    String         _pendingUrl;
    bool           _pending = false;
    ResultCallback _resultCb;
};
