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
#define TIME_SYNC_INTERVAL_MS        (60UL * 60UL * 1000UL)  // 1 hr
#define EVENT_SYNC_INTERVAL_MS       ( 5UL * 60UL * 1000UL)  // 5 min
#define DISPLAY_REFRESH_INTERVAL_MS  (       60UL * 1000UL)  // 1 min

// ─── Connectivity ─────────────────────────────────────────────────────────────
#define CONNECTIVITY_RETRY_DELAY_MS  5000
#define CONNECTIVITY_MAX_RETRIES     5

// ─── App Identity ─────────────────────────────────────────────────────────────
#define APP_NAME     "E-Ink Notifier"
#define APP_VERSION  "0.1.0"
