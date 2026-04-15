#include "managers/DisplayManager.h"
#include "config.h"
#include <Arduino.h>

DisplayManager::DisplayManager(IDisplay* display)
    : _display(display) {}

bool DisplayManager::init() {
    if (!_display) return false;
    _initialized = _display->init();
    if (!_initialized) {
        LOG("[DisplayManager] init failed");
    }
    return _initialized;
}

void DisplayManager::update(const WeatherData& weather,
                             const TimeData& time,
                             const std::vector<EventData>& events) {

    LOG("[DisplayManager] Updating display with new data...");
    if (!_initialized || !_display) return;
    _display->clear();
    _display->showTime(time);
    _display->showWeather(weather);
    _display->showEvents(events);
    _display->refresh();
}

void DisplayManager::showStatusMessage(const String& msg) {
    if (_display) _display->showStatus(msg);
}

void DisplayManager::showErrorMessage(const String& msg) {
    if (_display) _display->showError(msg);
}

void DisplayManager::showWaitConnection() {
    if (_display) _display->showWaitConnection();
}

bool DisplayManager::isReady() const {
    return _initialized && _display && _display->isReady();
}
