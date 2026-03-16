#pragma once
#include <Arduino.h>

enum class ConnectivityType : uint8_t {
    None,
    WiFi,
    Bluetooth,
    BLE
};

enum class ConnectivityStatus : uint8_t {
    Disconnected,
    Connecting,
    Connected,
    Error
};

enum class SyncStatus : uint8_t {
    Idle,
    Syncing,
    Success,
    Failed
};

struct DeviceState {
    ConnectivityType   activeConnectivity    = ConnectivityType::None;
    ConnectivityStatus connectivityStatus    = ConnectivityStatus::Disconnected;
    SyncStatus         syncStatus            = SyncStatus::Idle;
    unsigned long      lastWeatherSyncMs     = 0;
    unsigned long      lastEventSyncMs       = 0;
    unsigned long      lastTimeSyncMs        = 0;
    unsigned long      lastDisplayRefreshMs  = 0;
    bool               displayInitialized    = false;
    uint8_t            batteryPercent        = 100;
};
