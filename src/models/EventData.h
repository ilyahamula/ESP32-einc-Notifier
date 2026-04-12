#pragma once
#include <Arduino.h>
#include "TimeData.h"

enum class EventPriority : uint8_t {
    Low,
    Normal,
    High,
    Urgent
};

enum class EventType : uint8_t {
    Reminder,
    CalendarEvent,
    Notification,
    Alarm
};

struct EventData {
    String        id;
    String        title;
    String        description;
    uint32_t      triggerTimestamp = 0;   // Unix timestamp (UTC)
    TimeData      dateTime;              // Local time representation of triggerTimestamp
    uint32_t      durationSeconds  = 0;
    EventPriority priority         = EventPriority::Normal;
    EventType     type             = EventType::Reminder;
    bool          isAcknowledged   = false;
};
