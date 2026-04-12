#pragma once
#include "interfaces/ITimeProvider.h"
#include "models/TimeData.h"

class TimeManager {
public:
    static TimeManager& instance();

    // Call once in setup() before init().
    void setProvider(ITimeProvider* provider);

    bool init();
    void tick();

    const TimeData& getCurrentTime() const;
    bool            isSynced()       const;

    // Returns true exactly once per minute transition (resets automatically).
    // Also fires on the very first tick after init so the display draws immediately.
    bool minuteChanged();

private:
    TimeManager() = default;
    TimeManager(const TimeManager&)            = delete;
    TimeManager& operator=(const TimeManager&) = delete;

    ITimeProvider* _provider           = nullptr;
    TimeData       _current;
    uint8_t        _lastMinute         = 0xFF; // 0xFF = never ticked
    bool           _minuteFlag         = false;
};
