#pragma once
#include "interfaces/IDisplay.h"
#include "models/WeatherData.h"
#include "models/TimeData.h"
#include "models/EventData.h"
#include <vector>

class DisplayManager {
public:
    explicit DisplayManager(IDisplay* display);

    bool init();

    void update(const WeatherData& weather,
                const TimeData& time,
                const std::vector<EventData>& events);

    void showStatusMessage(const String& msg);
    void showErrorMessage(const String& msg);
    void showWaitConnection();

    bool isReady() const;

private:
    IDisplay* _display;
    bool      _initialized = false;
};
