#pragma once
#include "interfaces/IConnectivity.h"

// No-op connectivity — always disconnected.
// Replace with e.g. WiFiConnectivity or BLEConnectivity.
class NullConnectivity : public IConnectivity {
public:
    bool               connect()        override { return false; }
    void               disconnect()     override {}
    bool               isConnected() const override { return false; }
    ConnectivityType   getType()     const override { return ConnectivityType::None; }
    ConnectivityStatus getStatus()   const override { return ConnectivityStatus::Disconnected; }
    String             getIdentifier() const override { return ""; }
};
