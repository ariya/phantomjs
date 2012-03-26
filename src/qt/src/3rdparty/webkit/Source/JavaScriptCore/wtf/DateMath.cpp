/*
 * Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 * Copyright (C) 2006, 2007 Apple Inc. All rights reserved.
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
#include "DateMath.h"

#include "Assertions.h"
#include "ASCIICType.h"
#include "CurrentTime.h"
#if USE(JSC)
#include "JSObject.h"
#endif
#include "MathExtras.h"
#if USE(JSC)
#include "ScopeChain.h"
#endif
#include "StdLibExtras.h"
#include "StringExtras.h"

#include <algorithm>
#include <limits.h>
#include <limits>
#include <stdint.h>
#include <time.h>


#if HAVE(ERRNO_H)
#include <errno.h>
#endif

#if OS(WINCE)
extern "C" size_t strftime(char * const s, const size_t maxsize, const char * const format, const struct tm * const t);
extern "C" struct tm * localtime(const time_t *timer);
#endif

#if HAVE(SYS_TIME_H)
#include <sys/time.h>
#endif

#if HAVE(SYS_TIMEB_H)
#include <sys/timeb.h>
#endif

#if USE(JSC)
#include "CallFrame.h"
#endif

#define NaN std::numeric_limits<double>::quiet_NaN()

using namespace WTF;

namespace WTF {

/* Constants */

static const double minutesPerDay = 24.0 * 60.0;
static const double secondsPerDay = 24.0 * 60.0 * 60.0;
static const double secondsPerYear = 24.0 * 60.0 * 60.0 * 365.0;

static const double usecPerSec = 1000000.0;

static const double maxUnixTime = 2145859200.0; // 12/31/2037
// ECMAScript asks not to support for a date of which total
// millisecond value is larger than the following value.
// See 15.9.1.14 of ECMA-262 5th edition.
static const double maxECMAScriptTime = 8.64E15;

// Day of year for the first day of each month, where index 0 is January, and day 0 is January 1.
// First for non-leap years, then for leap years.
static const int firstDayOfMonth[2][12] = {
    {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334},
    {0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335}
};

static inline bool isLeapYear(int year)
{
    if (year % 4 != 0)
        return false;
    if (year % 400 == 0)
        return true;
    if (year % 100 == 0)
        return false;
    return true;
}

static inline int daysInYear(int year)
{
    return 365 + isLeapYear(year);
}

static inline double daysFrom1970ToYear(int year)
{
    // The Gregorian Calendar rules for leap years:
    // Every fourth year is a leap year.  2004, 2008, and 2012 are leap years.
    // However, every hundredth year is not a leap year.  1900 and 2100 are not leap years.
    // Every four hundred years, there's a leap year after all.  2000 and 2400 are leap years.

    static const int leapDaysBefore1971By4Rule = 1970 / 4;
    static const int excludedLeapDaysBefore1971By100Rule = 1970 / 100;
    static const int leapDaysBefore1971By400Rule = 1970 / 400;

    const double yearMinusOne = year - 1;
    const double yearsToAddBy4Rule = floor(yearMinusOne / 4.0) - leapDaysBefore1971By4Rule;
    const double yearsToExcludeBy100Rule = floor(yearMinusOne / 100.0) - excludedLeapDaysBefore1971By100Rule;
    const double yearsToAddBy400Rule = floor(yearMinusOne / 400.0) - leapDaysBefore1971By400Rule;

    return 365.0 * (year - 1970) + yearsToAddBy4Rule - yearsToExcludeBy100Rule + yearsToAddBy400Rule;
}

static inline double msToDays(double ms)
{
    return floor(ms / msPerDay);
}

int msToYear(double ms)
{
    int approxYear = static_cast<int>(floor(ms / (msPerDay * 365.2425)) + 1970);
    double msFromApproxYearTo1970 = msPerDay * daysFrom1970ToYear(approxYear);
    if (msFromApproxYearTo1970 > ms)
        return approxYear - 1;
    if (msFromApproxYearTo1970 + msPerDay * daysInYear(approxYear) <= ms)
        return approxYear + 1;
    return approxYear;
}

int dayInYear(double ms, int year)
{
    return static_cast<int>(msToDays(ms) - daysFrom1970ToYear(year));
}

static inline double msToMilliseconds(double ms)
{
    double result = fmod(ms, msPerDay);
    if (result < 0)
        result += msPerDay;
    return result;
}

// 0: Sunday, 1: Monday, etc.
static inline int msToWeekDay(double ms)
{
    int wd = (static_cast<int>(msToDays(ms)) + 4) % 7;
    if (wd < 0)
        wd += 7;
    return wd;
}

static inline int msToSeconds(double ms)
{
    double result = fmod(floor(ms / msPerSecond), secondsPerMinute);
    if (result < 0)
        result += secondsPerMinute;
    return static_cast<int>(result);
}

static inline int msToMinutes(double ms)
{
    double result = fmod(floor(ms / msPerMinute), minutesPerHour);
    if (result < 0)
        result += minutesPerHour;
    return static_cast<int>(result);
}

static inline int msToHours(double ms)
{
    double result = fmod(floor(ms/msPerHour), hoursPerDay);
    if (result < 0)
        result += hoursPerDay;
    return static_cast<int>(result);
}

int monthFromDayInYear(int dayInYear, bool leapYear)
{
    const int d = dayInYear;
    int step;

    if (d < (step = 31))
        return 0;
    step += (leapYear ? 29 : 28);
    if (d < step)
        return 1;
    if (d < (step += 31))
        return 2;
    if (d < (step += 30))
        return 3;
    if (d < (step += 31))
        return 4;
    if (d < (step += 30))
        return 5;
    if (d < (step += 31))
        return 6;
    if (d < (step += 31))
        return 7;
    if (d < (step += 30))
        return 8;
    if (d < (step += 31))
        return 9;
    if (d < (step += 30))
        return 10;
    return 11;
}

static inline bool checkMonth(int dayInYear, int& startDayOfThisMonth, int& startDayOfNextMonth, int daysInThisMonth)
{
    startDayOfThisMonth = startDayOfNextMonth;
    startDayOfNextMonth += daysInThisMonth;
    return (dayInYear <= startDayOfNextMonth);
}

int dayInMonthFromDayInYear(int dayInYear, bool leapYear)
{
    const int d = dayInYear;
    int step;
    int next = 30;

    if (d <= next)
        return d + 1;
    const int daysInFeb = (leapYear ? 29 : 28);
    if (checkMonth(d, step, next, daysInFeb))
        return d - step;
    if (checkMonth(d, step, next, 31))
        return d - step;
    if (checkMonth(d, step, next, 30))
        return d - step;
    if (checkMonth(d, step, next, 31))
        return d - step;
    if (checkMonth(d, step, next, 30))
        return d - step;
    if (checkMonth(d, step, next, 31))
        return d - step;
    if (checkMonth(d, step, next, 31))
        return d - step;
    if (checkMonth(d, step, next, 30))
        return d - step;
    if (checkMonth(d, step, next, 31))
        return d - step;
    if (checkMonth(d, step, next, 30))
        return d - step;
    step = next;
    return d - step;
}

static inline int monthToDayInYear(int month, bool isLeapYear)
{
    return firstDayOfMonth[isLeapYear][month];
}

static inline double timeToMS(double hour, double min, double sec, double ms)
{
    return (((hour * minutesPerHour + min) * secondsPerMinute + sec) * msPerSecond + ms);
}

double dateToDaysFrom1970(int year, int month, int day)
{
    year += month / 12;

    month %= 12;
    if (month < 0) {
        month += 12;
        --year;
    }

    double yearday = floor(daysFrom1970ToYear(year));
    ASSERT((year >= 1970 && yearday >= 0) || (year < 1970 && yearday < 0));
    int monthday = monthToDayInYear(month, isLeapYear(year));

    return yearday + monthday + day - 1;
}

// There is a hard limit at 2038 that we currently do not have a workaround
// for (rdar://problem/5052975).
static inline int maximumYearForDST()
{
    return 2037;
}

static inline int minimumYearForDST()
{
    // Because of the 2038 issue (see maximumYearForDST) if the current year is
    // greater than the max year minus 27 (2010), we want to use the max year
    // minus 27 instead, to ensure there is a range of 28 years that all years
    // can map to.
    return std::min(msToYear(jsCurrentTime()), maximumYearForDST() - 27) ;
}

/*
 * Find an equivalent year for the one given, where equivalence is deterined by
 * the two years having the same leapness and the first day of the year, falling
 * on the same day of the week.
 *
 * This function returns a year between this current year and 2037, however this
 * function will potentially return incorrect results if the current year is after
 * 2010, (rdar://problem/5052975), if the year passed in is before 1900 or after
 * 2100, (rdar://problem/5055038).
 */
int equivalentYearForDST(int year)
{
    // It is ok if the cached year is not the current year as long as the rules
    // for DST did not change between the two years; if they did the app would need
    // to be restarted.
    static int minYear = minimumYearForDST();
    int maxYear = maximumYearForDST();

    int difference;
    if (year > maxYear)
        difference = minYear - year;
    else if (year < minYear)
        difference = maxYear - year;
    else
        return year;

    int quotient = difference / 28;
    int product = (quotient) * 28;

    year += product;
    ASSERT((year >= minYear && year <= maxYear) || (product - year == static_cast<int>(NaN)));
    return year;
}

int32_t calculateUTCOffset()
{
#if PLATFORM(BREWMP)
    time_t localTime = static_cast<time_t>(currentTime());
#else
    time_t localTime = time(0);
#endif
    tm localt;
    getLocalTime(&localTime, &localt);

    // Get the difference between this time zone and UTC on the 1st of January of this year.
    localt.tm_sec = 0;
    localt.tm_min = 0;
    localt.tm_hour = 0;
    localt.tm_mday = 1;
    localt.tm_mon = 0;
    // Not setting localt.tm_year!
    localt.tm_wday = 0;
    localt.tm_yday = 0;
    localt.tm_isdst = 0;
#if HAVE(TM_GMTOFF)
    localt.tm_gmtoff = 0;
#endif
#if HAVE(TM_ZONE)
    localt.tm_zone = 0;
#endif
    
#if HAVE(TIMEGM)
    time_t utcOffset = timegm(&localt) - mktime(&localt);
#else
    // Using a canned date of 01/01/2009 on platforms with weaker date-handling foo.
    localt.tm_year = 109;
    time_t utcOffset = 1230768000 - mktime(&localt);
#endif

    return static_cast<int32_t>(utcOffset * 1000);
}

/*
 * Get the DST offset for the time passed in.
 */
static double calculateDSTOffsetSimple(double localTimeSeconds, double utcOffset)
{
    if (localTimeSeconds > maxUnixTime)
        localTimeSeconds = maxUnixTime;
    else if (localTimeSeconds < 0) // Go ahead a day to make localtime work (does not work with 0)
        localTimeSeconds += secondsPerDay;

    //input is UTC so we have to shift back to local time to determine DST thus the + getUTCOffset()
    double offsetTime = (localTimeSeconds * msPerSecond) + utcOffset;

    // Offset from UTC but doesn't include DST obviously
    int offsetHour =  msToHours(offsetTime);
    int offsetMinute =  msToMinutes(offsetTime);

    // FIXME: time_t has a potential problem in 2038
    time_t localTime = static_cast<time_t>(localTimeSeconds);

    tm localTM;
    getLocalTime(&localTime, &localTM);

    double diff = ((localTM.tm_hour - offsetHour) * secondsPerHour) + ((localTM.tm_min - offsetMinute) * 60);

    if (diff < 0)
        diff += secondsPerDay;

    return (diff * msPerSecond);
}

// Get the DST offset, given a time in UTC
double calculateDSTOffset(double ms, double utcOffset)
{
    // On Mac OS X, the call to localtime (see calculateDSTOffsetSimple) will return historically accurate
    // DST information (e.g. New Zealand did not have DST from 1946 to 1974) however the JavaScript
    // standard explicitly dictates that historical information should not be considered when
    // determining DST. For this reason we shift away from years that localtime can handle but would
    // return historically accurate information.
    int year = msToYear(ms);
    int equivalentYear = equivalentYearForDST(year);
    if (year != equivalentYear) {
        bool leapYear = isLeapYear(year);
        int dayInYearLocal = dayInYear(ms, year);
        int dayInMonth = dayInMonthFromDayInYear(dayInYearLocal, leapYear);
        int month = monthFromDayInYear(dayInYearLocal, leapYear);
        double day = dateToDaysFrom1970(equivalentYear, month, dayInMonth);
        ms = (day * msPerDay) + msToMilliseconds(ms);
    }

    return calculateDSTOffsetSimple(ms / msPerSecond, utcOffset);
}

void initializeDates()
{
#ifndef NDEBUG
    static bool alreadyInitialized;
    ASSERT(!alreadyInitialized);
    alreadyInitialized = true;
#endif

    equivalentYearForDST(2000); // Need to call once to initialize a static used in this function.
}

static inline double ymdhmsToSeconds(long year, int mon, int day, int hour, int minute, double second)
{
    double days = (day - 32075)
        + floor(1461 * (year + 4800.0 + (mon - 14) / 12) / 4)
        + 367 * (mon - 2 - (mon - 14) / 12 * 12) / 12
        - floor(3 * ((year + 4900.0 + (mon - 14) / 12) / 100) / 4)
        - 2440588;
    return ((days * hoursPerDay + hour) * minutesPerHour + minute) * secondsPerMinute + second;
}

// We follow the recommendation of RFC 2822 to consider all
// obsolete time zones not listed here equivalent to "-0000".
static const struct KnownZone {
#if !OS(WINDOWS)
    const
#endif
        char tzName[4];
    int tzOffset;
} known_zones[] = {
    { "UT", 0 },
    { "GMT", 0 },
    { "EST", -300 },
    { "EDT", -240 },
    { "CST", -360 },
    { "CDT", -300 },
    { "MST", -420 },
    { "MDT", -360 },
    { "PST", -480 },
    { "PDT", -420 }
};

inline static void skipSpacesAndComments(const char*& s)
{
    int nesting = 0;
    char ch;
    while ((ch = *s)) {
        if (!isASCIISpace(ch)) {
            if (ch == '(')
                nesting++;
            else if (ch == ')' && nesting > 0)
                nesting--;
            else if (nesting == 0)
                break;
        }
        s++;
    }
}

// returns 0-11 (Jan-Dec); -1 on failure
static int findMonth(const char* monthStr)
{
    ASSERT(monthStr);
    char needle[4];
    for (int i = 0; i < 3; ++i) {
        if (!*monthStr)
            return -1;
        needle[i] = static_cast<char>(toASCIILower(*monthStr++));
    }
    needle[3] = '\0';
    const char *haystack = "janfebmaraprmayjunjulaugsepoctnovdec";
    const char *str = strstr(haystack, needle);
    if (str) {
        int position = static_cast<int>(str - haystack);
        if (position % 3 == 0)
            return position / 3;
    }
    return -1;
}

static bool parseLong(const char* string, char** stopPosition, int base, long* result)
{
    *result = strtol(string, stopPosition, base);
    // Avoid the use of errno as it is not available on Windows CE
    if (string == *stopPosition || *result == LONG_MIN || *result == LONG_MAX)
        return false;
    return true;
}

double parseES5DateFromNullTerminatedCharacters(const char* dateString)
{
    // This parses a date of the form defined in ECMA-262-5, section 15.9.1.15
    // (similar to RFC 3339 / ISO 8601: YYYY-MM-DDTHH:mm:ss[.sss]Z).
    // In most cases it is intentionally strict (e.g. correct field widths, no stray whitespace).
    
    static const long daysPerMonth[12] = { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    
    const char* currentPosition = dateString;
    char* postParsePosition;
    
    // This is a bit more lenient on the year string than ES5 specifies:
    // instead of restricting to 4 digits (or 6 digits with mandatory +/-),
    // it accepts any integer value. Consider this an implementation fallback.
    long year;
    if (!parseLong(currentPosition, &postParsePosition, 10, &year))
        return NaN;
    if (*postParsePosition != '-')
        return NaN;
    currentPosition = postParsePosition + 1;
    
    long month;
    if (!isASCIIDigit(*currentPosition))
        return NaN;
    if (!parseLong(currentPosition, &postParsePosition, 10, &month))
        return NaN;
    if (*postParsePosition != '-' || (postParsePosition - currentPosition) != 2)
        return NaN;
    currentPosition = postParsePosition + 1;
    
    long day;
    if (!isASCIIDigit(*currentPosition))
        return NaN;
    if (!parseLong(currentPosition, &postParsePosition, 10, &day))
        return NaN;
    if (*postParsePosition != 'T' || (postParsePosition - currentPosition) != 2)
        return NaN;
    currentPosition = postParsePosition + 1;
    
    long hours;
    if (!isASCIIDigit(*currentPosition))
        return NaN;
    if (!parseLong(currentPosition, &postParsePosition, 10, &hours))
        return NaN;
    if (*postParsePosition != ':' || (postParsePosition - currentPosition) != 2)
        return NaN;
    currentPosition = postParsePosition + 1;
    
    long minutes;
    if (!isASCIIDigit(*currentPosition))
        return NaN;
    if (!parseLong(currentPosition, &postParsePosition, 10, &minutes))
        return NaN;
    if (*postParsePosition != ':' || (postParsePosition - currentPosition) != 2)
        return NaN;
    currentPosition = postParsePosition + 1;
    
    long intSeconds;
    if (!isASCIIDigit(*currentPosition))
        return NaN;
    if (!parseLong(currentPosition, &postParsePosition, 10, &intSeconds))
        return NaN;
    if ((postParsePosition - currentPosition) != 2)
        return NaN;
    
    double seconds = intSeconds;
    if (*postParsePosition == '.') {
        currentPosition = postParsePosition + 1;
        
        // In ECMA-262-5 it's a bit unclear if '.' can be present without milliseconds, but
        // a reasonable interpretation guided by the given examples and RFC 3339 says "no".
        // We check the next character to avoid reading +/- timezone hours after an invalid decimal.
        if (!isASCIIDigit(*currentPosition))
            return NaN;
        
        // We are more lenient than ES5 by accepting more or less than 3 fraction digits.
        long fracSeconds;
        if (!parseLong(currentPosition, &postParsePosition, 10, &fracSeconds))
            return NaN;
        
        long numFracDigits = postParsePosition - currentPosition;
        seconds += fracSeconds * pow(10.0, static_cast<double>(-numFracDigits));
    }
    currentPosition = postParsePosition;
    
    // A few of these checks could be done inline above, but since many of them are interrelated
    // we would be sacrificing readability to "optimize" the (presumably less common) failure path.
    if (month < 1 || month > 12)
        return NaN;
    if (day < 1 || day > daysPerMonth[month - 1])
        return NaN;
    if (month == 2 && day > 28 && !isLeapYear(year))
        return NaN;
    if (hours < 0 || hours > 24)
        return NaN;
    if (hours == 24 && (minutes || seconds))
        return NaN;
    if (minutes < 0 || minutes > 59)
        return NaN;
    if (seconds < 0 || seconds >= 61)
        return NaN;
    if (seconds > 60) {
        // Discard leap seconds by clamping to the end of a minute.
        seconds = 60;
    }
    
    long timeZoneSeconds = 0;
    if (*currentPosition != 'Z') {
        bool tzNegative;
        if (*currentPosition == '-')
            tzNegative = true;
        else if (*currentPosition == '+')
            tzNegative = false;
        else
            return NaN;
        currentPosition += 1;
        
        long tzHours;
        long tzHoursAbs;
        long tzMinutes;
        
        if (!isASCIIDigit(*currentPosition))
            return NaN;
        if (!parseLong(currentPosition, &postParsePosition, 10, &tzHours))
            return NaN;
        if (*postParsePosition != ':' || (postParsePosition - currentPosition) != 2)
            return NaN;
        tzHoursAbs = labs(tzHours);
        currentPosition = postParsePosition + 1;
        
        if (!isASCIIDigit(*currentPosition))
            return NaN;
        if (!parseLong(currentPosition, &postParsePosition, 10, &tzMinutes))
            return NaN;
        if ((postParsePosition - currentPosition) != 2)
            return NaN;
        currentPosition = postParsePosition;
        
        if (tzHoursAbs > 24)
            return NaN;
        if (tzMinutes < 0 || tzMinutes > 59)
            return NaN;
        
        timeZoneSeconds = 60 * (tzMinutes + (60 * tzHoursAbs));
        if (tzNegative)
            timeZoneSeconds = -timeZoneSeconds;
    } else {
        currentPosition += 1;
    }
    if (*currentPosition)
        return NaN;
    
    double dateSeconds = ymdhmsToSeconds(year, month, day, hours, minutes, seconds) - timeZoneSeconds;
    return dateSeconds * msPerSecond;
}

// Odd case where 'exec' is allowed to be 0, to accomodate a caller in WebCore.
static double parseDateFromNullTerminatedCharacters(const char* dateString, bool& haveTZ, int& offset)
{
    haveTZ = false;
    offset = 0;

    // This parses a date in the form:
    //     Tuesday, 09-Nov-99 23:12:40 GMT
    // or
    //     Sat, 01-Jan-2000 08:00:00 GMT
    // or
    //     Sat, 01 Jan 2000 08:00:00 GMT
    // or
    //     01 Jan 99 22:00 +0100    (exceptions in rfc822/rfc2822)
    // ### non RFC formats, added for Javascript:
    //     [Wednesday] January 09 1999 23:12:40 GMT
    //     [Wednesday] January 09 23:12:40 GMT 1999
    //
    // We ignore the weekday.
     
    // Skip leading space
    skipSpacesAndComments(dateString);

    long month = -1;
    const char *wordStart = dateString;
    // Check contents of first words if not number
    while (*dateString && !isASCIIDigit(*dateString)) {
        if (isASCIISpace(*dateString) || *dateString == '(') {
            if (dateString - wordStart >= 3)
                month = findMonth(wordStart);
            skipSpacesAndComments(dateString);
            wordStart = dateString;
        } else
           dateString++;
    }

    // Missing delimiter between month and day (like "January29")?
    if (month == -1 && wordStart != dateString)
        month = findMonth(wordStart);

    skipSpacesAndComments(dateString);

    if (!*dateString)
        return NaN;

    // ' 09-Nov-99 23:12:40 GMT'
    char* newPosStr;
    long day;
    if (!parseLong(dateString, &newPosStr, 10, &day))
        return NaN;
    dateString = newPosStr;

    if (!*dateString)
        return NaN;

    if (day < 0)
        return NaN;

    long year = 0;
    if (day > 31) {
        // ### where is the boundary and what happens below?
        if (*dateString != '/')
            return NaN;
        // looks like a YYYY/MM/DD date
        if (!*++dateString)
            return NaN;
        year = day;
        if (!parseLong(dateString, &newPosStr, 10, &month))
            return NaN;
        month -= 1;
        dateString = newPosStr;
        if (*dateString++ != '/' || !*dateString)
            return NaN;
        if (!parseLong(dateString, &newPosStr, 10, &day))
            return NaN;
        dateString = newPosStr;
    } else if (*dateString == '/' && month == -1) {
        dateString++;
        // This looks like a MM/DD/YYYY date, not an RFC date.
        month = day - 1; // 0-based
        if (!parseLong(dateString, &newPosStr, 10, &day))
            return NaN;
        if (day < 1 || day > 31)
            return NaN;
        dateString = newPosStr;
        if (*dateString == '/')
            dateString++;
        if (!*dateString)
            return NaN;
     } else {
        if (*dateString == '-')
            dateString++;

        skipSpacesAndComments(dateString);

        if (*dateString == ',')
            dateString++;

        if (month == -1) { // not found yet
            month = findMonth(dateString);
            if (month == -1)
                return NaN;

            while (*dateString && *dateString != '-' && *dateString != ',' && !isASCIISpace(*dateString))
                dateString++;

            if (!*dateString)
                return NaN;

            // '-99 23:12:40 GMT'
            if (*dateString != '-' && *dateString != '/' && *dateString != ',' && !isASCIISpace(*dateString))
                return NaN;
            dateString++;
        }
    }

    if (month < 0 || month > 11)
        return NaN;

    // '99 23:12:40 GMT'
    if (year <= 0 && *dateString) {
        if (!parseLong(dateString, &newPosStr, 10, &year))
            return NaN;
    }

    // Don't fail if the time is missing.
    long hour = 0;
    long minute = 0;
    long second = 0;
    if (!*newPosStr)
        dateString = newPosStr;
    else {
        // ' 23:12:40 GMT'
        if (!(isASCIISpace(*newPosStr) || *newPosStr == ',')) {
            if (*newPosStr != ':')
                return NaN;
            // There was no year; the number was the hour.
            year = -1;
        } else {
            // in the normal case (we parsed the year), advance to the next number
            dateString = ++newPosStr;
            skipSpacesAndComments(dateString);
        }

        parseLong(dateString, &newPosStr, 10, &hour);
        // Do not check for errno here since we want to continue
        // even if errno was set becasue we are still looking
        // for the timezone!

        // Read a number? If not, this might be a timezone name.
        if (newPosStr != dateString) {
            dateString = newPosStr;

            if (hour < 0 || hour > 23)
                return NaN;

            if (!*dateString)
                return NaN;

            // ':12:40 GMT'
            if (*dateString++ != ':')
                return NaN;

            if (!parseLong(dateString, &newPosStr, 10, &minute))
                return NaN;
            dateString = newPosStr;

            if (minute < 0 || minute > 59)
                return NaN;

            // ':40 GMT'
            if (*dateString && *dateString != ':' && !isASCIISpace(*dateString))
                return NaN;

            // seconds are optional in rfc822 + rfc2822
            if (*dateString ==':') {
                dateString++;

                if (!parseLong(dateString, &newPosStr, 10, &second))
                    return NaN;
                dateString = newPosStr;

                if (second < 0 || second > 59)
                    return NaN;
            }

            skipSpacesAndComments(dateString);

            if (strncasecmp(dateString, "AM", 2) == 0) {
                if (hour > 12)
                    return NaN;
                if (hour == 12)
                    hour = 0;
                dateString += 2;
                skipSpacesAndComments(dateString);
            } else if (strncasecmp(dateString, "PM", 2) == 0) {
                if (hour > 12)
                    return NaN;
                if (hour != 12)
                    hour += 12;
                dateString += 2;
                skipSpacesAndComments(dateString);
            }
        }
    }

    // Don't fail if the time zone is missing. 
    // Some websites omit the time zone (4275206).
    if (*dateString) {
        if (strncasecmp(dateString, "GMT", 3) == 0 || strncasecmp(dateString, "UTC", 3) == 0) {
            dateString += 3;
            haveTZ = true;
        }

        if (*dateString == '+' || *dateString == '-') {
            long o;
            if (!parseLong(dateString, &newPosStr, 10, &o))
                return NaN;
            dateString = newPosStr;

            if (o < -9959 || o > 9959)
                return NaN;

            int sgn = (o < 0) ? -1 : 1;
            o = labs(o);
            if (*dateString != ':') {
                offset = ((o / 100) * 60 + (o % 100)) * sgn;
            } else { // GMT+05:00
                long o2;
                if (!parseLong(dateString, &newPosStr, 10, &o2))
                    return NaN;
                dateString = newPosStr;
                offset = (o * 60 + o2) * sgn;
            }
            haveTZ = true;
        } else {
            for (size_t i = 0; i < WTF_ARRAY_LENGTH(known_zones); ++i) {
                if (0 == strncasecmp(dateString, known_zones[i].tzName, strlen(known_zones[i].tzName))) {
                    offset = known_zones[i].tzOffset;
                    dateString += strlen(known_zones[i].tzName);
                    haveTZ = true;
                    break;
                }
            }
        }
    }

    skipSpacesAndComments(dateString);

    if (*dateString && year == -1) {
        if (!parseLong(dateString, &newPosStr, 10, &year))
            return NaN;
        dateString = newPosStr;
    }

    skipSpacesAndComments(dateString);

    // Trailing garbage
    if (*dateString)
        return NaN;

    // Y2K: Handle 2 digit years.
    if (year >= 0 && year < 100) {
        if (year < 50)
            year += 2000;
        else
            year += 1900;
    }
    
    return ymdhmsToSeconds(year, month + 1, day, hour, minute, second) * msPerSecond;
}

double parseDateFromNullTerminatedCharacters(const char* dateString)
{
    bool haveTZ;
    int offset;
    double ms = parseDateFromNullTerminatedCharacters(dateString, haveTZ, offset);
    if (isnan(ms))
        return NaN;

    // fall back to local timezone
    if (!haveTZ) {
        double utcOffset = calculateUTCOffset();
        double dstOffset = calculateDSTOffset(ms, utcOffset);
        offset = static_cast<int>((utcOffset + dstOffset) / msPerMinute);
    }
    return ms - (offset * msPerMinute);
}

double timeClip(double t)
{
    if (!isfinite(t))
        return NaN;
    if (fabs(t) > maxECMAScriptTime)
        return NaN;
    return trunc(t);
}
} // namespace WTF

#if USE(JSC)
namespace JSC {

// Get the DST offset for the time passed in.
//
// NOTE: The implementation relies on the fact that no time zones have
// more than one daylight savings offset change per month.
// If this function is called with NaN it returns NaN.
static double getDSTOffset(ExecState* exec, double ms, double utcOffset)
{
    DSTOffsetCache& cache = exec->globalData().dstOffsetCache;
    double start = cache.start;
    double end = cache.end;

    if (start <= ms) {
        // If the time fits in the cached interval, return the cached offset.
        if (ms <= end) return cache.offset;

        // Compute a possible new interval end.
        double newEnd = end + cache.increment;

        if (ms <= newEnd) {
            double endOffset = calculateDSTOffset(newEnd, utcOffset);
            if (cache.offset == endOffset) {
                // If the offset at the end of the new interval still matches
                // the offset in the cache, we grow the cached time interval
                // and return the offset.
                cache.end = newEnd;
                cache.increment = msPerMonth;
                return endOffset;
            } else {
                double offset = calculateDSTOffset(ms, utcOffset);
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
    }

    // Compute the DST offset for the time and shrink the cache interval
    // to only contain the time. This allows fast repeated DST offset
    // computations for the same time.
    double offset = calculateDSTOffset(ms, utcOffset);
    cache.offset = offset;
    cache.start = ms;
    cache.end = ms;
    cache.increment = msPerMonth;
    return offset;
}

/*
 * Get the difference in milliseconds between this time zone and UTC (GMT)
 * NOT including DST.
 */
double getUTCOffset(ExecState* exec)
{
    double utcOffset = exec->globalData().cachedUTCOffset;
    if (!isnan(utcOffset))
        return utcOffset;
    exec->globalData().cachedUTCOffset = calculateUTCOffset();
    return exec->globalData().cachedUTCOffset;
}

double gregorianDateTimeToMS(ExecState* exec, const GregorianDateTime& t, double milliSeconds, bool inputIsUTC)
{
    double day = dateToDaysFrom1970(t.year + 1900, t.month, t.monthDay);
    double ms = timeToMS(t.hour, t.minute, t.second, milliSeconds);
    double result = (day * WTF::msPerDay) + ms;

    if (!inputIsUTC) { // convert to UTC
        double utcOffset = getUTCOffset(exec);
        result -= utcOffset;
        result -= getDSTOffset(exec, result, utcOffset);
    }

    return result;
}

// input is UTC
void msToGregorianDateTime(ExecState* exec, double ms, bool outputIsUTC, GregorianDateTime& tm)
{
    double dstOff = 0.0;
    double utcOff = 0.0;
    if (!outputIsUTC) {
        utcOff = getUTCOffset(exec);
        dstOff = getDSTOffset(exec, ms, utcOff);
        ms += dstOff + utcOff;
    }

    const int year = msToYear(ms);
    tm.second   =  msToSeconds(ms);
    tm.minute   =  msToMinutes(ms);
    tm.hour     =  msToHours(ms);
    tm.weekDay  =  msToWeekDay(ms);
    tm.yearDay  =  dayInYear(ms, year);
    tm.monthDay =  dayInMonthFromDayInYear(tm.yearDay, isLeapYear(year));
    tm.month    =  monthFromDayInYear(tm.yearDay, isLeapYear(year));
    tm.year     =  year - 1900;
    tm.isDST    =  dstOff != 0.0;
    tm.utcOffset = static_cast<long>((dstOff + utcOff) / WTF::msPerSecond);
    tm.timeZone = nullptr;
}

double parseDateFromNullTerminatedCharacters(ExecState* exec, const char* dateString)
{
    ASSERT(exec);
    bool haveTZ;
    int offset;
    double ms = WTF::parseDateFromNullTerminatedCharacters(dateString, haveTZ, offset);
    if (isnan(ms))
        return NaN;

    // fall back to local timezone
    if (!haveTZ) {
        double utcOffset = getUTCOffset(exec);
        double dstOffset = getDSTOffset(exec, ms, utcOffset);
        offset = static_cast<int>((utcOffset + dstOffset) / WTF::msPerMinute);
    }
    return ms - (offset * WTF::msPerMinute);
}

} // namespace JSC
#endif // USE(JSC)
