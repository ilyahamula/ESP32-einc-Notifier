#include "app/AppCoordinator.h"
#include "config.h"
#include <Arduino.h>

AppCoordinator::AppCoordinator(DisplayManager*      display,
                                WeatherManager*      weather,
                                EventManager*        events,
                                ConnectivityManager* connectivity,
                                ISyncService*        sync)
    : _display(display),
      _weather(weather),
      _events(events),
      _connectivity(connectivity),
      _sync(sync) {}

void AppCoordinator::setup() {
    LOG("[App] " APP_NAME " v" APP_VERSION " starting");

    if (_connectivity) {
        _connectivity->init();
        _state.activeConnectivity = _connectivity->getType();
        _state.connectivityStatus = _connectivity->getStatus();
    }

    if (_display) {
        _state.displayInitialized = _display->init();
        if (!_state.displayInitialized) {
            LOG("[App] WARNING: display init failed");
        }
    }

    if (_sync) {
        _sync->init();
        _registerSyncCallbacks();
    }

    TimeManager::instance().init();
    if (_weather) _weather->init();
    if (_events)  _events->init();

    LOG("[App] Setup complete");
}

void AppCoordinator::loop() {
    // 1. Keep connection alive / retry if dropped
    if (_connectivity) {
        _connectivity->tick();
        _state.connectivityStatus = _connectivity->getStatus();
    }

    // 2. Process incoming push messages
    if (_sync) _sync->tick();

    // 3. Poll data sources
    TimeManager::instance().tick();
    if (_weather) _weather->tick();
    if (_events)  _events->tick();

    // 4. Refresh display on each new minute (aligned to the wall clock).
    if (TimeManager::instance().minuteChanged()) {
        _updateDisplay();
    }
}

void AppCoordinator::_updateDisplay() {
    if (!_display || !_display->isReady()) return;

    static const std::vector<EventData> emptyEvents;

    const TimeData&               time    = TimeManager::instance().getCurrentTime();
    const WeatherData&            weather = _weather ? _weather->getCurrentWeather() : WeatherData{};
    const std::vector<EventData>& events  = _events  ? _events->getUpcomingEvents()  : emptyEvents;

    _display->update(weather, time, events);
    _state.lastDisplayRefreshMs = millis();
}

void AppCoordinator::_registerSyncCallbacks() {
    if (!_sync) return;

    _sync->onEventsReceived([this](const std::vector<EventData>& events) {
        if (_events) _events->ingestEvents(events);
        LOG_F("[App] Received %u events via sync\n", events.size());
    });

    _sync->onWeatherReceived([this](const WeatherData& weather) {
        (void)weather;
        LOG("[App] Weather update received via sync");
    });
}

const DeviceState& AppCoordinator::getState() const {
    return _state;
}
