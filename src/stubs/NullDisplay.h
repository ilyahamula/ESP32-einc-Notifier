#pragma once
#include "interfaces/IDisplay.h"

// No-op display — compiles and runs safely without any real hardware attached.
// Replace with a concrete GxEPD2Display class when implementing the display layer.
class NullDisplay : public IDisplay {
public:
    bool init()    override { return true; }
    void clear()   override {}
    void refresh() override {}

    void showWeather(const WeatherData&)           override {}
    void showTime(const TimeData&)                 override {}
    void showEvents(const std::vector<EventData>&) override {}
    void showStatus(const String&)                 override {}
    void showError(const String&)                  override {}
    void showWaitConnection()                       override {}

    bool isReady() const override { return true; }
};
