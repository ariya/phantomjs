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

#include "config.h"
#include "GregorianDateTime.h"

#include "DateMath.h"

#if OS(WINDOWS)
#include <windows.h>
#else
#include <time.h>
#endif

namespace WTF {

void GregorianDateTime::setToCurrentLocalTime()
{
#if OS(WINDOWS)
    SYSTEMTIME systemTime;
    GetLocalTime(&systemTime);
    TIME_ZONE_INFORMATION timeZoneInformation;
    DWORD timeZoneId = GetTimeZoneInformation(&timeZoneInformation);

    LONG bias = timeZoneInformation.Bias;
    if (timeZoneId == TIME_ZONE_ID_DAYLIGHT)
        bias += timeZoneInformation.DaylightBias;
    else if (timeZoneId == TIME_ZONE_ID_STANDARD)
        bias += timeZoneInformation.StandardBias;
    else
        ASSERT(timeZoneId == TIME_ZONE_ID_UNKNOWN);

    m_year = systemTime.wYear;
    m_month = systemTime.wMonth - 1;
    m_monthDay = systemTime.wDay;
    m_yearDay = dayInYear(m_year, m_month, m_monthDay);
    m_weekDay = systemTime.wDayOfWeek;
    m_hour = systemTime.wHour;
    m_minute = systemTime.wMinute;
    m_second = systemTime.wSecond;
    m_utcOffset = -bias * secondsPerMinute;
    m_isDST = timeZoneId == TIME_ZONE_ID_DAYLIGHT ? 1 : 0;
#else
    tm localTM;
    time_t localTime = time(0);
    localtime_r(&localTime, &localTM);

    m_year = localTM.tm_year + 1900;
    m_month = localTM.tm_mon;
    m_monthDay = localTM.tm_mday;
    m_yearDay = localTM.tm_yday;
    m_weekDay = localTM.tm_wday;
    m_hour = localTM.tm_hour;
    m_minute = localTM.tm_min;
    m_second = localTM.tm_sec;
    m_isDST = localTM.tm_isdst;
#if HAVE(TM_GMTOFF)
    m_utcOffset = localTM.tm_gmtoff;
#else
    m_utcOffset = calculateLocalTimeOffset(localTime * msPerSecond).offset / msPerSecond;
#endif
#endif
}

} // namespace WTF
