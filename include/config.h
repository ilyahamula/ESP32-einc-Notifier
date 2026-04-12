#pragma once

// ─── E-Ink Display SPI Pins ───────────────────────────────────────────────────
#define PIN_EPD_CS    5
#define PIN_EPD_DC    17
#define PIN_EPD_RST   16
#define PIN_EPD_BUSY  4

// ─── Serial ───────────────────────────────────────────────────────────────────
#define SERIAL_BAUD_RATE  115200

// ─── Display ──────────────────────────────────────────────────────────────────
#define DISPLAY_ROTATION   1    // 1 = landscape (296 x 128)
#define DISPLAY_WIDTH_PX   296
#define DISPLAY_HEIGHT_PX  128

// ─── Update Intervals (milliseconds) ─────────────────────────────────────────
#define WEATHER_UPDATE_INTERVAL_MS   (10UL * 60UL * 1000UL)  // 10 min
#define EVENT_SYNC_INTERVAL_MS       (       60UL * 1000UL)  // 1 min
// Display is refreshed on each wall-clock minute change (see TimeManager).
// RTC hardware re-read interval is defined in TimeManager::kHardwareSyncMs.

// ─── Connectivity ─────────────────────────────────────────────────────────────
#define CONNECTIVITY_RETRY_DELAY_MS  5000
#define CONNECTIVITY_MAX_RETRIES     5

// ─── App Identity ─────────────────────────────────────────────────────────────
#define APP_NAME     "E-Ink Notifier"
#define APP_VERSION  "0.1.0"

// ─── Serial / Debug ───────────────────────────────────────────────────────────
// Comment out to disable all serial logging and Serial.begin() in production.
#define DEBUG_SERIAL

#define SERIAL_BAUD_RATE 115200

#ifdef DEBUG_SERIAL
  #define LOG(msg)          Serial.println(msg)
  #define LOG_F(fmt, ...)   Serial.printf(fmt, ##__VA_ARGS__)
#else
  #define LOG(msg)          ((void)0)
  #define LOG_F(fmt, ...)   ((void)0)
#endif
