#include "app/AppCoordinator.h"
#include "config.h"
#include <Arduino.h>

AppCoordinator::AppCoordinator(DisplayManager*      display,
                                WeatherManager*      weather,
                                TimeManager*         time,
                                EventManager*        events,
                                ConnectivityManager* connectivity,
                                ISyncService*        sync)
    : _display(display),
      _weather(weather),
      _time(time),
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

    if (_time)    _time->init();
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

    // 2. Process incoming push messages (BLE/WiFi/Bluetooth packets)
    if (_sync) _sync->tick();

    // 3. Poll data sources on their own schedules
    if (_time)    _time->tick();
    if (_weather) _weather->tick();
    if (_events)  _events->tick();

    // 4. Refresh the display on its own schedule
    unsigned long now = millis();
    if (now - _lastDisplayRefreshMs >= DISPLAY_REFRESH_INTERVAL_MS) {
        _updateDisplay();
        _lastDisplayRefreshMs = now;
    }
}

void AppCoordinator::_updateDisplay() {
    if (!_display || !_display->isReady()) return;

    static const std::vector<EventData> emptyEvents;

    const TimeData&               time    = _time    ? _time->getCurrentTime()       : TimeData{};
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
        // A real impl would forward this to WeatherManager's cache.
        // Requires a push-capable WeatherManager extension.
        (void)weather;
        LOG("[App] Weather update received via sync");
    });
}

const DeviceState& AppCoordinator::getState() const {
    return _state;
}
