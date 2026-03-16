#pragma once
#include <Arduino.h>

enum class WeatherCondition : uint8_t {
    Unknown,
    Clear,
    PartlyCloudy,
    Cloudy,
    Rainy,
    Stormy,
    Snowy,
    Foggy
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
