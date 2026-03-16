# Architecture Notes — E-Ink Notifier (ESP32)

## Project Structure

```
ESP32-EINC-NOTIFIER/
├── include/
│   └── config.h                    ← pins, intervals, app identity
└── src/
    ├── main.cpp                    ← clean wiring only: stubs → managers → app
    ├── models/                     ← pure data (no logic, no dependencies)
    │   ├── WeatherData.h           ← temperature, humidity, wind, condition
    │   ├── TimeData.h              ← date/time + sync flag
    │   ├── EventData.h             ← reminders/calendar events
    │   └── DeviceState.h           ← connectivity + sync status enums + state
    ├── interfaces/                 ← pure virtual contracts
    │   ├── IDisplay.h
    │   ├── IWeatherProvider.h
    │   ├── ITimeProvider.h
    │   ├── IEventProvider.h
    │   ├── IConnectivity.h
    │   └── ISyncService.h          ← push-based sync with callbacks
    ├── managers/                   ← scheduling + caching + orchestration
    │   ├── DisplayManager          ← owns refresh timing, delegates to IDisplay
    │   ├── WeatherManager          ← interval-based polling, caches last result
    │   ├── TimeManager             ← periodic re-sync, always serves current time
    │   ├── EventManager            ← polls + accepts pushed events (ingestEvents)
    │   └── ConnectivityManager     ← auto-reconnect with retry + backoff
    ├── app/
    │   └── AppCoordinator          ← single orchestration point; setup()/loop()
    └── stubs/                      ← no-op null objects for all interfaces
        ├── NullDisplay.h
        ├── NullWeatherProvider.h
        ├── NullTimeProvider.h
        ├── NullEventProvider.h
        ├── NullConnectivity.h
        └── NullSyncService.h
```

## Extension Pattern

To connect a real library or service, create one file implementing the matching interface.

| What you're adding | Implement                                   | Replace in `main.cpp`    |
|--------------------|---------------------------------------------|--------------------------|
| GxEPD2 display     | `IDisplay` → `GxEPD2Display`                | `NullDisplay`            |
| NTP time sync      | `ITimeProvider` → `NtpTimeProvider`         | `NullTimeProvider`       |
| OpenWeatherMap     | `IWeatherProvider` → `OwmWeatherProvider`   | `NullWeatherProvider`    |
| Wi-Fi              | `IConnectivity` → `WiFiConnectivity`        | `NullConnectivity`       |
| Telegram bot       | `ISyncService` → `TelegramSyncService`      | `NullSyncService`        |
| Google Calendar    | `IEventProvider` → `CalDavEventProvider`    | `NullEventProvider`      |

`main.cpp` never changes — only the concrete object passed to the constructor changes.
