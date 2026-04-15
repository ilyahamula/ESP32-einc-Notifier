#pragma once
#include "interfaces/IWeatherProvider.h"
#include "interfaces/IConnectivity.h"

// Fetches current weather from api.weatherapi.com (free /v1/current.json endpoint).
// Get a free API key at https://www.weatherapi.com
//
// Usage:
//   WeatherApiProvider provider(connectivity, WEATHERAPI_KEY, "Chernivtsi");
class WeatherApiProvider : public IWeatherProvider {
public:
    WeatherApiProvider(IConnectivity* connectivity,
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
