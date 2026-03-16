#include <Arduino.h>
#include "config.h"

// ─── Managers ─────────────────────────────────────────────────────────────────
#include "managers/DisplayManager.h"
#include "managers/WeatherManager.h"
#include "managers/TimeManager.h"
#include "managers/EventManager.h"
#include "managers/ConnectivityManager.h"

// ─── Application Coordinator ──────────────────────────────────────────────────
#include "app/AppCoordinator.h"

// ─── Null Stubs (swap these out for real implementations as you develop) ───────
#include "stubs/NullDisplay.h"
#include "stubs/NullWeatherProvider.h"
#include "stubs/NullTimeProvider.h"
#include "stubs/NullEventProvider.h"
#include "stubs/NullConnectivity.h"
#include "stubs/NullSyncService.h"

// ─── Infrastructure Instances ─────────────────────────────────────────────────
static NullDisplay          display;
static NullWeatherProvider  weatherProvider;
static NullTimeProvider     timeProvider;
static NullEventProvider    eventProvider;
static NullConnectivity     connectivity;
static NullSyncService      syncService;

// ─── Manager Instances ────────────────────────────────────────────────────────
static DisplayManager     displayManager(&display);
static WeatherManager     weatherManager(&weatherProvider, WEATHER_UPDATE_INTERVAL_MS);
static TimeManager        timeManager(&timeProvider, TIME_SYNC_INTERVAL_MS);
static EventManager       eventManager(&eventProvider, EVENT_SYNC_INTERVAL_MS);
static ConnectivityManager connectivityManager(&connectivity,
                                               CONNECTIVITY_RETRY_DELAY_MS,
                                               CONNECTIVITY_MAX_RETRIES);

// ─── Application Coordinator ──────────────────────────────────────────────────
static AppCoordinator app(
    &displayManager,
    &weatherManager,
    &timeManager,
    &eventManager,
    &connectivityManager,
    &syncService
);

// ─── Arduino Entry Points ──────────────────────────────────────────────────────
void setup() {
    Serial.begin(SERIAL_BAUD_RATE);
    app.setup();
}

void loop() {
    app.loop();
}
