#include "providers/Lyligo_4_7_e_paper_TimeProvider.h"
#include "config.h"
#include <Arduino.h>

Lyligo_4_7_e_paper_TimeProvider::Lyligo_4_7_e_paper_TimeProvider(uint8_t sda, uint8_t scl)
    : _sda(sda), _scl(scl) {}

bool Lyligo_4_7_e_paper_TimeProvider::begin() {

    Wire.begin(_sda, _scl);
    _rtc.begin();

    // If the RTC time predates the firmware build, the chip has never been
    // set (fresh device / dead battery / corrupted time).  Seed from the
    // build timestamp so the clock is always at least as recent as the flash.
    if (!_rtc.isVaild() || isRtcBeforeBuildTime()) {
        LOG("[INFO] RTC time invalid or stale — seeding from build timestamp");
        setBuildTime();
    }

    // Apply stored timezone to the ESP32 POSIX environment (if set before begin).
    if (_timezone.length() > 0) {
        setenv("TZ", _timezone.c_str(), 1);
        tzset();
    }

    // Pull the current time into the ESP32 system clock so millis-based
    // code and time() / localtime() are consistent with the RTC.
    _rtc.syncToSystem();

    return sync();
}

// ─── ITimeProvider ───────────────────────────────────────────────────────────

bool Lyligo_4_7_e_paper_TimeProvider::sync() {
    RTC_Date d = _rtc.getDateTime();

    // Basic sanity check — PCF8563 rolls over at year 99; we expect ≥2020.
    if (d.year < 2020) {
        _synced = false;
        return false;
    }

    _cached.year   = d.year;
    _cached.month  = d.month;
    _cached.day    = d.day;
    _cached.hour   = d.hour;
    _cached.minute = d.minute;
    _cached.second = d.second;
    _cached.weekday  = pcfDowToEnum(_rtc.getDayOfWeek(d.day, d.month, d.year));
    _cached.timezone = _timezone;
    _cached.isSynced = true;

    _synced = true;
    return true;
}

bool Lyligo_4_7_e_paper_TimeProvider::getTime(TimeData& out) const {
    if (!_synced) return false;
    out = _cached;
    return true;
}

bool Lyligo_4_7_e_paper_TimeProvider::isSynced() const {
    return _synced;
}

void Lyligo_4_7_e_paper_TimeProvider::setTimezone(const String& tz) {
    _timezone = tz;
    _cached.timezone = tz;
    setenv("TZ", tz.c_str(), 1);
    tzset();
}

void Lyligo_4_7_e_paper_TimeProvider::setDateTime(const TimeData& t) {
    _rtc.setDateTime(t.year, t.month, t.day, t.hour, t.minute, t.second);
    // Re-sync the cached value from the chip to confirm the write.
    sync();
}

// ─── private ─────────────────────────────────────────────────────────────────

bool Lyligo_4_7_e_paper_TimeProvider::isRtcBeforeBuildTime() {
    LOG("[INFO] isRtcBeforeBuildTime: RTC time predates build time, likely never set or battery dead");
    static const char* kMonths = "JanFebMarAprMayJunJulAugSepOctNovDec";
    char mon[4] = {};
    int  bDay = 1, bYear = 2000;
    sscanf(__DATE__, "%3s %d %d", mon, &bDay, &bYear);

    uint8_t bMonth = 1;
    for (int i = 0; i < 12; ++i) {
        if (strncmp(mon, kMonths + i * 3, 3) == 0) { bMonth = i + 1; break; }
    }

    RTC_Date d = _rtc.getDateTime();
    if (d.year  != bYear)  return d.year  < bYear;
    if (d.month != bMonth) return d.month < bMonth;
    return d.day < bDay;
}

void Lyligo_4_7_e_paper_TimeProvider::setBuildTime() {
    // __DATE__ is "Mmm DD YYYY", __TIME__ is "HH:MM:SS" — both are compile-time
    // string literals baked into the firmware by the preprocessor.
    static const char* kMonths = "JanFebMarAprMayJunJulAugSepOctNovDec";
    char mon[4] = {};
    int  day = 1, year = 2000, hour = 0, minute = 0, second = 0;
    sscanf(__DATE__, "%3s %d %d", mon, &day, &year);
    sscanf(__TIME__, "%d:%d:%d", &hour, &minute, &second);

    uint8_t month = 1;
    for (int i = 0; i < 12; ++i) {
        if (strncmp(mon, kMonths + i * 3, 3) == 0) { month = i + 1; break; }
    }

    _rtc.setDateTime(year, month, (uint8_t)day,
                     (uint8_t)hour, (uint8_t)minute, (uint8_t)second);
}

// PCF8563 getDayOfWeek: 0=Sunday, 1=Monday, …, 6=Saturday
// DayOfWeek enum:       Monday=1, …, Saturday=6, Sunday=7
DayOfWeek Lyligo_4_7_e_paper_TimeProvider::pcfDowToEnum(uint32_t pcfDow) {
    switch (pcfDow) {
        case 0:  return DayOfWeek::Sunday;
        case 1:  return DayOfWeek::Monday;
        case 2:  return DayOfWeek::Tuesday;
        case 3:  return DayOfWeek::Wednesday;
        case 4:  return DayOfWeek::Thursday;
        case 5:  return DayOfWeek::Friday;
        case 6:  return DayOfWeek::Saturday;
        default: return DayOfWeek::Monday;
    }
}
