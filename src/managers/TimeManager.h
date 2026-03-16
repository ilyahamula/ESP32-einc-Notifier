#pragma once
#include "interfaces/ITimeProvider.h"
#include "models/TimeData.h"

class TimeManager {
public:
    TimeManager(ITimeProvider* provider, unsigned long syncIntervalMs);

    bool init();
    void tick();

    const TimeData& getCurrentTime() const;
    bool            isSynced()       const;

private:
    ITimeProvider* _provider;
    TimeData       _current;
    unsigned long  _syncIntervalMs;
    unsigned long  _lastSyncMs = 0;
};
