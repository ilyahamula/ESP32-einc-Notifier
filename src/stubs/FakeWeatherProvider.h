#pragma once
#include "interfaces/IWeatherProvider.h"

// Returns a fixed weather snapshot — useful for UI development without a network.
class FakeWeatherProvider : public IWeatherProvider {
public:
    bool fetch(WeatherData& out) override {
        out.temperatureCelsius = 18.5f;
        out.feelsLikeCelsius   = 16.0f;
        out.humidityPercent    = 62.0f;
        out.windSpeedKmh       = 14.0f;
        out.windDirection      = WindDirection::NW;
        out.condition          = WeatherCondition::PartlyCloudy;
        out.location           = "Kyiv, UA";
        out.isValid            = true;
        out.fetchedAtMs        = millis();
        return true;
    }

    bool isAvailable() const       override { return true; }
    void setLocation(const String&) override {}
};
