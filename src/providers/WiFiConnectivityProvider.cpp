#include "providers/WiFiConnectivityProvider.h"
#include "config.h"
#include <WiFi.h>

WiFiConnectivityProvider::WiFiConnectivityProvider(const char* ssid,
                                                   const char* password,
                                                   uint32_t    timeoutMs)
    : _ssid(ssid), _password(password), _timeoutMs(timeoutMs) {}

bool WiFiConnectivityProvider::connect() {
    _status = ConnectivityStatus::Connecting;
    LOG_F("[WiFi] Connecting to %s ...\n", _ssid);

    WiFi.mode(WIFI_STA);
    WiFi.begin(_ssid, _password);

    const uint32_t deadline = millis() + _timeoutMs;
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() > deadline) {
            _status = ConnectivityStatus::Error;
            LOG("[WiFi] Connection timed out");
            WiFi.disconnect(true);
            return false;
        }
        delay(200);
    }

    _status = ConnectivityStatus::Connected;
    LOG_F("[WiFi] Connected. IP: %s\n", WiFi.localIP().toString().c_str());
    return true;
}

void WiFiConnectivityProvider::disconnect() {
    WiFi.disconnect(true);
    _status = ConnectivityStatus::Disconnected;
    LOG("[WiFi] Disconnected");
}

bool WiFiConnectivityProvider::isConnected() const {
    return WiFi.status() == WL_CONNECTED;
}

ConnectivityType WiFiConnectivityProvider::getType() const {
    return ConnectivityType::WiFi;
}

ConnectivityStatus WiFiConnectivityProvider::getStatus() const {
    // Keep the stored status authoritative during connect/disconnect transitions;
    // once connected, mirror the live WiFi status so drops are detected.
    if (_status == ConnectivityStatus::Connected && WiFi.status() != WL_CONNECTED)
        return ConnectivityStatus::Disconnected;
    return _status;
}

String WiFiConnectivityProvider::getIdentifier() const {
    if (WiFi.status() == WL_CONNECTED)
        return WiFi.localIP().toString();
    return String(_ssid);
}
