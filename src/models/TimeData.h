#pragma once
#include <Arduino.h>

enum class DayOfWeek : uint8_t {
    Monday = 1, Tuesday, Wednesday, Thursday, Friday, Saturday, Sunday
};

struct TimeData {
    uint16_t  year    = 2000;
    uint8_t   month   = 1;
    uint8_t   day     = 1;
    uint8_t   hour    = 0;
    uint8_t   minute  = 0;
    uint8_t   second  = 0;
    DayOfWeek weekday = DayOfWeek::Monday;
    String    timezone;
    bool      isSynced = false;
};
