#include "display/Lyligo_4_7_e_paper.h"
#include "firasans.h"
#include <algorithm>
#include <cstring>
#include <esp_heap_caps.h>

// ─── string tables ──────────────────────────────────────────────────────────

static const char* kDayNames[] = {
    "", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"
};

static const char* kMonthNames[] = {
    "", "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

static const char* kWindDir[] = {
    "?", "N", "NE", "E", "SE", "S", "SW", "W", "NW"
};

// ─── lifecycle ──────────────────────────────────────────────────────────────

Lyligo_4_7_e_paper::~Lyligo_4_7_e_paper() {
    if (_fb) { heap_caps_free(_fb); _fb = nullptr; }
    if (_powerOn) epd_poweroff_all();
}

bool Lyligo_4_7_e_paper::init() {
    epd_init();

    _fb = static_cast<uint8_t*>(
        heap_caps_malloc(FB_SIZE, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (!_fb) return false;

    memset(_fb, 0xFF, FB_SIZE);  // white canvas

    epd_poweron();
    _powerOn = true;
    epd_clear();
    _ready = true;
    return true;
}

void Lyligo_4_7_e_paper::clear() {
    if (!_ready || !_fb) return;
    memset(_fb, 0xFF, FB_SIZE);
    ensurePowerOn();
    epd_clear();
}

void Lyligo_4_7_e_paper::refresh() {
    if (!_ready || !_fb) return;
    ensurePowerOn();
    epd_draw_grayscale_image(epd_full_screen(), _fb);
    epd_poweroff_all();
    _powerOn = false;
}

bool Lyligo_4_7_e_paper::isReady() const { return _ready; }

// ─── showTime ───────────────────────────────────────────────────────────────
//
//  Panel: x=0, y=0, w=640, h=135  (top-left, 2/3 width × 1/4 height)
//
//  Line 1  y≈82   "14:30"
//  Line 2  y≈126  "Thursday, 10 Apr 2026"
//
void Lyligo_4_7_e_paper::showTime(const TimeData& time) {
    if (!_ready || !_fb) return;
    _currentTime = time;

    clearFbRegion(TIME_X, TIME_Y, TIME_W, TIME_H);

    char timeBuf[8];
    snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d", time.hour, time.minute);

    uint8_t dowIdx = static_cast<uint8_t>(time.weekday);
    const char* dayName = (dowIdx >= 1 && dowIdx <= 7) ? kDayNames[dowIdx] : "";
    const char* monName = (time.month >= 1 && time.month <= 12) ? kMonthNames[time.month] : "???";
    char dateBuf[40];
    snprintf(dateBuf, sizeof(dateBuf), "%s, %02d %s %04d",
             dayName, time.day, monName, time.year);

    int32_t cx, cy;

    cx = TIME_X + 24;  cy = TIME_Y + 82;
    writeln((GFXfont*)&FiraSans, timeBuf, &cx, &cy, _fb);

    cx = TIME_X + 24;  cy = TIME_Y + 126;
    writeln((GFXfont*)&FiraSans, dateBuf, &cx, &cy, _fb);
}

// ─── showWeather ────────────────────────────────────────────────────────────
//
//  Panel: x=640, y=0, w=320, h=135  (top-right, 1/3 width × 1/4 height)
//
//  Line 1  y≈32   location
//  Line 2  y≈74   "22C / feels 20C"
//  Line 3  y≈116  "Wind 15km/h N  Hum 70%"
//
void Lyligo_4_7_e_paper::showWeather(const WeatherData& weather) {
    if (!_ready || !_fb) return;

    clearFbRegion(WEATHER_X, WEATHER_Y, WEATHER_W, WEATHER_H);

    char locBuf[36];
    snprintf(locBuf, sizeof(locBuf), "%.35s", weather.location.c_str());

    char tempBuf[36];
    snprintf(tempBuf, sizeof(tempBuf), "%.0fC / feels %.0fC",
             weather.temperatureCelsius, weather.feelsLikeCelsius);

    uint8_t wdIdx = static_cast<uint8_t>(weather.windDirection);
    const char* windDir = (wdIdx <= 8) ? kWindDir[wdIdx] : "?";
    char windHumBuf[40];
    snprintf(windHumBuf, sizeof(windHumBuf), "Wind %.0fkm/h %s  Hum %.0f%%",
             weather.windSpeedKmh, windDir, weather.humidityPercent);

    int32_t cx, cy;

    cx = WEATHER_X + 12;  cy = WEATHER_Y + 32;
    writeln((GFXfont*)&FiraSans, locBuf, &cx, &cy, _fb);

    cx = WEATHER_X + 12;  cy = WEATHER_Y + 74;
    writeln((GFXfont*)&FiraSans, tempBuf, &cx, &cy, _fb);

    cx = WEATHER_X + 12;  cy = WEATHER_Y + 116;
    writeln((GFXfont*)&FiraSans, windHumBuf, &cx, &cy, _fb);
}

// ─── showEvents ─────────────────────────────────────────────────────────────

void Lyligo_4_7_e_paper::showEvents(const std::vector<EventData>& events) {
    if (!_ready || !_fb) return;
    showUpcomingEvents(events);
    showWeekEvents(events);
}

// ─── showWeekEvents ──────────────────────────────────────────────────────────
//
//  Area: x=480, y=135, w=480, h=405
//
//  Shows the current Mon–Sun week.  Each day occupies a fixed-height row
//  (57 px).  A thin vertical separator divides the day-name column (left)
//  from the event column (right).  A thin horizontal line is drawn between
//  rows.
//
//  Event layout inside a row:
//    1 event  → single rectangle spanning the full event column width.
//    N events → column split into N equal-width rectangles, ordered by time.
//
//  Text inside each box: "Title: description" (baseline 20 px from box top,
//  clamped so it stays within the box).
//
void Lyligo_4_7_e_paper::showWeekEvents(const std::vector<EventData>& events) {
    clearFbRegion(WEEK_X, WEEK_Y, WEEK_W, WEEK_H);

    static const char* kShortDay[] = {
        "", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"
    };

    auto daysInMonth = [](int y, int m) -> int {
        if (m == 2 && (y % 4 == 0 && (y % 100 != 0 || y % 400 == 0))) return 29;
        static const int k[] = {0,31,28,31,30,31,30,31,31,30,31,30,31};
        return k[m];
    };

    // ── Vertical separator ───────────────────────────────────────────────
    for (int32_t y = WEEK_Y; y < WEEK_Y + WEEK_H; ++y) {
        epd_draw_pixel(DAY_SEP_X, y, 0x00, _fb);
    }

    // ── Rows: today (d=0) through today+6 (d=6) ──────────────────────────
    for (int32_t d = 0; d < WEEK_DAYS; ++d) {
        // Calendar date for this row.
        int rowYear  = _currentTime.year;
        int rowMonth = _currentTime.month;
        int rowDay   = _currentTime.day + d;
        while (rowDay > daysInMonth(rowYear, rowMonth)) {
            rowDay -= daysInMonth(rowYear, rowMonth);
            if (++rowMonth > 12) { rowMonth = 1; rowYear++; }
        }
        // Weekday: advance from today's DOW (1=Mon…7=Sun).
        const uint8_t rowDow = static_cast<uint8_t>(
            (static_cast<int>(_currentTime.weekday) - 1 + d) % 7 + 1);

        const int32_t rowY = WEEK_Y + d * DAY_ROW_H;

        // Horizontal row separator (skip the very first to avoid double-line).
        if (d > 0) {
            for (int32_t x = WEEK_X; x < WEEK_X + WEEK_W; ++x) {
                epd_draw_pixel(x, rowY, 0x00, _fb);
            }
        }

        // Day label: two lines — "Mon" on the upper half, "14" on the lower.
        int32_t cx = WEEK_X + 4;
        int32_t cy = rowY + 20;
        writeln((GFXfont*)&FiraSans, kShortDay[rowDow], &cx, &cy, _fb);

        char dayNumBuf[4];
        snprintf(dayNumBuf, sizeof(dayNumBuf), "%d", rowDay);
        cx = WEEK_X + 4;
        cy = rowY + 44;
        writeln((GFXfont*)&FiraSans, dayNumBuf, &cx, &cy, _fb);

        // ── Collect events matching this exact calendar date ──────────────
        std::vector<const EventData*> dayEvents;
        for (const auto& ev : events) {
            if (ev.dateTime.year  != rowYear)  continue;
            if (ev.dateTime.month != rowMonth) continue;
            if (ev.dateTime.day   != rowDay)   continue;
            dayEvents.push_back(&ev);
        }

        if (dayEvents.empty()) continue;

        // Sort by start time.
        std::sort(dayEvents.begin(), dayEvents.end(),
            [](const EventData* a, const EventData* b) {
                if (a->dateTime.hour != b->dateTime.hour)
                    return a->dateTime.hour < b->dateTime.hour;
                return a->dateTime.minute < b->dateTime.minute;
            });

        const int32_t n     = static_cast<int32_t>(dayEvents.size());
        const int32_t slotW = WEEK_EV_W / n;
        const int32_t boxH  = DAY_ROW_H - 2;  // 1 px margin top and bottom

        for (int32_t i = 0; i < n; ++i) {
            const EventData* ev = dayEvents[i];
            const int32_t boxX = WEEK_EV_X + i * slotW;
            const int32_t boxY = rowY + 1;
            const int32_t boxW = (i == n - 1)
                                 ? (WEEK_X + WEEK_W - 4 - boxX)  // last slot fills to edge
                                 : slotW;

            epd_draw_rect(boxX, boxY, boxW, boxH, 0x00, _fb);

            int32_t cy2 = boxY + 20;
            if (cy2 > boxY + boxH - 4) cy2 = boxY + boxH - 4;

            String text = ev->title;
            if (ev->description.length() > 0) {
                text += ": ";
                text += ev->description;
            }
            int32_t cx2 = boxX + 4;
            writeln((GFXfont*)&FiraSans, text.c_str(), &cx2, &cy2, _fb);
        }
    }
}

// ─── showUpcomingEvents ─────────────────────────────────────────────────────
//
//  Area: x=0, y=135, w=480, h=405
//
//  Left column (0..HOUR_SEP_X-1):
//    Thin vertical line at HOUR_SEP_X.
//    Hour labels "HH:00" printed every 2 hours, centred on the tick.
//    Tick marks at every full hour.
//
//  Right column (EVENT_COL_X..479):
//    One bordered rectangle per event that overlaps the ±6 h window.
//    Box height ∝ durationSeconds; top edge aligned to the start minute.
//    Text inside: "title: description"
//
void Lyligo_4_7_e_paper::showUpcomingEvents(const std::vector<EventData>& events) {
    clearFbRegion(UPCOMING_X, UPCOMING_Y, UPCOMING_W, UPCOMING_H);

    const float pixelsPerHour = static_cast<float>(UPCOMING_H) / HOURS_RANGE;
    const int32_t startHour   = static_cast<int32_t>(_currentTime.hour) - 6;
    const int32_t endHour     = startHour + HOURS_RANGE;

    // ── Vertical separator line ──────────────────────────────────────────
    for (int32_t y = UPCOMING_Y; y < UPCOMING_Y + UPCOMING_H; ++y) {
        epd_draw_pixel(HOUR_SEP_X, y, 0x00, _fb);
    }

    // ── Hour ticks and labels ────────────────────────────────────────────
    // Tick every hour, label every 2 hours.
    for (int32_t i = 0; i <= HOURS_RANGE; ++i) {
        const int32_t ty = UPCOMING_Y + static_cast<int32_t>(i * pixelsPerHour);

        // 6-px tick to the right of the separator
        for (int32_t tx = HOUR_SEP_X; tx <= HOUR_SEP_X + 5; ++tx) {
            epd_draw_pixel(tx, ty, 0x00, _fb);
        }

        if (i % 2 == 0) {
            const int32_t absHour     = startHour + i;
            const int32_t displayHour = ((absHour % 24) + 24) % 24;
            char label[6];
            snprintf(label, sizeof(label), "%02d:00", displayHour);

            // Baseline sits ~14 px below the tick so the label straddles it.
            int32_t cx = UPCOMING_X + 4;
            int32_t cy = ty + 14;
            writeln((GFXfont*)&FiraSans, label, &cx, &cy, _fb);
        }
    }

    // ── Event boxes ──────────────────────────────────────────────────────
    for (const auto& ev : events) {

        // Keep only events on the same calendar day as the cached time.
        if (ev.dateTime.year  != _currentTime.year  ||
            ev.dateTime.month != _currentTime.month ||
            ev.dateTime.day   != _currentTime.day) continue;

        const float evStartH = ev.dateTime.hour + ev.dateTime.minute / 60.0f;
        const float evEndH   = evStartH + ev.durationSeconds / 3600.0f;

        // Skip if the event does not overlap the visible window at all.
        if (evEndH < static_cast<float>(startHour) ||
            evStartH > static_cast<float>(endHour)) continue;

        // Top-left corner of the box (may be above the visible area).
        const float relStart = evStartH - static_cast<float>(startHour);
        int32_t boxY = UPCOMING_Y + static_cast<int32_t>(relStart * pixelsPerHour);
        int32_t boxH = static_cast<int32_t>(ev.durationSeconds / 3600.0f * pixelsPerHour);
        if (boxH < MIN_EVENT_H) boxH = MIN_EVENT_H;

        // Clamp to the visible area.
        const int32_t areaBottom = UPCOMING_Y + UPCOMING_H;
        if (boxY < UPCOMING_Y) {
            boxH -= (UPCOMING_Y - boxY);
            boxY  = UPCOMING_Y;
        }
        if (boxY + boxH > areaBottom) boxH = areaBottom - boxY;
        if (boxH <= 0) continue;

        const int32_t boxX = EVENT_COL_X;
        const int32_t boxW = EVENT_COL_W;

        // Border rectangle.
        epd_draw_rect(boxX, boxY, boxW, boxH, 0x00, _fb);

        // Text: "Title: description"  (single line, 4 px left padding).
        // Baseline ~20 px from the top of the box; clamp so it stays inside.
        int32_t cy = boxY + 20;
        if (cy > boxY + boxH - 4) cy = boxY + boxH - 4;

        String text = ev.title;
        if (ev.description.length() > 0) {
            text += ": ";
            text += ev.description;
        }
        int32_t cx = boxX + 4;
        writeln((GFXfont*)&FiraSans, text.c_str(), &cx, &cy, _fb);
    }
}

// ─── showStatus / showError ──────────────────────────────────────────────────

void Lyligo_4_7_e_paper::showStatus(const String& message) {
    if (!_ready || !_fb) return;
    int32_t cx = 10, cy = SCREEN_H - 12;
    writeln((GFXfont*)&FiraSans, message.c_str(), &cx, &cy, _fb);
}

void Lyligo_4_7_e_paper::showError(const String& message) {
    showStatus(message);
}

// ─── private helpers ─────────────────────────────────────────────────────────

void Lyligo_4_7_e_paper::ensurePowerOn() {
    if (!_powerOn) {
        epd_poweron();
        _powerOn = true;
    }
}

// Fill a rectangular region of the framebuffer with white (0xFF).
// x and w must both be even (nibble-aligned); enforced by truncation.
void Lyligo_4_7_e_paper::clearFbRegion(int32_t x, int32_t y, int32_t w, int32_t h) {
    if (!_fb) return;
    const int32_t stride = EPD_WIDTH / 2;
    const int32_t col    = x / 2;
    const int32_t bytes  = w / 2;
    for (int32_t row = y; row < y + h && row < EPD_HEIGHT; ++row) {
        memset(_fb + row * stride + col, 0xFF, bytes);
    }
}
