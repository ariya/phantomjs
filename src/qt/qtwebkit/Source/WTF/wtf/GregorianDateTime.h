/*
 * Copyright (C) 2012 Patrick Gansterer <paroga@paroga.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WTF_GregorianDateTime_h
#define WTF_GregorianDateTime_h

#include <string.h>
#include <time.h>
#include <wtf/Noncopyable.h>

namespace WTF {

class GregorianDateTime {
    WTF_MAKE_NONCOPYABLE(GregorianDateTime);
public:
    GregorianDateTime()
        : m_year(0)
        , m_month(0)
        , m_yearDay(0)
        , m_monthDay(0)
        , m_weekDay(0)
        , m_hour(0)
        , m_minute(0)
        , m_second(0)
        , m_utcOffset(0)
        , m_isDST(0)
    {
    }

    inline int year() const { return m_year; }
    inline int month() const { return m_month; }
    inline int yearDay() const { return m_yearDay; }
    inline int monthDay() const { return m_monthDay; }
    inline int weekDay() const { return m_weekDay; }
    inline int hour() const { return m_hour; }
    inline int minute() const { return m_minute; }
    inline int second() const { return m_second; }
    inline int utcOffset() const { return m_utcOffset; }
    inline int isDST() const { return m_isDST; }

    inline void setYear(int year) { m_year = year; }
    inline void setMonth(int month) { m_month = month; }
    inline void setYearDay(int yearDay) { m_yearDay = yearDay; }
    inline void setMonthDay(int monthDay) { m_monthDay = monthDay; }
    inline void setWeekDay(int weekDay) { m_weekDay = weekDay; }
    inline void setHour(int hour) { m_hour = hour; }
    inline void setMinute(int minute) { m_minute = minute; }
    inline void setSecond(int second) { m_second = second; }
    inline void setUtcOffset(int utcOffset) { m_utcOffset = utcOffset; }
    inline void setIsDST(int isDST) { m_isDST = isDST; }

    WTF_EXPORT_PRIVATE void setToCurrentLocalTime();

    operator tm() const
    {
        tm ret;
        memset(&ret, 0, sizeof(ret));

        ret.tm_year = m_year - 1900;
        ret.tm_mon = m_month;
        ret.tm_yday = m_yearDay;
        ret.tm_mday = m_monthDay;
        ret.tm_wday = m_weekDay;
        ret.tm_hour = m_hour;
        ret.tm_min = m_minute;
        ret.tm_sec = m_second;
        ret.tm_isdst = m_isDST;

#if HAVE(TM_GMTOFF)
        ret.tm_gmtoff = static_cast<long>(m_utcOffset);
#endif

        return ret;
    }

    void copyFrom(const GregorianDateTime& other)
    {
        m_year = other.m_year;
        m_month = other.m_month;
        m_yearDay = other.m_yearDay;
        m_monthDay = other.m_monthDay;
        m_weekDay = other.m_weekDay;
        m_hour = other.m_hour;
        m_minute = other.m_minute;
        m_second = other.m_second;
        m_utcOffset = other.m_utcOffset;
        m_isDST = other.m_isDST;
    }

private:
    int m_year;
    int m_month;
    int m_yearDay;
    int m_monthDay;
    int m_weekDay;
    int m_hour;
    int m_minute;
    int m_second;
    int m_utcOffset;
    int m_isDST;
};

} // namespace WTF

using WTF::GregorianDateTime;

#endif // WTF_GregorianDateTime_h
