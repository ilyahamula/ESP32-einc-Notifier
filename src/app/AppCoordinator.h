#pragma once
#include "managers/DisplayManager.h"
#include "managers/WeatherManager.h"
#include "managers/TimeManager.h"
#include "managers/EventManager.h"
#include "managers/ConnectivityManager.h"
#include "interfaces/ISyncService.h"
#include "models/DeviceState.h"

class AppCoordinator {
public:
    AppCoordinator(DisplayManager*      display,
                   WeatherManager*      weather,
                   TimeManager*         time,
                   EventManager*        events,
                   ConnectivityManager* connectivity,
                   ISyncService*        sync);

    void setup();
    void loop();

    const DeviceState& getState() const;

private:
    void _updateDisplay();
    void _registerSyncCallbacks();

    DisplayManager*      _display;
    WeatherManager*      _weather;
    TimeManager*         _time;
    EventManager*        _events;
    ConnectivityManager* _connectivity;
    ISyncService*        _sync;

    DeviceState   _state;
    unsigned long _lastDisplayRefreshMs = 0;
};
