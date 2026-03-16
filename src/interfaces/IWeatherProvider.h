#pragma once
#include "models/WeatherData.h"

class IWeatherProvider {
public:
    virtual ~IWeatherProvider() = default;

    // Populate `out` and return true on success
    virtual bool fetch(WeatherData& out) = 0;
    virtual bool isAvailable() const     = 0;
    virtual void setLocation(const String& location) = 0;
};
