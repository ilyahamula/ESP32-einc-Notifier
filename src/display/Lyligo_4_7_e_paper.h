#pragma once
#include "interfaces/IDisplay.h"
#include "epd_driver.h"

class Lyligo_4_7_e_paper : public IDisplay {
public:
    Lyligo_4_7_e_paper() = default;
    ~Lyligo_4_7_e_paper();

    bool init()    override;
    void clear()   override;
    void refresh() override;

    void showWeather(const WeatherData& weather)              override;
    void showTime(const TimeData& time)                       override;
    void showEvents(const std::vector<EventData>& events)     override;
    void showStatus(const String& message)                    override;
    void showError(const String& message)                     override;

    bool isReady() const override;

private:
    bool     _ready   = false;
    bool     _powerOn = false;
    uint8_t* _fb      = nullptr;   // full-screen PSRAM framebuffer (EPD_WIDTH/2 * EPD_HEIGHT)
    TimeData _currentTime;

    void ensurePowerOn();
    void clearFbRegion(int32_t x, int32_t y, int32_t w, int32_t h); 
    void showUpcomingEvents(const std::vector<EventData>& events);
    void showWeekEvents(const std::vector<EventData>& events);

    // ── Screen ────────────────────────────────────────────────────────────
    static constexpr int32_t SCREEN_W = EPD_WIDTH;   // 960
    static constexpr int32_t SCREEN_H = EPD_HEIGHT;  // 540
    static constexpr int32_t FB_SIZE  = (EPD_WIDTH / 2) * EPD_HEIGHT; // 259 200 bytes

    // ── Date/Time panel — top-left, 2/3 width × 1/4 height ───────────────
    static constexpr int32_t TIME_X = 0;
    static constexpr int32_t TIME_Y = 0;
    static constexpr int32_t TIME_W = SCREEN_W * 2 / 3;   // 640
    static constexpr int32_t TIME_H = SCREEN_H / 4;        // 135

    // ── Weather panel — top-right, 1/3 width × 1/4 height ────────────────
    static constexpr int32_t WEATHER_X = TIME_W;
    static constexpr int32_t WEATHER_Y = 0;
    static constexpr int32_t WEATHER_W = SCREEN_W / 3;    // 320
    static constexpr int32_t WEATHER_H = SCREEN_H / 4;    // 135

    // ── Events area — lower 3/4, full width ──────────────────────────────
    static constexpr int32_t EVENTS_Y = TIME_H;            // 135
    static constexpr int32_t EVENTS_H = SCREEN_H - TIME_H; // 405

    // ── Upcoming events — left half of events area ────────────────────────
    static constexpr int32_t UPCOMING_X = 0;
    static constexpr int32_t UPCOMING_Y = EVENTS_Y;        // 135
    static constexpr int32_t UPCOMING_W = SCREEN_W / 2;    // 480
    static constexpr int32_t UPCOMING_H = EVENTS_H;        // 405

    // ── Hours timeline ───────────────────────────────────────────────────
    // 12 slots total (current−6 … current+6), ~33.75 px/slot
    static constexpr int32_t HOURS_RANGE  = 12;
    static constexpr int32_t HOUR_COL_W  = 80;   // room for "23:00" in FontSmall (10pt)
    static constexpr int32_t HOUR_SEP_X  = UPCOMING_X + HOUR_COL_W + 2;
    static constexpr int32_t EVENT_COL_X = HOUR_SEP_X + 4;
    static constexpr int32_t EVENT_COL_W = UPCOMING_W - EVENT_COL_X - 4;
    static constexpr int32_t MIN_EVENT_H = 24;    // px minimum box height

    // ── Week events — right half of events area ───────────────────────────
    static constexpr int32_t WEEK_X      = SCREEN_W / 2;  // 480
    static constexpr int32_t WEEK_Y      = EVENTS_Y;      // 135
    static constexpr int32_t WEEK_W      = SCREEN_W / 2;  // 480
    static constexpr int32_t WEEK_H      = EVENTS_H;      // 405
    static constexpr int32_t WEEK_DAYS   = 7;
    static constexpr int32_t DAY_COL_W   = 100;           // enough for "Mon", "Thu", etc.
    static constexpr int32_t DAY_SEP_X   = WEEK_X + DAY_COL_W + 2;
    static constexpr int32_t DAY_ROW_H   = WEEK_H / WEEK_DAYS; // 57 px per day row
    static constexpr int32_t WEEK_EV_X   = DAY_SEP_X + 4;
    static constexpr int32_t WEEK_EV_W   = WEEK_X + WEEK_W - WEEK_EV_X - 4;
};
