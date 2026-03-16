#pragma once
#include <Arduino.h>

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
    uint32_t      durationSeconds  = 0;
    EventPriority priority         = EventPriority::Normal;
    EventType     type             = EventType::Reminder;
    bool          isAcknowledged   = false;
};
