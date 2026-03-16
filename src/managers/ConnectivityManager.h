#pragma once
#include "interfaces/IConnectivity.h"
#include "models/DeviceState.h"

class ConnectivityManager {
public:
    ConnectivityManager(IConnectivity* connectivity,
                        unsigned long  retryDelayMs,
                        uint8_t        maxRetries);

    bool init();
    void tick();

    bool ensureConnected();
    bool               isConnected() const;
    ConnectivityStatus getStatus()   const;
    ConnectivityType   getType()     const;

private:
    IConnectivity* _connectivity;
    unsigned long  _retryDelayMs;
    uint8_t        _maxRetries;
    uint8_t        _retryCount  = 0;
    unsigned long  _lastRetryMs = 0;
};
