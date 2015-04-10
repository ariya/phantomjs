/*
 * Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 * Copyright (C) 2006, 2007, 2012 Apple Inc. All rights reserved.
 * Copyright (C) 2009 Google Inc. All rights reserved.
 * Copyright (C) 2007-2009 Torch Mobile, Inc.
 * Copyright (C) 2010 &yet, LLC. (nate@andyet.net)
 *
 * The Original Code is Mozilla Communicator client code, released
 * March 31, 1998.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Alternatively, the contents of this file may be used under the terms
 * of either the Mozilla Public License Version 1.1, found at
 * http://www.mozilla.org/MPL/ (the "MPL") or the GNU General Public
 * License Version 2.0, found at http://www.fsf.org/copyleft/gpl.html
 * (the "GPL"), in which case the provisions of the MPL or the GPL are
 * applicable instead of those above.  If you wish to allow use of your
 * version of this file only under the terms of one of those two
 * licenses (the MPL or the GPL) and not to allow others to use your
 * version of this file under the LGPL, indicate your decision by
 * deletingthe provisions above and replace them with the notice and
 * other provisions required by the MPL or the GPL, as the case may be.
 * If you do not delete the provisions above, a recipient may use your
 * version of this file under any of the LGPL, the MPL or the GPL.

 * Copyright 2006-2008 the V8 project authors. All rights reserved.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of Google Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "JSDateMath.h"

#include "JSObject.h"
#include "JSScope.h"
#include "Operations.h"

#include <algorithm>
#include <limits.h>
#include <limits>
#include <stdint.h>
#include <time.h>
#include <wtf/ASCIICType.h>
#include <wtf/Assertions.h>
#include <wtf/CurrentTime.h>
#include <wtf/MathExtras.h>
#include <wtf/StdLibExtras.h>
#include <wtf/StringExtras.h>
#include <wtf/text/StringBuilder.h>

#if HAVE(ERRNO_H)
#include <errno.h>
#endif

#if HAVE(SYS_TIME_H)
#include <sys/time.h>
#endif

#if HAVE(SYS_TIMEB_H)
#include <sys/timeb.h>
#endif

using namespace WTF;

namespace JSC {

static inline double timeToMS(double hour, double min, double sec, double ms)
{
    return (((hour * minutesPerHour + min) * secondsPerMinute + sec) * msPerSecond + ms);
}

static inline int msToSeconds(double ms)
{
    double result = fmod(floor(ms / msPerSecond), secondsPerMinute);
    if (result < 0)
        result += secondsPerMinute;
    return static_cast<int>(result);
}

// 0: Sunday, 1: Monday, etc.
static inline int msToWeekDay(double ms)
{
    int wd = (static_cast<int>(msToDays(ms)) + 4) % 7;
    if (wd < 0)
        wd += 7;
    return wd;
}

// Get the combined UTC + DST offset for the time passed in.
//
// NOTE: The implementation relies on the fact that no time zones have
// more than one daylight savings offset change per month.
// If this function is called with NaN it returns NaN.
static LocalTimeOffset localTimeOffset(ExecState* exec, double ms)
{
    LocalTimeOffsetCache& cache = exec->vm().localTimeOffsetCache;
    double start = cache.start;
    double end = cache.end;

    if (start <= ms) {
        // If the time fits in the cached interval, return the cached offset.
        if (ms <= end) return cache.offset;

        // Compute a possible new interval end.
        double newEnd = end + cache.increment;

        if (ms <= newEnd) {
            LocalTimeOffset endOffset = calculateLocalTimeOffset(newEnd);
            if (cache.offset == endOffset) {
                // If the offset at the end of the new interval still matches
                // the offset in the cache, we grow the cached time interval
                // and return the offset.
                cache.end = newEnd;
                cache.increment = msPerMonth;
                return endOffset;
            }
            LocalTimeOffset offset = calculateLocalTimeOffset(ms);
            if (offset == endOffset) {
                // The offset at the given time is equal to the offset at the
                // new end of the interval, so that means that we've just skipped
                // the point in time where the DST offset change occurred. Updated
                // the interval to reflect this and reset the increment.
                cache.start = ms;
                cache.end = newEnd;
                cache.increment = msPerMonth;
            } else {
                // The interval contains a DST offset change and the given time is
                // before it. Adjust the increment to avoid a linear search for
                // the offset change point and change the end of the interval.
                cache.increment /= 3;
                cache.end = ms;
            }
            // Update the offset in the cache and return it.
            cache.offset = offset;
            return offset;
        }
    }

    // Compute the DST offset for the time and shrink the cache interval
    // to only contain the time. This allows fast repeated DST offset
    // computations for the same time.
    LocalTimeOffset offset = calculateLocalTimeOffset(ms);
    cache.offset = offset;
    cache.start = ms;
    cache.end = ms;
    cache.increment = msPerMonth;
    return offset;
}

double gregorianDateTimeToMS(ExecState* exec, const GregorianDateTime& t, double milliSeconds, bool inputIsUTC)
{
    double day = dateToDaysFrom1970(t.year(), t.month(), t.monthDay());
    double ms = timeToMS(t.hour(), t.minute(), t.second(), milliSeconds);
    double result = (day * WTF::msPerDay) + ms;

    if (!inputIsUTC)
        result -= localTimeOffset(exec, result).offset;

    return result;
}

// input is UTC
void msToGregorianDateTime(ExecState* exec, double ms, bool outputIsUTC, GregorianDateTime& tm)
{
    LocalTimeOffset localTime;
    if (!outputIsUTC) {
        localTime = localTimeOffset(exec, ms);
        ms += localTime.offset;
    }

    const int year = msToYear(ms);
    tm.setSecond(msToSeconds(ms));
    tm.setMinute(msToMinutes(ms));
    tm.setHour(msToHours(ms));
    tm.setWeekDay(msToWeekDay(ms));
    tm.setYearDay(dayInYear(ms, year));
    tm.setMonthDay(dayInMonthFromDayInYear(tm.yearDay(), isLeapYear(year)));
    tm.setMonth(monthFromDayInYear(tm.yearDay(), isLeapYear(year)));
    tm.setYear(year);
    tm.setIsDST(localTime.isDST);
    tm.setUtcOffset(localTime.offset / WTF::msPerSecond);
}

double parseDateFromNullTerminatedCharacters(ExecState* exec, const char* dateString)
{
    ASSERT(exec);
    bool haveTZ;
    int offset;
    double ms = WTF::parseDateFromNullTerminatedCharacters(dateString, haveTZ, offset);
    if (std::isnan(ms))
        return QNaN;

    // fall back to local timezone
    if (!haveTZ)
        offset = localTimeOffset(exec, ms).offset / WTF::msPerMinute;

    return ms - (offset * WTF::msPerMinute);
}

double parseDate(ExecState* exec, const String& date)
{
    if (date == exec->vm().cachedDateString)
        return exec->vm().cachedDateStringValue;
    double value = parseES5DateFromNullTerminatedCharacters(date.utf8().data());
    if (std::isnan(value))
        value = parseDateFromNullTerminatedCharacters(exec, date.utf8().data());
    exec->vm().cachedDateString = date;
    exec->vm().cachedDateStringValue = value;
    return value;
}

} // namespace JSC
