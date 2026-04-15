#pragma once
#include "models/WeatherData.h"
#include "models/TimeData.h"
#include "models/EventData.h"
#include <vector>

class IDisplay {
public:
    virtual ~IDisplay() = default;

    virtual bool init()    = 0;
    virtual void clear()   = 0;
    virtual void refresh() = 0;

    virtual void showWeather(const WeatherData& weather)              = 0;
    virtual void showTime(const TimeData& time)                       = 0;
    virtual void showEvents(const std::vector<EventData>& events)     = 0;
    virtual void showStatus(const String& message)                    = 0;
    virtual void showError(const String& message)                     = 0;
    virtual void showWaitConnection()                                  = 0;

    virtual bool isReady() const = 0;
};
