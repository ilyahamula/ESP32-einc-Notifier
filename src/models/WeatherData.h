#pragma once
#include <Arduino.h>

enum class WeatherCondition : uint8_t {
    Unknown,
    // Clear / Sun
    Sunny,              // 1000 day
    Clear,              // 1000 night
    // Clouds
    PartlyCloudy,       // 1003
    Cloudy,             // 1006
    Overcast,           // 1009
    // Fog / Mist
    Mist,               // 1030
    Fog,                // 1135
    FreezingFog,        // 1147
    // Possible / patchy
    PatchyRainPossible, // 1063
    PatchySnowPossible, // 1066
    PatchySleetPossible,// 1069
    PatchyFreezingDrizzle, // 1072
    ThunderyOutbreaks,  // 1087
    // Snow / blizzard
    BlowingSnow,        // 1114
    Blizzard,           // 1117
    // Drizzle
    LightDrizzle,       // 1150 1153
    FreezingDrizzle,    // 1168
    HeavyFreezingDrizzle, // 1171
    // Rain
    LightRain,          // 1180 1183
    ModerateRain,       // 1186 1189
    HeavyRain,          // 1192 1195
    LightFreezingRain,  // 1198
    HeavyFreezingRain,  // 1201
    // Sleet
    LightSleet,         // 1204
    HeavySleet,         // 1207
    // Snow
    LightSnow,          // 1210 1213
    ModerateSnow,       // 1216 1219
    HeavySnow,          // 1222 1225
    IcePellets,         // 1237
    // Showers
    LightRainShower,    // 1240
    HeavyRainShower,    // 1243 1246
    LightSleetShowers,  // 1249
    HeavySleetShowers,  // 1252
    LightSnowShowers,   // 1255
    HeavySnowShowers,   // 1258
    LightIcePellets,    // 1261
    HeavyIcePellets,    // 1264
    // Thunder
    LightRainThunder,   // 1273
    HeavyRainThunder,   // 1276
    LightSnowThunder,   // 1279
    HeavySnowThunder,   // 1282
};

enum class WindDirection : uint8_t {
    Unknown, N, NE, E, SE, S, SW, W, NW
};

struct WeatherData {
    float            temperatureCelsius = 0.0f;
    float            feelsLikeCelsius   = 0.0f;
    float            humidityPercent    = 0.0f;
    float            windSpeedKmh       = 0.0f;
    WindDirection    windDirection      = WindDirection::Unknown;
    WeatherCondition condition          = WeatherCondition::Unknown;
    String           location;
    bool             isValid            = false;
    unsigned long    fetchedAtMs        = 0;
};
