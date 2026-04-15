#pragma once
#include "interfaces/IWeatherProvider.h"
#include "interfaces/IConnectivity.h"

// Fetches current weather from api.openweathermap.org (free /data/2.5/weather endpoint).
// Requires a free API key from https://openweathermap.org/api
//
// Usage:
//   OpenWeatherMapProvider provider(connectivity, OWM_API_KEY, "Kyiv,UA");
//   WeatherData out;
//   if (provider.fetch(out)) { /* use out */ }
class OpenWeatherMapProvider : public IWeatherProvider {
public:
    OpenWeatherMapProvider(IConnectivity* connectivity,
                           const char*    apiKey,
                           const String&  location = "");

    bool fetch(WeatherData& out)             override;
    bool isAvailable()                 const override;
    void setLocation(const String& location) override;

private:
    IConnectivity* _connectivity;
    const char*    _apiKey;
    String         _location;

    static WeatherCondition mapConditionCode(int code);
    static WindDirection    mapWindDegrees(float deg);
};
