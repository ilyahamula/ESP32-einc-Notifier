#include "managers/EventManager.h"
#include "config.h"
#include <Arduino.h>

EventManager::EventManager(IEventProvider* provider, unsigned long syncIntervalMs)
    : _provider(provider), _syncIntervalMs(syncIntervalMs) {}

bool EventManager::init() {
    return true;
}

void EventManager::tick() {
    if (!_provider || !_provider->isAvailable()) return;
    unsigned long now = millis();
    if (_lastSyncMs == 0 || (now - _lastSyncMs) >= _syncIntervalMs) {
        _provider->fetchEvents(_events);
        _lastSyncMs = now;
        LOG_F("[EventManager] fetched %u events\n", _events.size());
    }
}

const std::vector<EventData>& EventManager::getUpcomingEvents() const {
    return _events;
}

void EventManager::ingestEvents(const std::vector<EventData>& incoming) {
    for (const auto& evt : incoming) {
        bool found = false;
        for (auto& existing : _events) {
            if (existing.id == evt.id) {
                existing = evt;
                found = true;
                break;
            }
        }
        if (!found) {
            _events.push_back(evt);
        }
    }
}

bool EventManager::hasEvents() const {
    return !_events.empty();
}
