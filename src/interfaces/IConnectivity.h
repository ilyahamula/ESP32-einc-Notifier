#pragma once
#include "models/DeviceState.h"

class IConnectivity {
public:
    virtual ~IConnectivity() = default;

    virtual bool connect()    = 0;
    virtual void disconnect() = 0;

    virtual bool               isConnected() const = 0;
    virtual ConnectivityType   getType()     const = 0;
    virtual ConnectivityStatus getStatus()   const = 0;

    // Returns a human-readable identifier: IP address, device name, etc.
    virtual String getIdentifier() const = 0;
};
