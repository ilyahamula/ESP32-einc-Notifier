#include "display/Lyligo_4_7_e_paper.h"
#include "fonts/FontSmall.h"    // SF Pro Text  Regular 10pt — advance_y=25  asc=20  desc=6
#include "fonts/FontMedium.h"   // SF Pro Text  Regular 14pt — advance_y=35  asc=28  desc=8
#include "fonts/FontClock.h"    // SF Pro Display Bold  32pt — advance_y=80  asc=64  desc=17
#include "fonts/FontDate.h"     // SF Pro Display Reg   14pt — advance_y=35  asc=28  desc=8
#include <algorithm>
#include <cstring>
#include <esp_heap_caps.h>

// ─── string tables ──────────────────────────────────────────────────────────

static const char* kDayNames[] = {
    "", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"
};

static const char* kShortDayNames[] = {
    "", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"
};

static const char* kMonthNames[] = {
    "", "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

static const char* kWindDir[] = {
    "?", "N", "NE", "E", "SE", "S", "SW", "W", "NW"
};

// ─── Font metrics ─────────────────────────────────────────────────────────────
//
//  All values in pixels.  Asc = ascender (above baseline), Desc = |descender|
//  (below baseline), H = total glyph height, Adv = advance_y (line spacing).
//
//  Baseline placement rule:
//    cy = areaTop + Asc + kPad
//    → glyph top    = areaTop + kPad          (never bleeds above area)
//    → glyph bottom = areaTop + kPad + H
//
//  Minimum area height for one line:  H + 2*kPad
//  Next line's baseline:              cy + Adv  (advances past both glyph + gap)

static constexpr int32_t kPad = 3;  // universal top/bottom padding inside boxes

// FontSmall  (10pt SF Pro Text Regular)
static constexpr int32_t kSmAsc  = 20, kSmDesc =  6, kSmH  = 26, kSmAdv  = 25;
// FontMedium (14pt SF Pro Text Regular)
static constexpr int32_t kMdAsc  = 28, kMdDesc =  8, kMdH  = 36, kMdAdv  = 35;
// FontClock  (32pt SF Pro Display Bold)
static constexpr int32_t kClAsc  = 64, kClDesc = 17, kClH  = 81; // kClAdv unused
// FontDate   (14pt SF Pro Display Regular) — identical metrics to FontMedium
static constexpr int32_t kDtAsc  = 28, kDtDesc =  8;

// ─── Text helpers ─────────────────────────────────────────────────────────────

static int32_t textWidth(const GFXfont* font, const char* str) {
    int32_t x = 0, y = 0, x1, y1, w, h;
    get_text_bounds(font, str, &x, &y, &x1, &y1, &w, &h, nullptr);
    return x1 + w;
}

static String fitText(const GFXfont* font, const String& text, int32_t maxW) {
    if (textWidth(font, text.c_str()) <= maxW) return text;
    String t = text;
    while (t.length() > 1) {
        t.remove(t.length() - 1);
        String candidate = t + "..";
        if (textWidth(font, candidate.c_str()) <= maxW) return candidate;
    }
    return "..";
}

static int wordCount(const String& s) {
    int n = 0; bool inWord = false;
    for (unsigned i = 0; i < s.length(); ++i) {
        if (s[i] != ' ') { if (!inWord) { n++; inWord = true; } }
        else               { inWord = false; }
    }
    return n;
}

// ─── Lifecycle ───────────────────────────────────────────────────────────────

Lyligo_4_7_e_paper::~Lyligo_4_7_e_paper() {
    if (_fb) { heap_caps_free(_fb); _fb = nullptr; }
    if (_powerOn) epd_poweroff_all();
}

bool Lyligo_4_7_e_paper::init() {
    epd_init();
    _fb = static_cast<uint8_t*>(
        heap_caps_malloc(FB_SIZE, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (!_fb) return false;
    memset(_fb, 0xFF, FB_SIZE);
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

// ─── showTime ────────────────────────────────────────────────────────────────
//
//  Panel: x=0, y=0, w=640, h=135
//  Font: FontClock (32pt Bold) for time, FontDate (14pt) for date.
//
//  Line 1 — "14:30" (FontClock, asc=64, desc=17):
//    cy1 = TIME_Y + 64 + kPad = 67
//    glyph top=3  bottom=67+17=84
//  Line 2 — "Thursday, 10 Apr 2026" (FontDate, asc=28, desc=8):
//    cy2 = 84 + kPad + 28 = 84 + 4 + 28 = 116
//    glyph top=88  bottom=116+8=124  (< 135 ✓, 11px margin)
//
void Lyligo_4_7_e_paper::showTime(const TimeData& time) {
    if (!_ready || !_fb) return;
    _currentTime = time;

    clearFbRegion(TIME_X, TIME_Y, TIME_W, TIME_H);
    drawBorder(TIME_X, TIME_Y, TIME_W, TIME_H);

    char timeBuf[8];
    snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d", time.hour, time.minute);

    uint8_t dowIdx = static_cast<uint8_t>(time.weekday);
    const char* dayName = (dowIdx >= 1 && dowIdx <= 7) ? kDayNames[dowIdx] : "";
    const char* monName = (time.month >= 1 && time.month <= 12) ? kMonthNames[time.month] : "???";
    char dateBuf[44];
    snprintf(dateBuf, sizeof(dateBuf), "%s, %02d %s %04d",
             dayName, time.day, monName, time.year);

    // Line 1: clock (FontClock).
    int32_t cy1 = TIME_Y + kClAsc + kPad;   // 67
    int32_t cx1 = TIME_X + 20;
    writeln((GFXfont*)&FontClock, timeBuf, &cx1, &cy1, _fb);

    // Line 2: date (FontDate) — placed below the clock glyph bottom + kPad.
    int32_t cy2 = TIME_Y + kClAsc + kPad + kClDesc + kPad + kDtAsc;  // 116
    int32_t cx2 = TIME_X + 20;
    String dateStr = fitText(&FontDate, String(dateBuf), TIME_W - 40);
    writeln((GFXfont*)&FontDate, dateStr.c_str(), &cx2, &cy2, _fb);
}

// ─── showWeather ──────────────────────────────────────────────────────────────
//
//  Panel: x=640, y=0, w=320, h=135
//  Font: FontMedium (14pt, asc=28, desc=8, adv=35) for all 3 lines.
//
//  cy1 = 28+3  = 31  → top=3,  bot=39
//  cy2 = 31+35 = 66  → top=38, bot=74
//  cy3 = 66+35 = 101 → top=73, bot=109  (< 135 ✓, 26px margin)
//
void Lyligo_4_7_e_paper::showWeather(const WeatherData& weather) {
    if (!_ready || !_fb) return;
    clearFbRegion(WEATHER_X, WEATHER_Y, WEATHER_W, WEATHER_H);
    drawBorder(WEATHER_X, WEATHER_Y, WEATHER_W, WEATHER_H);

    const int32_t usableW = WEATHER_W - 24;

    char locBuf[36];
    snprintf(locBuf, sizeof(locBuf), "%.35s", weather.location.c_str());
    String loc = fitText(&FontMedium, String(locBuf), usableW);

    char tempBuf[36];
    snprintf(tempBuf, sizeof(tempBuf), "%.0fC / feels %.0fC",
             weather.temperatureCelsius, weather.feelsLikeCelsius);
    String temp = fitText(&FontMedium, String(tempBuf), usableW);

    uint8_t wdIdx = static_cast<uint8_t>(weather.windDirection);
    const char* windDir = (wdIdx <= 8) ? kWindDir[wdIdx] : "?";
    char windHumBuf[40];
    snprintf(windHumBuf, sizeof(windHumBuf), "Wind %.0fkm/h %s  Hum %.0f%%",
             weather.windSpeedKmh, windDir, weather.humidityPercent);
    String wind = fitText(&FontMedium, String(windHumBuf), usableW);

    int32_t cy = WEATHER_Y + kMdAsc + kPad;  // 31
    int32_t cx;

    cx = WEATHER_X + 12;  writeln((GFXfont*)&FontMedium, loc.c_str(),  &cx, &cy, _fb);
    cy += kMdAdv; cx = WEATHER_X + 12;  writeln((GFXfont*)&FontMedium, temp.c_str(), &cx, &cy, _fb);
    cy += kMdAdv; cx = WEATHER_X + 12;  writeln((GFXfont*)&FontMedium, wind.c_str(), &cx, &cy, _fb);
}

// ─── showEvents ──────────────────────────────────────────────────────────────

void Lyligo_4_7_e_paper::showEvents(const std::vector<EventData>& events) {
    if (!_ready || !_fb) return;
    showUpcomingEvents(events);
    showWeekEvents(events);
}

// ─── showWeekEvents ───────────────────────────────────────────────────────────
//
//  Area: x=480, y=135, w=480, h=405   7 rows × 57px each.
//  Day label column (100px): FontSmall — two lines "Mon" / "12"
//    "Mon": cy = rowY + kSmAsc + kPad = rowY+23  top=rowY+3   bot=rowY+29
//    "12":  cy = rowY + 23 + kSmAdv  = rowY+48  top=rowY+28  bot=rowY+54 < rowY+57 ✓
//
//  Event box (boxH = 55px): FontSmall title only.
//    Min for text: kSmH + 2*kPad = 32px ≤ 55 ✓
//    cy = boxY + kSmAsc + kPad = boxY+23  bot=boxY+29
//    Title truncated with ".." if it doesn't fit box width.
//
void Lyligo_4_7_e_paper::showWeekEvents(const std::vector<EventData>& events) {
    clearFbRegion(WEEK_X, WEEK_Y, WEEK_W, WEEK_H);
    drawBorder(WEEK_X, WEEK_Y, WEEK_W, WEEK_H);

    auto daysInMonth = [](int y, int m) -> int {
        if (m == 2 && (y % 4 == 0 && (y % 100 != 0 || y % 400 == 0))) return 29;
        static const int k[] = {0,31,28,31,30,31,30,31,31,30,31,30,31};
        return k[m];
    };

    for (int32_t y = WEEK_Y; y < WEEK_Y + WEEK_H; ++y)
        epd_draw_pixel(DAY_SEP_X, y, 0x00, _fb);

    for (int32_t d = 0; d < WEEK_DAYS; ++d) {
        int rowYear  = _currentTime.year;
        int rowMonth = _currentTime.month;
        int rowDay   = _currentTime.day + d;
        while (rowDay > daysInMonth(rowYear, rowMonth)) {
            rowDay -= daysInMonth(rowYear, rowMonth);
            if (++rowMonth > 12) { rowMonth = 1; rowYear++; }
        }
        const uint8_t rowDow = static_cast<uint8_t>(
            (static_cast<int>(_currentTime.weekday) - 1 + d) % 7 + 1);
        const int32_t rowY = WEEK_Y + d * DAY_ROW_H;

        if (d > 0) {
            for (int32_t x = WEEK_X; x < WEEK_X + WEEK_W; ++x)
                epd_draw_pixel(x, rowY, 0x00, _fb);
        }

        // Day label — two lines: "Mon" then "12" (FontSmall).
        const uint8_t dowIdx = (rowDow >= 1 && rowDow <= 7) ? rowDow : 1;
        int32_t lx, ly;

        lx = WEEK_X + 4;  ly = rowY + kSmAsc + kPad;
        writeln((GFXfont*)&FontSmall, kShortDayNames[dowIdx], &lx, &ly, _fb);

        char dayNumBuf[4];
        snprintf(dayNumBuf, sizeof(dayNumBuf), "%d", rowDay);
        lx = WEEK_X + 4;  ly = rowY + kSmAsc + kPad + kSmAdv;  // rowY+48
        writeln((GFXfont*)&FontSmall, dayNumBuf, &lx, &ly, _fb);

        // Collect events for this exact date.
        std::vector<const EventData*> dayEvents;
        for (const auto& ev : events) {
            if (ev.dateTime.year != rowYear || ev.dateTime.month != rowMonth ||
                ev.dateTime.day  != rowDay) continue;
            dayEvents.push_back(&ev);
        }
        if (dayEvents.empty()) continue;

        std::sort(dayEvents.begin(), dayEvents.end(),
            [](const EventData* a, const EventData* b) {
                if (a->dateTime.hour != b->dateTime.hour)
                    return a->dateTime.hour < b->dateTime.hour;
                return a->dateTime.minute < b->dateTime.minute;
            });

        const int32_t n     = static_cast<int32_t>(dayEvents.size());
        const int32_t slotW = WEEK_EV_W / n;
        const int32_t boxH  = DAY_ROW_H - 2;  // 55px

        for (int32_t i = 0; i < n; ++i) {
            const EventData* ev = dayEvents[i];
            const int32_t boxX = WEEK_EV_X + i * slotW;
            const int32_t boxY = rowY + 1;
            const int32_t boxW = (i == n - 1)
                                 ? (WEEK_X + WEEK_W - 4 - boxX)
                                 : slotW;

            epd_draw_rect(boxX, boxY, boxW, boxH, 0x00, _fb);

            // Title in FontSmall — fits when boxH ≥ kSmH + 2*kPad = 32px.
            const int32_t textMaxW = boxW - 2 * kPad - 2;
            if (textMaxW > 0 && boxH >= kSmH + 2 * kPad) {
                String label = fitText(&FontSmall, ev->title, textMaxW);
                int32_t tcx = boxX + kPad + 1;
                int32_t tcy = boxY + kSmAsc + kPad;
                writeln((GFXfont*)&FontSmall, label.c_str(), &tcx, &tcy, _fb);
            }
        }
    }
}

// ─── showUpcomingEvents ───────────────────────────────────────────────────────
//
//  Area: x=0, y=135, w=480, h=405   pph ≈ 67.5px/hour (6-hour window).
//
//  View: [currentTime … currentTime+6h], top = now (with minutes), pph = 405/6.
//
//  Hour labels (FontSmall, asc=20, desc=6):
//    Centre on tick: labelCy = ty + (kSmAsc−kSmDesc)/2 = ty+7
//    Guard: skip if glyph bleeds outside area.
//
//  Event boxes — mixed fonts:
//    Line 1 title  (FontMedium): needs kMdH + 2*kPad = 42px
//    Line 2 time   (FontSmall):  total ≥ 71px
//    Line 3 desc   (FontSmall):  total ≥ 96px, desc ≥ 2 words
//
void Lyligo_4_7_e_paper::showUpcomingEvents(const std::vector<EventData>& events) {
    clearFbRegion(UPCOMING_X, UPCOMING_Y, UPCOMING_W, UPCOMING_H);
    drawBorder(UPCOMING_X, UPCOMING_Y, UPCOMING_W, UPCOMING_H);

    static constexpr float kViewHours = 6.0f;
    const float pph       = static_cast<float>(UPCOMING_H) / kViewHours;
    const float startTime = _currentTime.hour + _currentTime.minute / 60.0f;
    const float endTime   = startTime + kViewHours;

    for (int32_t y = UPCOMING_Y; y < UPCOMING_Y + UPCOMING_H; ++y)
        epd_draw_pixel(HOUR_SEP_X, y, 0x00, _fb);

    // Tick + label for every whole hour within the view.
    const int32_t firstHour = static_cast<int32_t>(startTime) +
                              (startTime > static_cast<float>(static_cast<int32_t>(startTime)) ? 1 : 0);
    for (int32_t h = firstHour; static_cast<float>(h) <= endTime; ++h) {
        const int32_t ty = UPCOMING_Y + static_cast<int32_t>((h - startTime) * pph);

        for (int32_t tx = HOUR_SEP_X; tx <= HOUR_SEP_X + 5; ++tx)
            epd_draw_pixel(tx, ty, 0x00, _fb);

        // Vertically centre FontSmall label on tick.
        const int32_t labelCy  = ty + (kSmAsc - kSmDesc) / 2;
        const int32_t glyphTop = labelCy - kSmAsc;
        if (glyphTop < UPCOMING_Y)                        continue;
        if (labelCy + kSmDesc > UPCOMING_Y + UPCOMING_H) continue;

        const int32_t dispHour = ((h % 24) + 24) % 24;
        char label[6];
        snprintf(label, sizeof(label), "%02d:00", dispHour);
        int32_t cx = UPCOMING_X + 4, cy = labelCy;
        writeln((GFXfont*)&FontSmall, label, &cx, &cy, _fb);
    }

    // Event text thresholds (derived from font metrics above):
    // kMinTitle = kMdH + 2*kPad = 36+6 = 42px  (title line fits)
    // titleBottom offset from boxY = kPad + kMdAsc + kMdDesc = 3+28+8 = 39px
    // cy2 offset from boxY         = 39 + kPad + kSmAsc     = 39+3+20 = 62px
    // time bottom offset           = 62 + kSmDesc            = 68px  → need boxH ≥ 68+kPad = 71
    // cy3 offset from boxY         = 62 + kSmAdv             = 87px
    // desc bottom offset           = 87 + kSmDesc            = 93px  → need boxH ≥ 93+kPad = 96
    static constexpr int32_t kMinTitle = kMdH + 2 * kPad;  // 42

    for (const auto& ev : events) {
        if (ev.dateTime.year  != _currentTime.year  ||
            ev.dateTime.month != _currentTime.month ||
            ev.dateTime.day   != _currentTime.day) continue;

        const float evStartH = ev.dateTime.hour + ev.dateTime.minute / 60.0f;
        const float evEndH   = evStartH + ev.durationSeconds / 3600.0f;
        if (evEndH  < startTime) continue;
        if (evStartH > endTime)  continue;

        const float relStart = evStartH - startTime;
        int32_t boxY = UPCOMING_Y + static_cast<int32_t>(relStart * pph);
        int32_t boxH = static_cast<int32_t>(ev.durationSeconds / 3600.0f * pph);
        if (boxH < MIN_EVENT_H) boxH = MIN_EVENT_H;

        const int32_t areaBottom = UPCOMING_Y + UPCOMING_H;
        if (boxY < UPCOMING_Y) { boxH -= (UPCOMING_Y - boxY); boxY = UPCOMING_Y; }
        if (boxY + boxH > areaBottom) boxH = areaBottom - boxY;
        if (boxH <= 0) continue;

        const int32_t boxW     = EVENT_COL_W;
        const int32_t textMaxW = boxW - 2 * kPad - 2;
        epd_draw_rect(EVENT_COL_X, boxY, boxW, boxH, 0x00, _fb);

        // Line 1: title (FontMedium) — needs boxH ≥ 42px.
        if (boxH < kMinTitle || textMaxW <= 0) continue;
        int32_t cy1 = boxY + kMdAsc + kPad;
        int32_t cx1 = EVENT_COL_X + kPad + 1;
        writeln((GFXfont*)&FontMedium,
                fitText(&FontMedium, ev.title, textMaxW).c_str(),
                &cx1, &cy1, _fb);

        // Line 2: start time + duration (FontSmall) — needs boxH ≥ 71px.
        const int32_t titleBottom = boxY + kPad + kMdAsc + kMdDesc;  // boxY+39
        const int32_t cy2 = titleBottom + kPad + kSmAsc;              // boxY+62
        if (boxH < cy2 - boxY + kSmDesc + kPad) continue;

        const uint32_t totalMin = ev.durationSeconds / 60;
        char timeLine[32];
        if (totalMin < 60)
            snprintf(timeLine, sizeof(timeLine), "%02d:%02d  %u min",
                     ev.dateTime.hour, ev.dateTime.minute, totalMin);
        else {
            const uint32_t h = totalMin / 60, m = totalMin % 60;
            if (m == 0)
                snprintf(timeLine, sizeof(timeLine), "%02d:%02d  %uh",
                         ev.dateTime.hour, ev.dateTime.minute, h);
            else
                snprintf(timeLine, sizeof(timeLine), "%02d:%02d  %uh %um",
                         ev.dateTime.hour, ev.dateTime.minute, h, m);
        }
        int32_t cx2 = EVENT_COL_X + kPad + 1;
        int32_t mcy2 = cy2;
        writeln((GFXfont*)&FontSmall,
                fitText(&FontSmall, String(timeLine), textMaxW).c_str(),
                &cx2, &mcy2, _fb);

        // Line 3: description (FontSmall) — needs boxH ≥ 96px AND ≥2 words.
        const int32_t cy3 = cy2 + kSmAdv;
        if (boxH < cy3 - boxY + kSmDesc + kPad) continue;
        if (wordCount(ev.description) < 2) continue;
        int32_t cx3 = EVENT_COL_X + kPad + 1;
        int32_t mcy3 = cy3;
        writeln((GFXfont*)&FontSmall,
                fitText(&FontSmall, ev.description, textMaxW).c_str(),
                &cx3, &mcy3, _fb);
    }
}

// ─── showStatus / showError ───────────────────────────────────────────────────

void Lyligo_4_7_e_paper::showStatus(const String& message) {
    if (!_ready || !_fb) return;
    int32_t cx = 10, cy = SCREEN_H - kSmDesc - kPad;
    writeln((GFXfont*)&FontSmall, message.c_str(), &cx, &cy, _fb);
}

void Lyligo_4_7_e_paper::showError(const String& message) {
    showStatus(message);
}

// ─── Private helpers ──────────────────────────────────────────────────────────

void Lyligo_4_7_e_paper::ensurePowerOn() {
    if (!_powerOn) { epd_poweron(); _powerOn = true; }
}

void Lyligo_4_7_e_paper::drawBorder(int32_t x, int32_t y, int32_t w, int32_t h) {
    if (!_fb) return;
    epd_draw_rect(x, y, w, h, 0x00, _fb);
}

void Lyligo_4_7_e_paper::clearFbRegion(int32_t x, int32_t y, int32_t w, int32_t h) {
    if (!_fb) return;
    const int32_t stride = EPD_WIDTH / 2;
    const int32_t col    = x / 2;
    const int32_t bytes  = w / 2;
    for (int32_t row = y; row < y + h && row < EPD_HEIGHT; ++row)
        memset(_fb + row * stride + col, 0xFF, bytes);
}
