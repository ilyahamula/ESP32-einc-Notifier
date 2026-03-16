#pragma once
#include "models/EventData.h"
#include <vector>

class IEventProvider {
public:
    virtual ~IEventProvider() = default;

    virtual bool fetchEvents(std::vector<EventData>& out) = 0;
    virtual bool isAvailable() const                      = 0;
    virtual void acknowledgeEvent(const String& eventId)  = 0;
};
