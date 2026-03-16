#pragma once
#include "interfaces/ITimeProvider.h"

// No-op provider — always reports not synced.
// Replace with e.g. NtpTimeProvider or RtcTimeProvider.
class NullTimeProvider : public ITimeProvider {
public:
    bool sync()                     override { return false; }
    bool getTime(TimeData&) const   override { return false; }
    bool isSynced() const           override { return false; }
    void setTimezone(const String&) override {}
};
