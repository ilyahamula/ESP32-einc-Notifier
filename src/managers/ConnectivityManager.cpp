#include "managers/ConnectivityManager.h"
#include "config.h"
#include <Arduino.h>

ConnectivityManager::ConnectivityManager(IConnectivity* connectivity,
                                          unsigned long  retryDelayMs,
                                          uint8_t        maxRetries)
    : _connectivity(connectivity),
      _retryDelayMs(retryDelayMs),
      _maxRetries(maxRetries) {}

bool ConnectivityManager::init() {
    if (!_connectivity) return false;
    return _connectivity->connect();
}

void ConnectivityManager::tick() {
    if (!_connectivity) return;
    if (_connectivity->isConnected()) {
        _retryCount = 0;
        return;
    }
    unsigned long now = millis();
    if (_retryCount < _maxRetries &&
        (now - _lastRetryMs) >= _retryDelayMs) {
        LOG_F("[ConnectivityManager] reconnect attempt %u/%u\n",
              _retryCount + 1, _maxRetries);
        _connectivity->connect();
        _retryCount++;
        _lastRetryMs = now;
    }
}

bool ConnectivityManager::ensureConnected() {
    if (_connectivity && _connectivity->isConnected()) return true;
    return init();
}

bool ConnectivityManager::isConnected() const {
    return _connectivity && _connectivity->isConnected();
}

ConnectivityStatus ConnectivityManager::getStatus() const {
    if (!_connectivity) return ConnectivityStatus::Disconnected;
    return _connectivity->getStatus();
}

ConnectivityType ConnectivityManager::getType() const {
    if (!_connectivity) return ConnectivityType::None;
    return _connectivity->getType();
}
