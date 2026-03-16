#pragma once
#include "interfaces/ISyncService.h"

// No-op sync service — does nothing.
// Replace with e.g. TelegramSyncService, BLESyncService, or MqttSyncService.
class NullSyncService : public ISyncService {
public:
    bool init()           override { return true; }
    void tick()           override {}
    bool isRunning() const override { return false; }
    bool requestSync()    override { return false; }

    void onEventsReceived(EventSyncCallback)    override {}
    void onWeatherReceived(WeatherSyncCallback) override {}
};
