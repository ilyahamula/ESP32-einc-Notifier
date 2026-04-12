#include "managers/TimeManager.h"
#include "config.h"
#include <Arduino.h>

TimeManager& TimeManager::instance() {
    static TimeManager inst;
    return inst;
}

void TimeManager::setProvider(ITimeProvider* provider) {
    _provider = provider;
}

bool TimeManager::init() {
    if (!_provider) return false;
    bool ok = _provider->sync();
    if (ok)
        _provider->getTime(_current);
    return ok;
}

void TimeManager::tick() {
    if (!_provider) return;

    _provider->sync();
    _provider->getTime(_current);

    if (_current.minute != _lastMinute) {
        _lastMinute  = _current.minute;
        _minuteFlag  = true;
        LOG_F("[TimeManager] minute → %02u:%02u\n", _current.hour, _current.minute);
    }
}

bool TimeManager::minuteChanged() {
    if (_minuteFlag) {
        _minuteFlag = false;
        return true;
    }
    return false;
}

const TimeData& TimeManager::getCurrentTime() const {
    return _current;
}

bool TimeManager::isSynced() const {
    return _provider && _provider->isSynced();
}
