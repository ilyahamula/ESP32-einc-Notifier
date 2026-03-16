#pragma once
#include "models/EventData.h"
#include "models/WeatherData.h"
#include <functional>
#include <vector>

using EventSyncCallback   = std::function<void(const std::vector<EventData>&)>;
using WeatherSyncCallback = std::function<void(const WeatherData&)>;

class ISyncService {
public:
    virtual ~ISyncService() = default;

    virtual bool init()            = 0;
    virtual void tick()            = 0;   // called every loop() iteration
    virtual bool isRunning() const = 0;
    virtual bool requestSync()     = 0;

    virtual void onEventsReceived(EventSyncCallback callback)    = 0;
    virtual void onWeatherReceived(WeatherSyncCallback callback) = 0;
};
