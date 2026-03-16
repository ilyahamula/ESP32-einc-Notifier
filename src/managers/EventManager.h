#pragma once
#include "interfaces/IEventProvider.h"
#include "models/EventData.h"
#include <vector>

class EventManager {
public:
    EventManager(IEventProvider* provider, unsigned long syncIntervalMs);

    bool init();
    void tick();

    const std::vector<EventData>& getUpcomingEvents() const;

    // Merge a pushed batch of events (e.g. received from ISyncService)
    void ingestEvents(const std::vector<EventData>& events);

    bool hasEvents() const;

private:
    IEventProvider*        _provider;
    std::vector<EventData> _events;
    unsigned long          _syncIntervalMs;
    unsigned long          _lastSyncMs = 0;
};
