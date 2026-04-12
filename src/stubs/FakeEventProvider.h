#pragma once
#include "interfaces/IEventProvider.h"
#include <string.h>
#include <stdio.h>

// Generates a realistic event set anchored to the firmware build timestamp:
//   - One "upcoming" event exactly 2 hours after build time (high priority).
//   - Seven events spread across Mon–Sun of the build week at pseudo-random
//     but deterministic times (seeded from the build date, so the layout is
//     stable across reboots but varies between builds).
class FakeEventProvider : public IEventProvider {
public:
    bool fetchEvents(std::vector<EventData>& out) override {
        out.clear();

        int     bYear, bDay, bHour, bMin, bSec;
        uint8_t bMonth;
        parseBuildDateTime(bYear, bMonth, bDay, bHour, bMin, bSec);

        // ── Upcoming: build time + 2 h ────────────────────────────────────
        int upHour = bHour + 2, upDay = bDay;
        int upMonth = bMonth, upYear = bYear;
        if (upHour >= 24) { upHour -= 24; upDay++; }
        adjustDate(upYear, upMonth, upDay);

        out.push_back(makeEvent("E0", "Team Sync", "upcoming · auto-generated",
                                upYear, upMonth, upDay, upHour, bMin, 0,
                                30 * 60, EventPriority::High));

        // ── Weekly events: one per day Mon–Sun, pseudo-random times ──────
        // Sakamoto: 0=Sun,1=Mon,…,6=Sat  →  offset to Monday of build week.
        int dow = sakamoto(bYear, bMonth, bDay);
        int mondayOffset = (dow == 0) ? -6 : -(dow - 1);

        static const char* kTitles[] = {
            "Sprint Planning", "Code Review",   "1:1 with manager",
            "Design Sync",     "Infra Call",    "Retrospective",    "Demo"
        };
        static const char* kDescs[] = {
            "Q2 roadmap", "PR review", "feedback",
            "UI review",  "deploy plan", "sprint wrap-up", "stakeholders"
        };

        uint32_t seed = (uint32_t)bYear * 10000UL + bMonth * 100 + bDay;

        for (int i = 0; i < 7; ++i) {
            int evDay = bDay + mondayOffset + i;
            int evMonth = bMonth, evYear = bYear;
            adjustDate(evYear, evMonth, evDay);

            seed = lcg(seed);  uint8_t hour = 8 + (seed % 10);       // 08:00–17:00
            seed = lcg(seed);  uint8_t min  = (seed & 1) ? 30 : 0;
            seed = lcg(seed);  uint32_t dur = (30 + (seed % 4) * 15) * 60; // 30/45/60/75 min

            String id = "E" + String(i + 1);
            out.push_back(makeEvent(id.c_str(), kTitles[i], kDescs[i],
                                    evYear, evMonth, evDay, hour, min, 0,
                                    dur, EventPriority::Normal));
        }

        return true;
    }

    bool isAvailable() const             override { return true; }
    void acknowledgeEvent(const String&) override {}

private:
    // ── Date helpers ──────────────────────────────────────────────────────

    static void parseBuildDateTime(int& year, uint8_t& month, int& day,
                                   int& hour, int& minute, int& second) {
        static const char* kMonths = "JanFebMarAprMayJunJulAugSepOctNovDec";
        char mon[4] = {};
        sscanf(__DATE__, "%3s %d %d", mon, &day, &year);
        sscanf(__TIME__, "%d:%d:%d",  &hour, &minute, &second);
        month = 1;
        for (int i = 0; i < 12; ++i)
            if (strncmp(mon, kMonths + i * 3, 3) == 0) { month = i + 1; break; }
    }

    // Sakamoto's algorithm — returns 0=Sun, 1=Mon … 6=Sat.
    static int sakamoto(int y, int m, int d) {
        static const int t[] = {0,3,2,5,0,3,5,1,4,6,2,4};
        if (m < 3) y--;
        return (y + y/4 - y/100 + y/400 + t[m-1] + d) % 7;
    }

    static uint8_t daysInMonth(int y, int m) {
        static const uint8_t k[] = {31,28,31,30,31,30,31,31,30,31,30,31};
        if (m == 2 && (y % 4 == 0 && (y % 100 != 0 || y % 400 == 0))) return 29;
        return k[m - 1];
    }

    static void adjustDate(int& y, int& m, int& d) {
        while (d < 1)               { if (--m < 1)  { m = 12; y--; } d += daysInMonth(y, m); }
        while (d > daysInMonth(y,m)){ d -= daysInMonth(y, m); if (++m > 12) { m = 1; y++; } }
    }

    // Convert Sakamoto (0=Sun…6=Sat) → DayOfWeek enum (Monday=1…Sunday=7).
    static DayOfWeek toDayOfWeek(int y, int m, int d) {
        int sd = sakamoto(y, m, d);
        return static_cast<DayOfWeek>(sd == 0 ? 7 : sd);
    }

    // Simple LCG for deterministic pseudo-random spread.
    static uint32_t lcg(uint32_t s) { return s * 1664525UL + 1013904223UL; }

    // ── Event factory ─────────────────────────────────────────────────────

    static EventData makeEvent(const char* id, const char* title, const char* desc,
                                int year, int month, int day,
                                int hour, int minute, int second,
                                uint32_t durationSeconds, EventPriority priority) {
        EventData ev;
        ev.id              = id;
        ev.title           = title;
        ev.description     = desc;
        ev.durationSeconds = durationSeconds;
        ev.priority        = priority;
        ev.type            = EventType::CalendarEvent;

        ev.dateTime.year     = (uint16_t)year;
        ev.dateTime.month    = (uint8_t)month;
        ev.dateTime.day      = (uint8_t)day;
        ev.dateTime.hour     = (uint8_t)hour;
        ev.dateTime.minute   = (uint8_t)minute;
        ev.dateTime.second   = (uint8_t)second;
        ev.dateTime.weekday  = toDayOfWeek(year, month, day);
        ev.dateTime.isSynced = true;
        return ev;
    }
};
