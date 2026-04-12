#pragma once
#include "interfaces/ITimeProvider.h"
#include <Wire.h>
#include <pcf8563.h>

// LilyGo T5-4.7 S3 default I2C pins (shared with touch controller)
static constexpr uint8_t kRtcSda = 18;
static constexpr uint8_t kRtcScl = 17;

class Lyligo_4_7_e_paper_TimeProvider : public ITimeProvider {
public:
    explicit Lyligo_4_7_e_paper_TimeProvider(uint8_t sda = kRtcSda, uint8_t scl = kRtcScl);

    // Must be called once in setup() before any other method.
    // Initialises I2C and the PCF8563.  Returns false if the chip is not found
    // or its voltage-low flag is set (battery dead — time may be invalid).
    bool begin();

    // ITimeProvider
    bool sync()                        override;
    bool getTime(TimeData& out) const  override;
    bool isSynced() const              override;
    void setTimezone(const String& tz) override;

    // Set the RTC hardware clock (call after an NTP sync, for example).
    void setDateTime(const TimeData& t);

    // Write the firmware build timestamp to the RTC — used as a fallback when
    // the backup battery was dead and the stored time is invalid.
    void setBuildTime();

    // Returns true if the RTC date is earlier than the firmware build date.
    bool isRtcBeforeBuildTime();

private:
    PCF8563_Class _rtc;
    uint8_t       _sda;
    uint8_t       _scl;
    bool          _synced = false;
    TimeData      _cached;
    String        _timezone;

    // PCF8563 getDayOfWeek returns 0=Sun, 1=Mon … 6=Sat.
    // Our DayOfWeek enum: Monday=1 … Sunday=7.
    static DayOfWeek pcfDowToEnum(uint32_t pcfDow);
};
