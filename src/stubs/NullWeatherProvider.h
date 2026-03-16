#pragma once
#include "interfaces/IWeatherProvider.h"

// No-op provider — always reports unavailable.
// Replace with e.g. OpenWeatherMapProvider or a BLE-pushed provider.
class NullWeatherProvider : public IWeatherProvider {
public:
    bool fetch(WeatherData&)       override { return false; }
    bool isAvailable() const       override { return false; }
    void setLocation(const String&) override {}
};
