#pragma once
#include "interfaces/IEventProvider.h"

// No-op provider — always empty.
// Replace with e.g. TelegramEventProvider, CalDavProvider, or BleEventProvider.
class NullEventProvider : public IEventProvider {
public:
    bool fetchEvents(std::vector<EventData>&) override { return false; }
    bool isAvailable() const                  override { return false; }
    void acknowledgeEvent(const String&)      override {}
};
