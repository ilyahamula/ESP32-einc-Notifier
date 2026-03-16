#pragma once
#include "models/TimeData.h"

class ITimeProvider {
public:
    virtual ~ITimeProvider() = default;

    virtual bool sync()                       = 0;
    virtual bool getTime(TimeData& out) const = 0;
    virtual bool isSynced() const             = 0;
    virtual void setTimezone(const String& tz)= 0;
};
