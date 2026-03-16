#pragma once
#include "interfaces/IWeatherProvider.h"
#include "models/WeatherData.h"

class WeatherManager {
public:
    WeatherManager(IWeatherProvider* provider, unsigned long intervalMs);

    bool init();
    void tick();

    const WeatherData& getCurrentWeather() const;
    bool hasData()    const;
    bool forceUpdate();

private:
    IWeatherProvider* _provider;
    WeatherData       _cached;
    unsigned long     _intervalMs;
    unsigned long     _lastFetchMs = 0;
};
