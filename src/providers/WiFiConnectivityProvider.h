#pragma once
#include "interfaces/IConnectivity.h"

class WiFiConnectivityProvider : public IConnectivity {
public:
    // ssid and password must outlive this object (string literals are fine).
    WiFiConnectivityProvider(const char* ssid, const char* password,
                             uint32_t timeoutMs = 10000);

    bool connect()    override;
    void disconnect() override;

    bool               isConnected()   const override;
    ConnectivityType   getType()       const override;
    ConnectivityStatus getStatus()     const override;
    String             getIdentifier() const override;

private:
    const char*        _ssid;
    const char*        _password;
    uint32_t           _timeoutMs;
    ConnectivityStatus _status = ConnectivityStatus::Disconnected;
};
