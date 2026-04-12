#include "managers/WeatherManager.h"
#include "config.h"
#include <Arduino.h>

WeatherManager::WeatherManager(IWeatherProvider* provider, unsigned long intervalMs)
    : _provider(provider), _intervalMs(intervalMs) {}

bool WeatherManager::init() {
    return _provider != nullptr;
}

void WeatherManager::tick() {
    if (!_provider) return;
    unsigned long now = millis();
    if (_lastFetchMs == 0 || (now - _lastFetchMs) >= _intervalMs) {
        forceUpdate();
    }
}

const WeatherData& WeatherManager::getCurrentWeather() const {
    return _cached;
}

bool WeatherManager::hasData() const {
    return _cached.isValid;
}

bool WeatherManager::forceUpdate() {
    if (!_provider || !_provider->isAvailable()) return false;
    bool ok = _provider->fetch(_cached);
    if (ok) {
        _cached.fetchedAtMs = millis();
        _lastFetchMs        = millis();
        LOG("[WeatherManager] data updated");
    }
    return ok;
}
