#include "providers/OpenWeatherMapProvider.h"
#include "config.h"
#include <HTTPClient.h>
#include <cJSON.h>
#include <Arduino.h>

// OpenWeatherMap free endpoint:
//   GET https://api.openweathermap.org/data/2.5/weather?q={city}&appid={key}&units=metric
// Response subset we use:
//   weather[0].id        → WeatherCondition
//   main.temp            → temperatureCelsius
//   main.feels_like      → feelsLikeCelsius
//   main.humidity        → humidityPercent
//   wind.speed (m/s)     → windSpeedKmh  (×3.6)
//   wind.deg             → windDirection
//   name + sys.country   → location

static constexpr const char* kBaseUrl =
    "https://api.openweathermap.org/data/2.5/weather";

OpenWeatherMapProvider::OpenWeatherMapProvider(IConnectivity* connectivity,
                                               const char*    apiKey,
                                               const String&  location)
    : _connectivity(connectivity), _apiKey(apiKey), _location(location) {}

bool OpenWeatherMapProvider::isAvailable() const {
    return _connectivity && _connectivity->isConnected() && _location.length() > 0;
}

void OpenWeatherMapProvider::setLocation(const String& location) {
    _location = location;
}

bool OpenWeatherMapProvider::fetch(WeatherData& out) {
    if (!isAvailable()) {
        LOG("[OWM] Not available (no connection or location not set)");
        return false;
    }

    char url[256];
    snprintf(url, sizeof(url),
             "%s?q=%s&appid=%s&units=metric",
             kBaseUrl, _location.c_str(), _apiKey);

    HTTPClient http;
    http.begin(url);
    http.setTimeout(8000);
    const int code = http.GET();

    if (code != 200) {
        LOG_F("[OWM] HTTP error %d\n", code);
        http.end();
        return false;
    }

    const String body = http.getString();
    http.end();

    cJSON* root = cJSON_Parse(body.c_str());
    if (!root) {
        LOG("[OWM] JSON parse failed");
        return false;
    }

    bool ok = false;

    // weather[0].id
    cJSON* weather = cJSON_GetObjectItem(root, "weather");
    if (cJSON_IsArray(weather) && cJSON_GetArraySize(weather) > 0) {
        cJSON* w0  = cJSON_GetArrayItem(weather, 0);
        cJSON* wid = cJSON_GetObjectItem(w0, "id");
        if (cJSON_IsNumber(wid))
            out.condition = mapConditionCode(static_cast<int>(wid->valuedouble));
    }

    // main
    cJSON* main = cJSON_GetObjectItem(root, "main");
    if (cJSON_IsObject(main)) {
        cJSON* temp  = cJSON_GetObjectItem(main, "temp");
        cJSON* feels = cJSON_GetObjectItem(main, "feels_like");
        cJSON* hum   = cJSON_GetObjectItem(main, "humidity");
        if (cJSON_IsNumber(temp))  out.temperatureCelsius = static_cast<float>(temp->valuedouble);
        if (cJSON_IsNumber(feels)) out.feelsLikeCelsius   = static_cast<float>(feels->valuedouble);
        if (cJSON_IsNumber(hum))   out.humidityPercent    = static_cast<float>(hum->valuedouble);
        ok = true;
    }

    // wind
    cJSON* wind = cJSON_GetObjectItem(root, "wind");
    if (cJSON_IsObject(wind)) {
        cJSON* spd = cJSON_GetObjectItem(wind, "speed");
        cJSON* deg = cJSON_GetObjectItem(wind, "deg");
        if (cJSON_IsNumber(spd))
            out.windSpeedKmh = static_cast<float>(spd->valuedouble) * 3.6f;
        if (cJSON_IsNumber(deg))
            out.windDirection = mapWindDegrees(static_cast<float>(deg->valuedouble));
    }

    // location: "name, sys.country"
    cJSON* name    = cJSON_GetObjectItem(root, "name");
    cJSON* sys     = cJSON_GetObjectItem(root, "sys");
    cJSON* country = sys ? cJSON_GetObjectItem(sys, "country") : nullptr;
    if (cJSON_IsString(name)) {
        out.location = name->valuestring;
        if (cJSON_IsString(country)) {
            out.location += ", ";
            out.location += country->valuestring;
        }
    }

    cJSON_Delete(root);

    if (ok) {
        out.isValid     = true;
        out.fetchedAtMs = millis();
        LOG_F("[OWM] Fetched: %.1fC, %s\n",
              out.temperatureCelsius, out.location.c_str());
    }
    return ok;
}

// ─── OWM condition code → WeatherCondition ────────────────────────────────────
// https://openweathermap.org/weather-conditions
WeatherCondition OpenWeatherMapProvider::mapConditionCode(int code) {
    if (code >= 200 && code < 300) return WeatherCondition::ThunderyOutbreaks;
    if (code >= 300 && code < 400) return WeatherCondition::LightDrizzle;
    if (code >= 500 && code < 600) return WeatherCondition::ModerateRain;
    if (code >= 600 && code < 700) return WeatherCondition::ModerateSnow;
    if (code >= 700 && code < 800) return WeatherCondition::Fog;
    if (code == 800)               return WeatherCondition::Clear;
    if (code == 801 || code == 802) return WeatherCondition::PartlyCloudy;
    if (code >= 803)               return WeatherCondition::Cloudy;
    return WeatherCondition::Unknown;
}

// ─── Wind degrees → WindDirection ─────────────────────────────────────────────
WindDirection OpenWeatherMapProvider::mapWindDegrees(float deg) {
    // Each sector spans 45°, centred on the cardinal/intercardinal direction.
    // Shift by 22.5° so 0°±22.5° = N, 45°±22.5° = NE, etc.
    const int sector = static_cast<int>((deg + 22.5f) / 45.0f) % 8;
    switch (sector) {
        case 0: return WindDirection::N;
        case 1: return WindDirection::NE;
        case 2: return WindDirection::E;
        case 3: return WindDirection::SE;
        case 4: return WindDirection::S;
        case 5: return WindDirection::SW;
        case 6: return WindDirection::W;
        case 7: return WindDirection::NW;
        default: return WindDirection::Unknown;
    }
}
