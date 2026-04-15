#include "display/Lyligo_4_7_e_paper.h"
#include "fonts/FontSmall.h"    // SF Pro Text  Regular 10pt — advance_y=25  asc=20  desc=6
#include "fonts/FontMedium.h"   // SF Pro Text  Regular 14pt — advance_y=35  asc=28  desc=8
#include "fonts/FontClock.h"    // SF Pro Display Bold  32pt — advance_y=80  asc=64  desc=17
#include "fonts/FontTimeBig.h"  // SF Pro Display Bold  51pt — advance_y=127 asc=102 desc=26
#include "fonts/FontDate.h"     // SF Pro Display Reg   14pt — advance_y=35  asc=28  desc=8
#include "fonts/FontLarge.h"    // SF Pro Display Reg   18pt — advance_y=45  asc=36  desc=10
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

// ─── Weather icon helpers ─────────────────────────────────────────────────────

static inline void wpx(int32_t x, int32_t y, uint8_t* fb) {
    if (x >= 0 && x < EPD_WIDTH && y >= 0 && y < EPD_HEIGHT)
        epd_draw_pixel(x, y, 0x00, fb);
}

static void wCircle(int32_t cx, int32_t cy, int32_t r, bool fill, uint8_t* fb) {
    for (int32_t dy = -r; dy <= r; ++dy)
        for (int32_t dx = -r; dx <= r; ++dx) {
            int32_t d2 = dx*dx + dy*dy;
            if (fill ? d2 <= r*r : (d2 >= (r-1)*(r-1) && d2 <= r*r))
                wpx(cx+dx, cy+dy, fb);
        }
}

static void wLine(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint8_t* fb) {
    int32_t dx = abs(x1-x0), dy = -abs(y1-y0);
    int32_t sx = x0<x1 ? 1 : -1, sy = y0<y1 ? 1 : -1, err = dx+dy;
    for (;;) {
        wpx(x0, y0, fb);
        if (x0==x1 && y0==y1) break;
        int32_t e2 = 2*err;
        if (e2 >= dy) { err+=dy; x0+=sx; }
        if (e2 <= dx) { err+=dx; y0+=sy; }
    }
}

// Maps any condition to one of the 7 icon groups for drawing.
static WeatherCondition toIconGroup(WeatherCondition c) {
    switch (c) {
        case WeatherCondition::Sunny:
        case WeatherCondition::Clear:
            return WeatherCondition::Clear;

        case WeatherCondition::PartlyCloudy:
        case WeatherCondition::PatchyRainPossible:
        case WeatherCondition::PatchySnowPossible:
        case WeatherCondition::PatchySleetPossible:
        case WeatherCondition::PatchyFreezingDrizzle:
            return WeatherCondition::PartlyCloudy;

        case WeatherCondition::Cloudy:
        case WeatherCondition::Overcast:
            return WeatherCondition::Cloudy;

        case WeatherCondition::Mist:
        case WeatherCondition::Fog:
        case WeatherCondition::FreezingFog:
            return WeatherCondition::Fog;

        case WeatherCondition::ThunderyOutbreaks:
        case WeatherCondition::LightRainThunder:
        case WeatherCondition::HeavyRainThunder:
        case WeatherCondition::LightSnowThunder:
        case WeatherCondition::HeavySnowThunder:
            return WeatherCondition::ThunderyOutbreaks;

        case WeatherCondition::BlowingSnow:
        case WeatherCondition::Blizzard:
        case WeatherCondition::LightSnow:
        case WeatherCondition::ModerateSnow:
        case WeatherCondition::HeavySnow:
        case WeatherCondition::IcePellets:
        case WeatherCondition::LightSnowShowers:
        case WeatherCondition::HeavySnowShowers:
        case WeatherCondition::LightIcePellets:
        case WeatherCondition::HeavyIcePellets:
            return WeatherCondition::ModerateSnow;

        case WeatherCondition::LightDrizzle:
        case WeatherCondition::FreezingDrizzle:
        case WeatherCondition::HeavyFreezingDrizzle:
        case WeatherCondition::LightRain:
        case WeatherCondition::ModerateRain:
        case WeatherCondition::HeavyRain:
        case WeatherCondition::LightFreezingRain:
        case WeatherCondition::HeavyFreezingRain:
        case WeatherCondition::LightSleet:
        case WeatherCondition::HeavySleet:
        case WeatherCondition::LightRainShower:
        case WeatherCondition::HeavyRainShower:
        case WeatherCondition::LightSleetShowers:
        case WeatherCondition::HeavySleetShowers:
            return WeatherCondition::ModerateRain;

        default:
            return WeatherCondition::Unknown;
    }
}

static void conditionLines(WeatherCondition c, const char*& l1, const char*& l2) {
    l2 = nullptr;
    switch (c) {
        case WeatherCondition::Sunny:                 l1 = "Sunny";              break;
        case WeatherCondition::Clear:                 l1 = "Clear";              break;
        case WeatherCondition::PartlyCloudy:          l1 = "Partly";  l2 = "Cloudy";   break;
        case WeatherCondition::Cloudy:                l1 = "Cloudy";             break;
        case WeatherCondition::Overcast:              l1 = "Overcast";           break;
        case WeatherCondition::Mist:                  l1 = "Mist";               break;
        case WeatherCondition::Fog:                   l1 = "Fog";                break;
        case WeatherCondition::FreezingFog:           l1 = "Freezing"; l2 = "Fog";     break;
        case WeatherCondition::PatchyRainPossible:    l1 = "Patchy";  l2 = "Rain";     break;
        case WeatherCondition::PatchySnowPossible:    l1 = "Patchy";  l2 = "Snow";     break;
        case WeatherCondition::PatchySleetPossible:   l1 = "Patchy";  l2 = "Sleet";    break;
        case WeatherCondition::PatchyFreezingDrizzle: l1 = "Patchy";  l2 = "Drizzle";  break;
        case WeatherCondition::ThunderyOutbreaks:     l1 = "Thunder";            break;
        case WeatherCondition::BlowingSnow:           l1 = "Blowing"; l2 = "Snow";     break;
        case WeatherCondition::Blizzard:              l1 = "Blizzard";           break;
        case WeatherCondition::LightDrizzle:          l1 = "Drizzle";            break;
        case WeatherCondition::FreezingDrizzle:       l1 = "Frz.";    l2 = "Drizzle";  break;
        case WeatherCondition::HeavyFreezingDrizzle:  l1 = "Hvy Frz"; l2 = "Drizzle";  break;
        case WeatherCondition::LightRain:             l1 = "Light";   l2 = "Rain";     break;
        case WeatherCondition::ModerateRain:          l1 = "Rain";               break;
        case WeatherCondition::HeavyRain:             l1 = "Heavy";   l2 = "Rain";     break;
        case WeatherCondition::LightFreezingRain:     l1 = "Frz.";    l2 = "Rain";     break;
        case WeatherCondition::HeavyFreezingRain:     l1 = "Hvy Frz"; l2 = "Rain";     break;
        case WeatherCondition::LightSleet:            l1 = "Light";   l2 = "Sleet";    break;
        case WeatherCondition::HeavySleet:            l1 = "Heavy";   l2 = "Sleet";    break;
        case WeatherCondition::LightSnow:             l1 = "Light";   l2 = "Snow";     break;
        case WeatherCondition::ModerateSnow:          l1 = "Snow";               break;
        case WeatherCondition::HeavySnow:             l1 = "Heavy";   l2 = "Snow";     break;
        case WeatherCondition::IcePellets:            l1 = "Ice";     l2 = "Pellets";  break;
        case WeatherCondition::LightRainShower:       l1 = "Light";   l2 = "Shower";   break;
        case WeatherCondition::HeavyRainShower:       l1 = "Heavy";   l2 = "Shower";   break;
        case WeatherCondition::LightSleetShowers:     l1 = "Sleet";   l2 = "Shower";   break;
        case WeatherCondition::HeavySleetShowers:     l1 = "Sleet";   l2 = "Shower";   break;
        case WeatherCondition::LightSnowShowers:      l1 = "Snow";    l2 = "Shower";   break;
        case WeatherCondition::HeavySnowShowers:      l1 = "Snow";    l2 = "Shower";   break;
        case WeatherCondition::LightIcePellets:       l1 = "Ice";     l2 = "Shower";   break;
        case WeatherCondition::HeavyIcePellets:       l1 = "Ice";     l2 = "Shower";   break;
        case WeatherCondition::LightRainThunder:      l1 = "Thunder"; l2 = "Rain";     break;
        case WeatherCondition::HeavyRainThunder:      l1 = "Thunder"; l2 = "Rain";     break;
        case WeatherCondition::LightSnowThunder:      l1 = "Thunder"; l2 = "Snow";     break;
        case WeatherCondition::HeavySnowThunder:      l1 = "Thunder"; l2 = "Snow";     break;
        default:                                       l1 = "Unknown";            break;
    }
}

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
// FontTimeBig (51pt SF Pro Display Bold)
static constexpr int32_t kTbAsc  = 102, kTbDesc = 26;
// FontDate   (14pt SF Pro Display Regular) — identical metrics to FontMedium
static constexpr int32_t kDtAsc  = 28, kDtDesc =  8, kDtAdv = 35;
// FontLarge  (18pt SF Pro Display Regular)
static constexpr int32_t kLgAsc  = 36, kLgDesc = 10, kLgAdv = 45;

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
//
//  Time (FontTimeBig 51pt Bold): fills the full area height.
//    cy  = TIME_Y + kPad + kTbAsc = 105
//    top = 3, bottom = 131 (< 135 ✓)
//
//  Date (FontDate 14pt): two lines right of the time text, vertically centred.
//    two-line block height = kDtAsc+kDtDesc + kPad + kDtAsc+kDtDesc = 75px
//    block top offset = (135 - 75) / 2 = 30px
//    line 1 (day name) cy = TIME_Y + 30 + kDtAsc = 58
//    line 2 (date)     cy = 58 + kDtAdv           = 93
//
void Lyligo_4_7_e_paper::showTime(const TimeData& time) {
    if (!_ready || !_fb) return;
    _currentTime = time;

    clearFbRegion(TIME_X, TIME_Y, TIME_W, TIME_H);
    drawBorder(TIME_X, TIME_Y, TIME_W, TIME_H);

    // ── Time ──────────────────────────────────────────────────────────────────
    char timeBuf[8];
    snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d", time.hour, time.minute);

    int32_t cx1 = TIME_X + 8, cy1 = TIME_Y + kPad + kTbAsc;
    writeln((GFXfont*)&FontTimeBig, timeBuf, &cx1, &cy1, _fb);

    // ── Date (right of time, vertically centred) ──────────────────────────────
    const int32_t timeW    = textWidth((GFXfont*)&FontTimeBig, timeBuf);
    const int32_t dateX    = TIME_X + 8 + timeW + 16;
    const int32_t dateMaxW = TIME_W - dateX - 8;
    if (dateMaxW <= 0) return;

    const uint8_t dowIdx  = static_cast<uint8_t>(time.weekday);
    const char*   dayName = (dowIdx >= 1 && dowIdx <= 7) ? kDayNames[dowIdx] : "";
    const char*   monName = (time.month >= 1 && time.month <= 12) ? kMonthNames[time.month] : "???";
    char dateBuf[32];
    snprintf(dateBuf, sizeof(dateBuf), "%02d %s %04d", time.day, monName, time.year);

    static constexpr int32_t kBlockH = kDtAsc + kDtDesc + kPad + kDtAsc + kDtDesc;  // 75
    const int32_t blockTop = TIME_Y + (TIME_H - kBlockH) / 2;

    int32_t cx2 = dateX, cy2 = blockTop + kDtAsc;
    writeln((GFXfont*)&FontDate,
            fitText(&FontDate, String(dayName), dateMaxW).c_str(),
            &cx2, &cy2, _fb);

    int32_t cx3 = dateX, cy3 = cy2 + kDtAdv;
    writeln((GFXfont*)&FontDate,
            fitText(&FontDate, String(dateBuf), dateMaxW).c_str(),
            &cx3, &cy3, _fb);
}

// ─── drawWeatherIcon ──────────────────────────────────────────────────────────
//
//  Draws a sz×sz weather icon with top-left at (ox, oy).
//  All geometry is proportional to sz (reference: sz=64).
//
void Lyligo_4_7_e_paper::drawWeatherIcon(int32_t ox, int32_t oy, int32_t sz, WeatherCondition cond) {
    // Helper: scale a reference-64 coordinate to actual sz.
    auto s = [&](int32_t v) -> int32_t { return (v * sz + 32) / 64; };

    const int32_t cx = ox + sz/2, cy = oy + sz/2;

    switch (toIconGroup(cond)) {
        case WeatherCondition::Clear: {
            const int32_t r  = s(12), gap = s(4), ray = s(10);
            wCircle(cx, cy, r, true, _fb);
            wLine(cx,        cy-r-gap,  cx,        cy-r-gap-ray, _fb);  // N
            wLine(cx,        cy+r+gap,  cx,        cy+r+gap+ray, _fb);  // S
            wLine(cx-r-gap,  cy,        cx-r-gap-ray, cy,         _fb);  // W
            wLine(cx+r+gap,  cy,        cx+r+gap+ray, cy,         _fb);  // E
            const int32_t d1 = s(10), d2 = s(17);
            wLine(cx+d1, cy-d1, cx+d2, cy-d2, _fb);  // NE
            wLine(cx+d1, cy+d1, cx+d2, cy+d2, _fb);  // SE
            wLine(cx-d1, cy+d1, cx-d2, cy+d2, _fb);  // SW
            wLine(cx-d1, cy-d1, cx-d2, cy-d2, _fb);  // NW
            break;
        }

        case WeatherCondition::PartlyCloudy: {
            // Mini sun top-left quarter
            const int32_t sx = ox + s(14), sy = oy + s(14);
            const int32_t sr = s(6), sg = s(2), sray = s(4);
            wCircle(sx, sy, sr, true, _fb);
            wLine(sx, sy-sr-sg, sx, sy-sr-sg-sray, _fb);
            wLine(sx, sy+sr+sg, sx, sy+sr+sg+sray, _fb);
            wLine(sx-sr-sg, sy, sx-sr-sg-sray, sy, _fb);
            wLine(sx+sr+sg, sy, sx+sr+sg+sray, sy, _fb);
            // Cloud bottom-right
            const int32_t cb1x = ox+s(26), cb1y = oy+s(40), cb1r = s(10);
            const int32_t cb2x = ox+s(42), cb2y = oy+s(38), cb2r = s(8);
            const int32_t cb3x = ox+s(52), cb3y = oy+s(42), cb3r = s(6);
            wCircle(cb1x, cb1y, cb1r, true, _fb);
            wCircle(cb2x, cb2y, cb2r, true, _fb);
            wCircle(cb3x, cb3y, cb3r, true, _fb);
            for (int32_t px = ox+s(16); px <= ox+s(58); ++px)
                for (int32_t py = oy+s(42); py <= oy+s(60); ++py)
                    wpx(px, py, _fb);
            break;
        }

        case WeatherCondition::Cloudy: {
            const int32_t c1x = ox+s(20), c1y = oy+s(30), c1r = s(12);
            const int32_t c2x = ox+s(38), c2y = oy+s(26), c2r = s(14);
            const int32_t c3x = ox+s(52), c3y = oy+s(32), c3r = s(10);
            wCircle(c1x, c1y, c1r, true, _fb);
            wCircle(c2x, c2y, c2r, true, _fb);
            wCircle(c3x, c3y, c3r, true, _fb);
            for (int32_t px = ox+s(8); px <= ox+s(62); ++px)
                for (int32_t py = oy+s(32); py <= oy+s(54); ++py)
                    wpx(px, py, _fb);
            break;
        }

        case WeatherCondition::ModerateRain: {
            const int32_t c1x = ox+s(20), c1y = oy+s(22), c1r = s(10);
            const int32_t c2x = ox+s(38), c2y = oy+s(18), c2r = s(12);
            const int32_t c3x = ox+s(52), c3y = oy+s(24), c3r = s(8);
            wCircle(c1x, c1y, c1r, true, _fb);
            wCircle(c2x, c2y, c2r, true, _fb);
            wCircle(c3x, c3y, c3r, true, _fb);
            for (int32_t px = ox+s(10); px <= ox+s(60); ++px)
                for (int32_t py = oy+s(24); py <= oy+s(38); ++py)
                    wpx(px, py, _fb);
            // 4 diagonal drops
            for (int32_t d = 0; d < 4; ++d) {
                const int32_t rx = ox + s(10 + d*14);
                wLine(rx+s(4), oy+s(42), rx,      oy+s(54), _fb);
                wLine(rx+s(4), oy+s(42), rx+s(6), oy+s(42), _fb);
            }
            break;
        }

        case WeatherCondition::ThunderyOutbreaks: {
            const int32_t c1x = ox+s(20), c1y = oy+s(20), c1r = s(10);
            const int32_t c2x = ox+s(38), c2y = oy+s(16), c2r = s(12);
            const int32_t c3x = ox+s(52), c3y = oy+s(22), c3r = s(8);
            wCircle(c1x, c1y, c1r, true, _fb);
            wCircle(c2x, c2y, c2r, true, _fb);
            wCircle(c3x, c3y, c3r, true, _fb);
            for (int32_t px = ox+s(10); px <= ox+s(60); ++px)
                for (int32_t py = oy+s(22); py <= oy+s(34); ++py)
                    wpx(px, py, _fb);
            // Lightning bolt
            wLine(cx+s(4),  oy+s(36), cx-s(4),  oy+s(48), _fb);
            wLine(cx-s(4),  oy+s(48), cx+s(4),  oy+s(48), _fb);
            wLine(cx+s(4),  oy+s(48), cx-s(6),  oy+s(62), _fb);
            break;
        }

        case WeatherCondition::ModerateSnow: {
            const int32_t arm = s(28), tick = s(8);
            wLine(cx, cy-arm, cx, cy+arm, _fb);
            wLine(cx-arm, cy, cx+arm, cy, _fb);
            const int32_t da = s(20);
            wLine(cx-da, cy-da, cx+da, cy+da, _fb);
            wLine(cx+da, cy-da, cx-da, cy+da, _fb);
            // ticks on cardinal arms
            wLine(cx-tick, cy-arm+s(6), cx+tick, cy-arm+s(6), _fb);
            wLine(cx-tick, cy+arm-s(6), cx+tick, cy+arm-s(6), _fb);
            wLine(cx-arm+s(6), cy-tick, cx-arm+s(6), cy+tick, _fb);
            wLine(cx+arm-s(6), cy-tick, cx+arm-s(6), cy+tick, _fb);
            break;
        }

        case WeatherCondition::Fog: {
            const int32_t lw = s(52), lx = ox + s(6);
            const int32_t sw = s(40), sx2 = ox + s(12);
            for (int32_t t = 0; t < 3; ++t) {
                const int32_t y = oy + s(14 + t * 18);
                for (int32_t px = lx;  px < lx+lw;  ++px) wpx(px, y,   _fb);
                for (int32_t px = lx;  px < lx+lw;  ++px) wpx(px, y+1, _fb);
                if (t < 2) {
                    const int32_t ys = oy + s(23 + t * 18);
                    for (int32_t px = sx2; px < sx2+sw; ++px) wpx(px, ys,   _fb);
                    for (int32_t px = sx2; px < sx2+sw; ++px) wpx(px, ys+1, _fb);
                }
            }
            break;
        }

        default:
            wLine(cx-s(10), cy, cx+s(10), cy, _fb);
            break;
    }
}

// ─── showWeather ──────────────────────────────────────────────────────────────
//
//  Panel: x=480, y=0, w=480, h=135
//
//  Left column — temperature + feels like:
//    temp  (FontClock) cy = kPad + kClAsc = 67,  bottom = 84
//    feels (FontSmall) cy = 84 + kPad + kSmAsc  = 107
//
//  Middle column — icon (sz=kClAsc=64) + condition name under it:
//    iconY = kPad = 3,  iconBottom = 3+64 = 67
//    condCy = 67 + kPad + kSmAsc = 90
//
//  Right column — location (FontLarge, right-aligned), humidity, wind (FontSmall):
//    locCy  = kPad + kLgAsc         = 39
//    humCy  = 39 + kLgAdv           = 84
//    windCy = 84 + kSmAdv           = 109  → bottom 115 ✓
//
void Lyligo_4_7_e_paper::showWeather(const WeatherData& weather) {
    if (!_ready || !_fb) return;
    clearFbRegion(WEATHER_X, WEATHER_Y, WEATHER_W, WEATHER_H);
    drawBorder(WEATHER_X, WEATHER_Y, WEATHER_W, WEATHER_H);

    static constexpr int32_t kIconSz = kClAsc;  // 64px icon matches temp height

    // ── Left: temperature ────────────────────────────────────────────────────
    char tempBuf[8];
    snprintf(tempBuf, sizeof(tempBuf), "%dC", static_cast<int>(weather.temperatureCelsius));
    int32_t tcx = WEATHER_X + 8, tcy = WEATHER_Y + kPad + kClAsc;
    writeln((GFXfont*)&FontClock, tempBuf, &tcx, &tcy, _fb);

    char feelsBuf[16];
    snprintf(feelsBuf, sizeof(feelsBuf), "feels %dC", static_cast<int>(weather.feelsLikeCelsius));
    int32_t fcx = WEATHER_X + 8, fcy = WEATHER_Y + kPad + kClAsc + kClDesc + kPad + kSmAsc;
    writeln((GFXfont*)&FontSmall, feelsBuf, &fcx, &fcy, _fb);

    // ── Middle: icon + condition name ────────────────────────────────────────
    const int32_t tempTextW = textWidth((GFXfont*)&FontClock, tempBuf);
    const int32_t iconX     = WEATHER_X + 8 + tempTextW + 16;
    const int32_t iconY     = WEATHER_Y + kPad;
    drawWeatherIcon(iconX, iconY, kIconSz, weather.condition);

    const int32_t condCy = iconY + kIconSz + kPad + kSmAsc;
    const char *cl1, *cl2;
    conditionLines(weather.condition, cl1, cl2);
    // Centre each line under the icon (no width cap — all labels fit within kIconSz).
    auto drawCentred = [&](const char* txt, int32_t cy) {
        const int32_t w = textWidth((GFXfont*)&FontSmall, txt);
        int32_t cx = iconX + (kIconSz - w) / 2, cpy = cy;
        writeln((GFXfont*)&FontSmall, txt, &cx, &cpy, _fb);
    };
    drawCentred(cl1, condCy);
    if (cl2) drawCentred(cl2, condCy + kSmAdv);

    // ── Right: location, humidity, wind ──────────────────────────────────────
    const int32_t rightEdge = WEATHER_X + WEATHER_W - 8;
    const int32_t rightColX = iconX + kIconSz + 12;
    const int32_t rightColW = rightEdge - rightColX;
    if (rightColW <= 0) return;

    // Location (FontLarge, right-aligned)
    String loc = fitText((GFXfont*)&FontLarge, weather.location, rightColW);
    const int32_t locW = textWidth((GFXfont*)&FontLarge, loc.c_str());
    int32_t lcx = rightEdge - locW, lcy = WEATHER_Y + kPad + kLgAsc;
    writeln((GFXfont*)&FontLarge, loc.c_str(), &lcx, &lcy, _fb);

    // Humidity (FontSmall, right-aligned)
    char humBuf[20];
    snprintf(humBuf, sizeof(humBuf), "Humidity %.0f%%", weather.humidityPercent);
    String hum = fitText((GFXfont*)&FontSmall, String(humBuf), rightColW);
    const int32_t humW = textWidth((GFXfont*)&FontSmall, hum.c_str());
    int32_t hcx = rightEdge - humW, hcy = WEATHER_Y + kPad + kLgAsc + kLgAdv;
    writeln((GFXfont*)&FontSmall, hum.c_str(), &hcx, &hcy, _fb);

    // Wind (FontSmall, right-aligned)
    const uint8_t wdIdx   = static_cast<uint8_t>(weather.windDirection);
    const char*   windDir = (wdIdx <= 8) ? kWindDir[wdIdx] : "?";
    char windBuf[24];
    snprintf(windBuf, sizeof(windBuf), "Wind %.0fkm/h %s", weather.windSpeedKmh, windDir);
    String wind = fitText((GFXfont*)&FontSmall, String(windBuf), rightColW);
    const int32_t windW = textWidth((GFXfont*)&FontSmall, wind.c_str());
    int32_t wcx = rightEdge - windW, wcy = hcy + kSmAdv;
    writeln((GFXfont*)&FontSmall, wind.c_str(), &wcx, &wcy, _fb);
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

    // Column width sized to the widest day label content ("Wed", "30") + margins.
    const int32_t dayLabelW = std::max(textWidth((GFXfont*)&FontSmall, "Wed"),
                                       textWidth((GFXfont*)&FontSmall, "30"));
    const int32_t daySepX  = WEEK_X + 4 + dayLabelW + 4;
    const int32_t weekEvX  = daySepX + 4;
    const int32_t weekEvW  = WEEK_X + WEEK_W - weekEvX - 4;

    for (int32_t y = WEEK_Y; y < WEEK_Y + WEEK_H; ++y)
        epd_draw_pixel(daySepX, y, 0x00, _fb);

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
        const int32_t slotW = weekEvW / n;
        const int32_t boxH  = DAY_ROW_H - 2;  // 55px

        for (int32_t i = 0; i < n; ++i) {
            const EventData* ev = dayEvents[i];
            const int32_t boxX = weekEvX + i * slotW;
            const int32_t boxY = rowY + 1;
            const int32_t boxW = (i == n - 1)
                                 ? (WEEK_X + WEEK_W - 4 - boxX)
                                  : slotW - 1;  // 1px white gap between adjacent events

            epd_fill_rect(boxX, boxY, boxW, boxH, 0x00, _fb);

            // Title + start time in white on the black fill.
            const int32_t textMaxW = boxW - 2 * kPad - 2;
            if (textMaxW > 0 && boxH >= kSmH + 2 * kPad) {
                FontProperties wp = {};
                wp.fg_color = 0x0F;
                wp.bg_color = 0x00;

                String label = fitText(&FontSmall, ev->title, textMaxW);
                int32_t tcx = boxX + kPad + 1;
                int32_t tcy = boxY + kSmAsc + kPad;
                write_mode((GFXfont*)&FontSmall, label.c_str(), &tcx, &tcy, _fb,
                           WHITE_ON_BLACK, &wp);

                // Start time below title — fits when line 2 bottom (kPad+kSmAsc+kSmAdv+kSmDesc=54) < boxH.
                if (kPad + kSmAsc + kSmAdv + kSmDesc < boxH) {
                    char timeBuf[6];
                    snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d",
                             ev->dateTime.hour, ev->dateTime.minute);
                    int32_t ttcx = boxX + kPad + 1;
                    int32_t ttcy = tcy + kSmAdv;
                    write_mode((GFXfont*)&FontSmall, timeBuf, &ttcx, &ttcy, _fb,
                               WHITE_ON_BLACK, &wp);
                }
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

    // Column width sized to the widest hour label ("23:00") + margins.
    const int32_t hourSepX  = UPCOMING_X + 4 + textWidth((GFXfont*)&FontSmall, "23:00") + 4;
    const int32_t eventColX = hourSepX + 4;
    const int32_t eventColW = UPCOMING_W - eventColX - 4;

    for (int32_t y = UPCOMING_Y; y < UPCOMING_Y + UPCOMING_H; ++y)
        epd_draw_pixel(hourSepX, y, 0x00, _fb);

    // Tick + label for every whole hour within the view.
    const int32_t firstHour = static_cast<int32_t>(startTime) +
                              (startTime > static_cast<float>(static_cast<int32_t>(startTime)) ? 1 : 0);
    for (int32_t h = firstHour; static_cast<float>(h) <= endTime; ++h) {
        const int32_t ty = UPCOMING_Y + static_cast<int32_t>((h - startTime) * pph);

        for (int32_t tx = hourSepX; tx <= hourSepX + 5; ++tx)
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

    for (const auto& ev : events) {
        // Accept events from the current day OR the next day (for windows crossing midnight).
        bool isCurrentDay = (ev.dateTime.year  == _currentTime.year  &&
                             ev.dateTime.month == _currentTime.month &&
                             ev.dateTime.day   == _currentTime.day);
        bool isNextDay    = false;
        if (!isCurrentDay) {
            // Build a simple day index to check adjacency (handles month/year wrap via carry).
            // We only need to know if ev is exactly one calendar day after _currentTime.
            // Compute day-of-year offset isn't necessary — just check by incrementing.
            int ey = _currentTime.year, em = _currentTime.month, ed = _currentTime.day + 1;
            static const int dpm[] = {0,31,28,31,30,31,30,31,31,30,31,30,31};
            int maxDay = dpm[em];
            if (em == 2 && ((ey % 4 == 0 && ey % 100 != 0) || ey % 400 == 0)) maxDay = 29;
            if (ed > maxDay) { ed = 1; em++; }
            if (em > 12)     { em = 1; ey++; }
            isNextDay = (ev.dateTime.year  == ey &&
                         ev.dateTime.month == em &&
                         ev.dateTime.day   == ed);
        }
        if (!isCurrentDay && !isNextDay) continue;

        float evStartH = ev.dateTime.hour + ev.dateTime.minute / 60.0f;
        if (isNextDay) evStartH += 24.0f;  // shift next-day events into the same timeline
        const float evEndH = evStartH + ev.durationSeconds / 3600.0f;
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

        const int32_t boxW     = eventColW;
        const int32_t textX    = eventColX + kPad + 1;
        const int32_t textMaxW = boxW - 2 * kPad - 2;
        if (textMaxW <= 0) continue;

        // Build time string "HH:MM-HH:MM" — always shown right-aligned.
        const uint32_t endTotalMin = ev.dateTime.hour * 60 + ev.dateTime.minute
                                     + ev.durationSeconds / 60;
        const int32_t endH = static_cast<int32_t>(endTotalMin / 60) % 24;
        const int32_t endM = static_cast<int32_t>(endTotalMin % 60);
        char timeStr[12];
        snprintf(timeStr, sizeof(timeStr), "%02d:%02d-%02d:%02d",
                 ev.dateTime.hour, ev.dateTime.minute, endH, endM);
        const int32_t timeW = textWidth((GFXfont*)&FontSmall, timeStr);

        // Minimum gap between title and time (~3 spaces).
        static constexpr int32_t kTimeGap = 8;
        const int32_t titleMaxW = textMaxW - timeW - kTimeGap;
        const String fittedTitle = (titleMaxW > 0)
            ? fitText((GFXfont*)&FontSmall, ev.title, titleMaxW)
            : String();

        // Text baseline top-aligned inside box.
        const int32_t textCy = boxY + kPad + kSmAsc;

        // X position of time label (right-aligned inside box).
        const int32_t timeX = eventColX + boxW - 1 - kPad - timeW;

        // Fill box black; text rendered white on top.
        epd_fill_rect(eventColX, boxY, boxW, boxH, 0x00, _fb);

        FontProperties wp = {};
        wp.fg_color = 0x0F;
        wp.bg_color = 0x00;

        // Render title (left) and time range (right).
        if (!fittedTitle.isEmpty()) {
            int32_t cx = textX, cy = textCy;
            write_mode((GFXfont*)&FontSmall, fittedTitle.c_str(), &cx, &cy, _fb,
                       WHITE_ON_BLACK, &wp);
        }
        int32_t tcx = timeX, tcy = textCy;
        write_mode((GFXfont*)&FontSmall, timeStr, &tcx, &tcy, _fb,
                   WHITE_ON_BLACK, &wp);
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

void Lyligo_4_7_e_paper::showWaitConnection() {
    if (!_ready || !_fb) return;
    memset(_fb, 0xFF, FB_SIZE);
    static const char* msg = "Waiting for connection...";
    const int32_t msgW = textWidth((GFXfont*)&FontMedium, msg);
    int32_t cx = (SCREEN_W - msgW) / 2;
    int32_t cy = SCREEN_H / 2 + kMdAsc / 2;
    writeln((GFXfont*)&FontMedium, msg, &cx, &cy, _fb);
    epd_draw_grayscale_image(epd_full_screen(), _fb);
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
