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

// ─── Real implementations ─────────────────────────────────────────────────────
#include "display/Lyligo_4_7_e_paper.h"
#include "providers/Lyligo_4_7_e_paper_TimeProvider.h"

// ─── Fake stubs (provide sample data without network/hardware) ────────────────
#include "stubs/FakeWeatherProvider.h"
#include "stubs/FakeEventProvider.h"
#include "stubs/FakeConnectivity.h"
#include "stubs/NullSyncService.h"

// ─── Infrastructure Instances ─────────────────────────────────────────────────
static Lyligo_4_7_e_paper              display;
static Lyligo_4_7_e_paper_TimeProvider timeProvider;
static FakeWeatherProvider             weatherProvider;
static FakeEventProvider               eventProvider;
static FakeConnectivity                connectivity;
static NullSyncService                 syncService;

// ─── Manager Instances ────────────────────────────────────────────────────────
static DisplayManager      displayManager(&display);
static WeatherManager      weatherManager(&weatherProvider, WEATHER_UPDATE_INTERVAL_MS);
static EventManager        eventManager(&eventProvider, EVENT_SYNC_INTERVAL_MS);
static ConnectivityManager connectivityManager(&connectivity,
                                               CONNECTIVITY_RETRY_DELAY_MS,
                                               CONNECTIVITY_MAX_RETRIES);

// ─── Application Coordinator ──────────────────────────────────────────────────
static AppCoordinator app(
    &displayManager,
    &weatherManager,
    &eventManager,
    &connectivityManager,
    &syncService
);

// ─── Arduino Entry Points ──────────────────────────────────────────────────────
void setup() {
#ifdef DEBUG_SERIAL
    Serial.begin(SERIAL_BAUD_RATE);
    delay(1000);
#endif

    if (!timeProvider.begin()) {
        LOG("[WARN] RTC chip not found — time unavailable");
    }

    TimeManager::instance().setProvider(&timeProvider);

    app.setup();
}

void loop() {
    app.loop();
}
