#pragma once
#include "interfaces/IConnectivity.h"

// Mimics a live WiFi connection without touching any hardware.
// connect() simulates a short handshake delay and always succeeds.
class FakeConnectivity : public IConnectivity {
public:
    bool connect() override {
        _status = ConnectivityStatus::Connecting;
        delay(50);  // simulate handshake
        _status    = ConnectivityStatus::Connected;
        _connected = true;
        return true;
    }

    void disconnect() override {
        _status    = ConnectivityStatus::Disconnected;
        _connected = false;
    }

    bool               isConnected() const override { return _connected; }
    ConnectivityType   getType()     const override { return ConnectivityType::WiFi; }
    ConnectivityStatus getStatus()   const override { return _status; }
    String             getIdentifier() const override { return "192.168.1.100"; }

private:
    bool               _connected = false;
    ConnectivityStatus _status    = ConnectivityStatus::Disconnected;
};
