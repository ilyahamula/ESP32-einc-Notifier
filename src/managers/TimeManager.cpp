#include "managers/TimeManager.h"
#include <Arduino.h>

TimeManager::TimeManager(ITimeProvider* provider, unsigned long syncIntervalMs)
    : _provider(provider), _syncIntervalMs(syncIntervalMs) {}

bool TimeManager::init() {
    if (!_provider) return false;
    bool ok = _provider->sync();
    if (ok) _lastSyncMs = millis();
    return ok;
}

void TimeManager::tick() {
    if (!_provider) return;
    unsigned long now = millis();
    if (_lastSyncMs == 0 || (now - _lastSyncMs) >= _syncIntervalMs) {
        if (_provider->sync()) {
            _lastSyncMs = now;
            Serial.println("[TimeManager] time synced");
        }
    }
    _provider->getTime(_current);
}

const TimeData& TimeManager::getCurrentTime() const {
    return _current;
}

bool TimeManager::isSynced() const {
    return _provider && _provider->isSynced();
}
