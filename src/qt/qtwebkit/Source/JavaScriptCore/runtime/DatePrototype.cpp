/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
 *  Copyright (C) 2008, 2009 Torch Mobile, Inc. All rights reserved.
 *  Copyright (C) 2010 Torch Mobile (Beijing) Co. Ltd. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 *
 */

#include "config.h"
#include "DatePrototype.h"

#include "DateConversion.h"
#include "DateInstance.h"
#include "Error.h"
#include "JSDateMath.h"
#include "JSGlobalObject.h"
#include "JSString.h"
#include "JSStringBuilder.h"
#include "Lookup.h"
#include "ObjectPrototype.h"
#include "Operations.h"

#if !PLATFORM(MAC) && HAVE(LANGINFO_H)
#include <langinfo.h>
#endif

#include <limits.h>
#include <locale.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <wtf/Assertions.h>
#include <wtf/MathExtras.h>
#include <wtf/StringExtras.h>

#if HAVE(SYS_PARAM_H)
#include <sys/param.h>
#endif

#if HAVE(SYS_TIME_H)
#include <sys/time.h>
#endif

#if HAVE(SYS_TIMEB_H)
#include <sys/timeb.h>
#endif

#if OS(DARWIN) && USE(CF)
#include <CoreFoundation/CoreFoundation.h>
#elif USE(ICU_UNICODE)
#include <unicode/udat.h>
#endif

using namespace WTF;

namespace JSC {

static EncodedJSValue JSC_HOST_CALL dateProtoFuncGetDate(ExecState*);
static EncodedJSValue JSC_HOST_CALL dateProtoFuncGetDay(ExecState*);
static EncodedJSValue JSC_HOST_CALL dateProtoFuncGetFullYear(ExecState*);
static EncodedJSValue JSC_HOST_CALL dateProtoFuncGetHours(ExecState*);
static EncodedJSValue JSC_HOST_CALL dateProtoFuncGetMilliSeconds(ExecState*);
static EncodedJSValue JSC_HOST_CALL dateProtoFuncGetMinutes(ExecState*);
static EncodedJSValue JSC_HOST_CALL dateProtoFuncGetMonth(ExecState*);
static EncodedJSValue JSC_HOST_CALL dateProtoFuncGetSeconds(ExecState*);
static EncodedJSValue JSC_HOST_CALL dateProtoFuncGetTime(ExecState*);
static EncodedJSValue JSC_HOST_CALL dateProtoFuncGetTimezoneOffset(ExecState*);
static EncodedJSValue JSC_HOST_CALL dateProtoFuncGetUTCDate(ExecState*);
static EncodedJSValue JSC_HOST_CALL dateProtoFuncGetUTCDay(ExecState*);
static EncodedJSValue JSC_HOST_CALL dateProtoFuncGetUTCFullYear(ExecState*);
static EncodedJSValue JSC_HOST_CALL dateProtoFuncGetUTCHours(ExecState*);
static EncodedJSValue JSC_HOST_CALL dateProtoFuncGetUTCMilliseconds(ExecState*);
static EncodedJSValue JSC_HOST_CALL dateProtoFuncGetUTCMinutes(ExecState*);
static EncodedJSValue JSC_HOST_CALL dateProtoFuncGetUTCMonth(ExecState*);
static EncodedJSValue JSC_HOST_CALL dateProtoFuncGetUTCSeconds(ExecState*);
static EncodedJSValue JSC_HOST_CALL dateProtoFuncGetYear(ExecState*);
static EncodedJSValue JSC_HOST_CALL dateProtoFuncSetDate(ExecState*);
static EncodedJSValue JSC_HOST_CALL dateProtoFuncSetFullYear(ExecState*);
static EncodedJSValue JSC_HOST_CALL dateProtoFuncSetHours(ExecState*);
static EncodedJSValue JSC_HOST_CALL dateProtoFuncSetMilliSeconds(ExecState*);
static EncodedJSValue JSC_HOST_CALL dateProtoFuncSetMinutes(ExecState*);
static EncodedJSValue JSC_HOST_CALL dateProtoFuncSetMonth(ExecState*);
static EncodedJSValue JSC_HOST_CALL dateProtoFuncSetSeconds(ExecState*);
static EncodedJSValue JSC_HOST_CALL dateProtoFuncSetTime(ExecState*);
static EncodedJSValue JSC_HOST_CALL dateProtoFuncSetUTCDate(ExecState*);
static EncodedJSValue JSC_HOST_CALL dateProtoFuncSetUTCFullYear(ExecState*);
static EncodedJSValue JSC_HOST_CALL dateProtoFuncSetUTCHours(ExecState*);
static EncodedJSValue JSC_HOST_CALL dateProtoFuncSetUTCMilliseconds(ExecState*);
static EncodedJSValue JSC_HOST_CALL dateProtoFuncSetUTCMinutes(ExecState*);
static EncodedJSValue JSC_HOST_CALL dateProtoFuncSetUTCMonth(ExecState*);
static EncodedJSValue JSC_HOST_CALL dateProtoFuncSetUTCSeconds(ExecState*);
static EncodedJSValue JSC_HOST_CALL dateProtoFuncSetYear(ExecState*);
static EncodedJSValue JSC_HOST_CALL dateProtoFuncToDateString(ExecState*);
static EncodedJSValue JSC_HOST_CALL dateProtoFuncToGMTString(ExecState*);
static EncodedJSValue JSC_HOST_CALL dateProtoFuncToLocaleDateString(ExecState*);
static EncodedJSValue JSC_HOST_CALL dateProtoFuncToLocaleString(ExecState*);
static EncodedJSValue JSC_HOST_CALL dateProtoFuncToLocaleTimeString(ExecState*);
static EncodedJSValue JSC_HOST_CALL dateProtoFuncToString(ExecState*);
static EncodedJSValue JSC_HOST_CALL dateProtoFuncToTimeString(ExecState*);
static EncodedJSValue JSC_HOST_CALL dateProtoFuncToUTCString(ExecState*);
static EncodedJSValue JSC_HOST_CALL dateProtoFuncToISOString(ExecState*);
static EncodedJSValue JSC_HOST_CALL dateProtoFuncToJSON(ExecState*);

}

#include "DatePrototype.lut.h"

namespace JSC {

enum LocaleDateTimeFormat { LocaleDateAndTime, LocaleDate, LocaleTime };
 
#if OS(DARWIN) && USE(CF)

// FIXME: Since this is superior to the strftime-based version, why limit this to OS(DARWIN)?
// Instead we should consider using this whenever USE(CF) is true.

static CFDateFormatterStyle styleFromArgString(const String& string, CFDateFormatterStyle defaultStyle)
{
    if (string == "short")
        return kCFDateFormatterShortStyle;
    if (string == "medium")
        return kCFDateFormatterMediumStyle;
    if (string == "long")
        return kCFDateFormatterLongStyle;
    if (string == "full")
        return kCFDateFormatterFullStyle;
    return defaultStyle;
}

static JSCell* formatLocaleDate(ExecState* exec, DateInstance*, double timeInMilliseconds, LocaleDateTimeFormat format)
{
    CFDateFormatterStyle dateStyle = (format != LocaleTime ? kCFDateFormatterLongStyle : kCFDateFormatterNoStyle);
    CFDateFormatterStyle timeStyle = (format != LocaleDate ? kCFDateFormatterLongStyle : kCFDateFormatterNoStyle);

    bool useCustomFormat = false;
    String customFormatString;

    String arg0String = exec->argument(0).toString(exec)->value(exec);
    if (arg0String == "custom" && !exec->argument(1).isUndefined()) {
        useCustomFormat = true;
        customFormatString = exec->argument(1).toString(exec)->value(exec);
    } else if (format == LocaleDateAndTime && !exec->argument(1).isUndefined()) {
        dateStyle = styleFromArgString(arg0String, dateStyle);
        timeStyle = styleFromArgString(exec->argument(1).toString(exec)->value(exec), timeStyle);
    } else if (format != LocaleTime && !exec->argument(0).isUndefined())
        dateStyle = styleFromArgString(arg0String, dateStyle);
    else if (format != LocaleDate && !exec->argument(0).isUndefined())
        timeStyle = styleFromArgString(arg0String, timeStyle);

    CFLocaleRef locale = CFLocaleCopyCurrent();
    CFDateFormatterRef formatter = CFDateFormatterCreate(0, locale, dateStyle, timeStyle);
    CFRelease(locale);

    if (useCustomFormat) {
        CFStringRef customFormatCFString = CFStringCreateWithCharacters(0, customFormatString.characters(), customFormatString.length());
        CFDateFormatterSetFormat(formatter, customFormatCFString);
        CFRelease(customFormatCFString);
    }

    CFStringRef string = CFDateFormatterCreateStringWithAbsoluteTime(0, formatter, floor(timeInMilliseconds / msPerSecond) - kCFAbsoluteTimeIntervalSince1970);

    CFRelease(formatter);

    // We truncate the string returned from CFDateFormatter if it's absurdly long (> 200 characters).
    // That's not great error handling, but it just won't happen so it doesn't matter.
    UChar buffer[200];
    const size_t bufferLength = WTF_ARRAY_LENGTH(buffer);
    size_t length = CFStringGetLength(string);
    ASSERT(length <= bufferLength);
    if (length > bufferLength)
        length = bufferLength;
    CFStringGetCharacters(string, CFRangeMake(0, length), buffer);

    CFRelease(string);

    return jsNontrivialString(exec, String(buffer, length));
}

#elif USE(ICU_UNICODE) && !UCONFIG_NO_FORMATTING

static JSCell* formatLocaleDate(ExecState* exec, DateInstance*, double timeInMilliseconds, LocaleDateTimeFormat format)
{
    UDateFormatStyle timeStyle = (format != LocaleDate ? UDAT_LONG : UDAT_NONE);
    UDateFormatStyle dateStyle = (format != LocaleTime ? UDAT_LONG : UDAT_NONE);

    UErrorCode status = U_ZERO_ERROR;
    UDateFormat* df = udat_open(timeStyle, dateStyle, 0, 0, -1, 0, 0, &status);
    if (!df)
        return jsEmptyString(exec);

    UChar buffer[128];
    int32_t length;
    length = udat_format(df, timeInMilliseconds, buffer, 128, 0, &status);
    udat_close(df);
    if (status != U_ZERO_ERROR)
        return jsEmptyString(exec);

    return jsNontrivialString(exec, String(buffer, length));
}

#else

static JSCell* formatLocaleDate(ExecState* exec, const GregorianDateTime& gdt, LocaleDateTimeFormat format)
{
#if OS(WINDOWS)
    SYSTEMTIME systemTime;
    memset(&systemTime, 0, sizeof(systemTime));
    systemTime.wYear = gdt.year();
    systemTime.wMonth = gdt.month() + 1;
    systemTime.wDay = gdt.monthDay();
    systemTime.wDayOfWeek = gdt.weekDay();
    systemTime.wHour = gdt.hour();
    systemTime.wMinute = gdt.minute();
    systemTime.wSecond = gdt.second();

    Vector<UChar, 128> buffer;
    size_t length = 0;

    if (format == LocaleDate) {
        buffer.resize(GetDateFormatW(LOCALE_USER_DEFAULT, DATE_LONGDATE, &systemTime, 0, 0, 0));
        length = GetDateFormatW(LOCALE_USER_DEFAULT, DATE_LONGDATE, &systemTime, 0, buffer.data(), buffer.size());
    } else if (format == LocaleTime) {
        buffer.resize(GetTimeFormatW(LOCALE_USER_DEFAULT, 0, &systemTime, 0, 0, 0));
        length = GetTimeFormatW(LOCALE_USER_DEFAULT, 0, &systemTime, 0, buffer.data(), buffer.size());
    } else if (format == LocaleDateAndTime) {
        buffer.resize(GetDateFormatW(LOCALE_USER_DEFAULT, DATE_LONGDATE, &systemTime, 0, 0, 0) + GetTimeFormatW(LOCALE_USER_DEFAULT, 0, &systemTime, 0, 0, 0));
        length = GetDateFormatW(LOCALE_USER_DEFAULT, DATE_LONGDATE, &systemTime, 0, buffer.data(), buffer.size());
        if (length) {
            buffer[length - 1] = ' ';
            length += GetTimeFormatW(LOCALE_USER_DEFAULT, 0, &systemTime, 0, buffer.data() + length, buffer.size() - length);
        }
    } else
        RELEASE_ASSERT_NOT_REACHED();

    //  Remove terminating null character.
    if (length)
        length--;

    return jsNontrivialString(exec, String(buffer.data(), length));

#else // OS(WINDOWS)

#if HAVE(LANGINFO_H)
    static const nl_item formats[] = { D_T_FMT, D_FMT, T_FMT };
#else
    static const char* const formatStrings[] = { "%#c", "%#x", "%X" };
#endif

    // Offset year if needed
    struct tm localTM = gdt;
    int year = gdt.year();
    bool yearNeedsOffset = year < 1900 || year > 2038;
    if (yearNeedsOffset)
        localTM.tm_year = equivalentYearForDST(year) - 1900;

#if HAVE(LANGINFO_H)
    // We do not allow strftime to generate dates with 2-digits years,
    // both to avoid ambiguity, and a crash in strncpy, for years that
    // need offset.
    char* formatString = strdup(nl_langinfo(formats[format]));
    char* yPos = strchr(formatString, 'y');
    if (yPos)
        *yPos = 'Y';
#endif

    // Do the formatting
    const int bufsize = 128;
    char timebuffer[bufsize];

#if HAVE(LANGINFO_H)
    size_t ret = strftime(timebuffer, bufsize, formatString, &localTM);
    free(formatString);
#else
    size_t ret = strftime(timebuffer, bufsize, formatStrings[format], &localTM);
#endif

    if (ret == 0)
        return jsEmptyString(exec);

    // Copy original into the buffer
    if (yearNeedsOffset && format != LocaleTime) {
        static const int yearLen = 5;   // FIXME will be a problem in the year 10,000
        char yearString[yearLen];

        snprintf(yearString, yearLen, "%d", localTM.tm_year + 1900);
        char* yearLocation = strstr(timebuffer, yearString);
        snprintf(yearString, yearLen, "%d", year);

        strncpy(yearLocation, yearString, yearLen - 1);
    }

    // Convert multi-byte result to UNICODE.
    // If __STDC_ISO_10646__ is defined, wide character represents
    // UTF-16 (or UTF-32) code point. In most modern Unix like system
    // (e.g. Linux with glibc 2.2 and above) the macro is defined,
    // and wide character represents UTF-32 code point.
    // Here we static_cast potential UTF-32 to UTF-16, it should be
    // safe because date and (or) time related characters in different languages
    // should be in UNICODE BMP. If mbstowcs fails, we just fall
    // back on using multi-byte result as-is.
#ifdef __STDC_ISO_10646__
    UChar buffer[bufsize];
    wchar_t tempbuffer[bufsize];
    size_t length = mbstowcs(tempbuffer, timebuffer, bufsize - 1);
    if (length != static_cast<size_t>(-1)) {
        for (size_t i = 0; i < length; ++i)
            buffer[i] = static_cast<UChar>(tempbuffer[i]);
        return jsNontrivialString(exec, String(buffer, length));
    }
#endif

    return jsNontrivialString(exec, timebuffer);
#endif // OS(WINDOWS)
}

static JSCell* formatLocaleDate(ExecState* exec, DateInstance* dateObject, double, LocaleDateTimeFormat format)
{
    const GregorianDateTime* gregorianDateTime = dateObject->gregorianDateTime(exec);
    if (!gregorianDateTime)
        return jsNontrivialString(exec, ASCIILiteral("Invalid Date"));
    return formatLocaleDate(exec, *gregorianDateTime, format);
}

#endif // OS(DARWIN) && USE(CF)

static EncodedJSValue formateDateInstance(ExecState* exec, DateTimeFormat format, bool asUTCVariant)
{
    JSValue thisValue = exec->hostThisValue();
    if (!thisValue.inherits(&DateInstance::s_info))
        return throwVMTypeError(exec);

    DateInstance* thisDateObj = asDateInstance(thisValue);

    const GregorianDateTime* gregorianDateTime = asUTCVariant
        ? thisDateObj->gregorianDateTimeUTC(exec)
        : thisDateObj->gregorianDateTime(exec);
    if (!gregorianDateTime)
        return JSValue::encode(jsNontrivialString(exec, String(ASCIILiteral("Invalid Date"))));

    return JSValue::encode(jsNontrivialString(exec, formatDateTime(*gregorianDateTime, format, asUTCVariant)));
}

// Converts a list of arguments sent to a Date member function into milliseconds, updating
// ms (representing milliseconds) and t (representing the rest of the date structure) appropriately.
//
// Format of member function: f([hour,] [min,] [sec,] [ms])
static bool fillStructuresUsingTimeArgs(ExecState* exec, int maxArgs, double* ms, GregorianDateTime* t)
{
    double milliseconds = 0;
    bool ok = true;
    int idx = 0;
    int numArgs = exec->argumentCount();
    
    // JS allows extra trailing arguments -- ignore them
    if (numArgs > maxArgs)
        numArgs = maxArgs;

    // hours
    if (maxArgs >= 4 && idx < numArgs) {
        t->setHour(0);
        double hours = exec->argument(idx++).toIntegerPreserveNaN(exec);
        ok = std::isfinite(hours);
        milliseconds += hours * msPerHour;
    }

    // minutes
    if (maxArgs >= 3 && idx < numArgs && ok) {
        t->setMinute(0);
        double minutes = exec->argument(idx++).toIntegerPreserveNaN(exec);
        ok = std::isfinite(minutes);
        milliseconds += minutes * msPerMinute;
    }
    
    // seconds
    if (maxArgs >= 2 && idx < numArgs && ok) {
        t->setSecond(0);
        double seconds = exec->argument(idx++).toIntegerPreserveNaN(exec);
        ok = std::isfinite(seconds);
        milliseconds += seconds * msPerSecond;
    }
    
    if (!ok)
        return false;
        
    // milliseconds
    if (idx < numArgs) {
        double millis = exec->argument(idx).toIntegerPreserveNaN(exec);
        ok = std::isfinite(millis);
        milliseconds += millis;
    } else
        milliseconds += *ms;
    
    *ms = milliseconds;
    return ok;
}

// Converts a list of arguments sent to a Date member function into years, months, and milliseconds, updating
// ms (representing milliseconds) and t (representing the rest of the date structure) appropriately.
//
// Format of member function: f([years,] [months,] [days])
static bool fillStructuresUsingDateArgs(ExecState *exec, int maxArgs, double *ms, GregorianDateTime *t)
{
    int idx = 0;
    bool ok = true;
    int numArgs = exec->argumentCount();
  
    // JS allows extra trailing arguments -- ignore them
    if (numArgs > maxArgs)
        numArgs = maxArgs;
  
    // years
    if (maxArgs >= 3 && idx < numArgs) {
        double years = exec->argument(idx++).toIntegerPreserveNaN(exec);
        ok = std::isfinite(years);
        t->setYear(toInt32(years));
    }
    // months
    if (maxArgs >= 2 && idx < numArgs && ok) {
        double months = exec->argument(idx++).toIntegerPreserveNaN(exec);
        ok = std::isfinite(months);
        t->setMonth(toInt32(months));
    }
    // days
    if (idx < numArgs && ok) {
        double days = exec->argument(idx++).toIntegerPreserveNaN(exec);
        ok = std::isfinite(days);
        t->setMonthDay(0);
        *ms += days * msPerDay;
    }
    
    return ok;
}

const ClassInfo DatePrototype::s_info = {"Date", &DateInstance::s_info, 0, ExecState::dateTable, CREATE_METHOD_TABLE(DatePrototype)};

/* Source for DatePrototype.lut.h
@begin dateTable
  toString              dateProtoFuncToString                DontEnum|Function       0
  toISOString           dateProtoFuncToISOString             DontEnum|Function       0
  toUTCString           dateProtoFuncToUTCString             DontEnum|Function       0
  toDateString          dateProtoFuncToDateString            DontEnum|Function       0
  toTimeString          dateProtoFuncToTimeString            DontEnum|Function       0
  toLocaleString        dateProtoFuncToLocaleString          DontEnum|Function       0
  toLocaleDateString    dateProtoFuncToLocaleDateString      DontEnum|Function       0
  toLocaleTimeString    dateProtoFuncToLocaleTimeString      DontEnum|Function       0
  valueOf               dateProtoFuncGetTime                 DontEnum|Function       0
  getTime               dateProtoFuncGetTime                 DontEnum|Function       0
  getFullYear           dateProtoFuncGetFullYear             DontEnum|Function       0
  getUTCFullYear        dateProtoFuncGetUTCFullYear          DontEnum|Function       0
  toGMTString           dateProtoFuncToGMTString             DontEnum|Function       0
  getMonth              dateProtoFuncGetMonth                DontEnum|Function       0
  getUTCMonth           dateProtoFuncGetUTCMonth             DontEnum|Function       0
  getDate               dateProtoFuncGetDate                 DontEnum|Function       0
  getUTCDate            dateProtoFuncGetUTCDate              DontEnum|Function       0
  getDay                dateProtoFuncGetDay                  DontEnum|Function       0
  getUTCDay             dateProtoFuncGetUTCDay               DontEnum|Function       0
  getHours              dateProtoFuncGetHours                DontEnum|Function       0
  getUTCHours           dateProtoFuncGetUTCHours             DontEnum|Function       0
  getMinutes            dateProtoFuncGetMinutes              DontEnum|Function       0
  getUTCMinutes         dateProtoFuncGetUTCMinutes           DontEnum|Function       0
  getSeconds            dateProtoFuncGetSeconds              DontEnum|Function       0
  getUTCSeconds         dateProtoFuncGetUTCSeconds           DontEnum|Function       0
  getMilliseconds       dateProtoFuncGetMilliSeconds         DontEnum|Function       0
  getUTCMilliseconds    dateProtoFuncGetUTCMilliseconds      DontEnum|Function       0
  getTimezoneOffset     dateProtoFuncGetTimezoneOffset       DontEnum|Function       0
  setTime               dateProtoFuncSetTime                 DontEnum|Function       1
  setMilliseconds       dateProtoFuncSetMilliSeconds         DontEnum|Function       1
  setUTCMilliseconds    dateProtoFuncSetUTCMilliseconds      DontEnum|Function       1
  setSeconds            dateProtoFuncSetSeconds              DontEnum|Function       2
  setUTCSeconds         dateProtoFuncSetUTCSeconds           DontEnum|Function       2
  setMinutes            dateProtoFuncSetMinutes              DontEnum|Function       3
  setUTCMinutes         dateProtoFuncSetUTCMinutes           DontEnum|Function       3
  setHours              dateProtoFuncSetHours                DontEnum|Function       4
  setUTCHours           dateProtoFuncSetUTCHours             DontEnum|Function       4
  setDate               dateProtoFuncSetDate                 DontEnum|Function       1
  setUTCDate            dateProtoFuncSetUTCDate              DontEnum|Function       1
  setMonth              dateProtoFuncSetMonth                DontEnum|Function       2
  setUTCMonth           dateProtoFuncSetUTCMonth             DontEnum|Function       2
  setFullYear           dateProtoFuncSetFullYear             DontEnum|Function       3
  setUTCFullYear        dateProtoFuncSetUTCFullYear          DontEnum|Function       3
  setYear               dateProtoFuncSetYear                 DontEnum|Function       1
  getYear               dateProtoFuncGetYear                 DontEnum|Function       0
  toJSON                dateProtoFuncToJSON                  DontEnum|Function       1
@end
*/

// ECMA 15.9.4

DatePrototype::DatePrototype(ExecState* exec, Structure* structure)
    : DateInstance(exec, structure)
{
}

void DatePrototype::finishCreation(ExecState* exec, JSGlobalObject*)
{
    Base::finishCreation(exec->vm());
    ASSERT(inherits(&s_info));

    // The constructor will be added later, after DateConstructor has been built.
}

bool DatePrototype::getOwnPropertySlot(JSCell* cell, ExecState* exec, PropertyName propertyName, PropertySlot& slot)
{
    return getStaticFunctionSlot<JSObject>(exec, ExecState::dateTable(exec), jsCast<DatePrototype*>(cell), propertyName, slot);
}

bool DatePrototype::getOwnPropertyDescriptor(JSObject* object, ExecState* exec, PropertyName propertyName, PropertyDescriptor& descriptor)
{
    return getStaticFunctionDescriptor<JSObject>(exec, ExecState::dateTable(exec), jsCast<DatePrototype*>(object), propertyName, descriptor);
}

// Functions

EncodedJSValue JSC_HOST_CALL dateProtoFuncToString(ExecState* exec)
{
    const bool asUTCVariant = false;
    return formateDateInstance(exec, DateTimeFormatDateAndTime, asUTCVariant);
}

EncodedJSValue JSC_HOST_CALL dateProtoFuncToUTCString(ExecState* exec)
{
    const bool asUTCVariant = true;
    return formateDateInstance(exec, DateTimeFormatDateAndTime, asUTCVariant);
}

EncodedJSValue JSC_HOST_CALL dateProtoFuncToISOString(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    if (!thisValue.inherits(&DateInstance::s_info))
        return throwVMTypeError(exec);
    
    DateInstance* thisDateObj = asDateInstance(thisValue); 
    if (!std::isfinite(thisDateObj->internalNumber()))
        return throwVMError(exec, createRangeError(exec, ASCIILiteral("Invalid Date")));

    const GregorianDateTime* gregorianDateTime = thisDateObj->gregorianDateTimeUTC(exec);
    if (!gregorianDateTime)
        return JSValue::encode(jsNontrivialString(exec, String(ASCIILiteral("Invalid Date"))));
    // Maximum amount of space we need in buffer: 7 (max. digits in year) + 2 * 5 (2 characters each for month, day, hour, minute, second) + 4 (. + 3 digits for milliseconds)
    // 6 for formatting and one for null termination = 28. We add one extra character to allow us to force null termination.
    char buffer[28];
    // If the year is outside the bounds of 0 and 9999 inclusive we want to use the extended year format (ES 15.9.1.15.1).
    int ms = static_cast<int>(fmod(thisDateObj->internalNumber(), msPerSecond));
    if (ms < 0)
        ms += msPerSecond;

    int charactersWritten;
    if (gregorianDateTime->year() > 9999 || gregorianDateTime->year() < 0)
        charactersWritten = snprintf(buffer, sizeof(buffer), "%+07d-%02d-%02dT%02d:%02d:%02d.%03dZ", gregorianDateTime->year(), gregorianDateTime->month() + 1, gregorianDateTime->monthDay(), gregorianDateTime->hour(), gregorianDateTime->minute(), gregorianDateTime->second(), ms);
    else
        charactersWritten = snprintf(buffer, sizeof(buffer), "%04d-%02d-%02dT%02d:%02d:%02d.%03dZ", gregorianDateTime->year(), gregorianDateTime->month() + 1, gregorianDateTime->monthDay(), gregorianDateTime->hour(), gregorianDateTime->minute(), gregorianDateTime->second(), ms);

    ASSERT(charactersWritten > 0 && static_cast<unsigned>(charactersWritten) < sizeof(buffer));
    if (static_cast<unsigned>(charactersWritten) >= sizeof(buffer))
        return JSValue::encode(jsEmptyString(exec));

    return JSValue::encode(jsNontrivialString(exec, String(buffer, charactersWritten)));
}

EncodedJSValue JSC_HOST_CALL dateProtoFuncToDateString(ExecState* exec)
{
    const bool asUTCVariant = false;
    return formateDateInstance(exec, DateTimeFormatDate, asUTCVariant);
}

EncodedJSValue JSC_HOST_CALL dateProtoFuncToTimeString(ExecState* exec)
{
    const bool asUTCVariant = false;
    return formateDateInstance(exec, DateTimeFormatTime, asUTCVariant);
}

EncodedJSValue JSC_HOST_CALL dateProtoFuncToLocaleString(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    if (!thisValue.inherits(&DateInstance::s_info))
        return throwVMTypeError(exec);

    DateInstance* thisDateObj = asDateInstance(thisValue); 
    return JSValue::encode(formatLocaleDate(exec, thisDateObj, thisDateObj->internalNumber(), LocaleDateAndTime));
}

EncodedJSValue JSC_HOST_CALL dateProtoFuncToLocaleDateString(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    if (!thisValue.inherits(&DateInstance::s_info))
        return throwVMTypeError(exec);

    DateInstance* thisDateObj = asDateInstance(thisValue); 
    return JSValue::encode(formatLocaleDate(exec, thisDateObj, thisDateObj->internalNumber(), LocaleDate));
}

EncodedJSValue JSC_HOST_CALL dateProtoFuncToLocaleTimeString(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    if (!thisValue.inherits(&DateInstance::s_info))
        return throwVMTypeError(exec);

    DateInstance* thisDateObj = asDateInstance(thisValue); 
    return JSValue::encode(formatLocaleDate(exec, thisDateObj, thisDateObj->internalNumber(), LocaleTime));
}

EncodedJSValue JSC_HOST_CALL dateProtoFuncGetTime(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    if (!thisValue.inherits(&DateInstance::s_info))
        return throwVMTypeError(exec);

    return JSValue::encode(asDateInstance(thisValue)->internalValue());
}

EncodedJSValue JSC_HOST_CALL dateProtoFuncGetFullYear(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    if (!thisValue.inherits(&DateInstance::s_info))
        return throwVMTypeError(exec);

    DateInstance* thisDateObj = asDateInstance(thisValue); 

    const GregorianDateTime* gregorianDateTime = thisDateObj->gregorianDateTime(exec);
    if (!gregorianDateTime)
        return JSValue::encode(jsNaN());
    return JSValue::encode(jsNumber(gregorianDateTime->year()));
}

EncodedJSValue JSC_HOST_CALL dateProtoFuncGetUTCFullYear(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    if (!thisValue.inherits(&DateInstance::s_info))
        return throwVMTypeError(exec);

    DateInstance* thisDateObj = asDateInstance(thisValue); 

    const GregorianDateTime* gregorianDateTime = thisDateObj->gregorianDateTimeUTC(exec);
    if (!gregorianDateTime)
        return JSValue::encode(jsNaN());
    return JSValue::encode(jsNumber(gregorianDateTime->year()));
}

EncodedJSValue JSC_HOST_CALL dateProtoFuncToGMTString(ExecState* exec)
{
    const bool asUTCVariant = true;
    return formateDateInstance(exec, DateTimeFormatDateAndTime, asUTCVariant);
}

EncodedJSValue JSC_HOST_CALL dateProtoFuncGetMonth(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    if (!thisValue.inherits(&DateInstance::s_info))
        return throwVMTypeError(exec);

    DateInstance* thisDateObj = asDateInstance(thisValue); 

    const GregorianDateTime* gregorianDateTime = thisDateObj->gregorianDateTime(exec);
    if (!gregorianDateTime)
        return JSValue::encode(jsNaN());
    return JSValue::encode(jsNumber(gregorianDateTime->month()));
}

EncodedJSValue JSC_HOST_CALL dateProtoFuncGetUTCMonth(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    if (!thisValue.inherits(&DateInstance::s_info))
        return throwVMTypeError(exec);

    DateInstance* thisDateObj = asDateInstance(thisValue); 

    const GregorianDateTime* gregorianDateTime = thisDateObj->gregorianDateTimeUTC(exec);
    if (!gregorianDateTime)
        return JSValue::encode(jsNaN());
    return JSValue::encode(jsNumber(gregorianDateTime->month()));
}

EncodedJSValue JSC_HOST_CALL dateProtoFuncGetDate(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    if (!thisValue.inherits(&DateInstance::s_info))
        return throwVMTypeError(exec);

    DateInstance* thisDateObj = asDateInstance(thisValue); 

    const GregorianDateTime* gregorianDateTime = thisDateObj->gregorianDateTime(exec);
    if (!gregorianDateTime)
        return JSValue::encode(jsNaN());
    return JSValue::encode(jsNumber(gregorianDateTime->monthDay()));
}

EncodedJSValue JSC_HOST_CALL dateProtoFuncGetUTCDate(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    if (!thisValue.inherits(&DateInstance::s_info))
        return throwVMTypeError(exec);

    DateInstance* thisDateObj = asDateInstance(thisValue); 

    const GregorianDateTime* gregorianDateTime = thisDateObj->gregorianDateTimeUTC(exec);
    if (!gregorianDateTime)
        return JSValue::encode(jsNaN());
    return JSValue::encode(jsNumber(gregorianDateTime->monthDay()));
}

EncodedJSValue JSC_HOST_CALL dateProtoFuncGetDay(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    if (!thisValue.inherits(&DateInstance::s_info))
        return throwVMTypeError(exec);

    DateInstance* thisDateObj = asDateInstance(thisValue); 

    const GregorianDateTime* gregorianDateTime = thisDateObj->gregorianDateTime(exec);
    if (!gregorianDateTime)
        return JSValue::encode(jsNaN());
    return JSValue::encode(jsNumber(gregorianDateTime->weekDay()));
}

EncodedJSValue JSC_HOST_CALL dateProtoFuncGetUTCDay(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    if (!thisValue.inherits(&DateInstance::s_info))
        return throwVMTypeError(exec);

    DateInstance* thisDateObj = asDateInstance(thisValue); 

    const GregorianDateTime* gregorianDateTime = thisDateObj->gregorianDateTimeUTC(exec);
    if (!gregorianDateTime)
        return JSValue::encode(jsNaN());
    return JSValue::encode(jsNumber(gregorianDateTime->weekDay()));
}

EncodedJSValue JSC_HOST_CALL dateProtoFuncGetHours(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    if (!thisValue.inherits(&DateInstance::s_info))
        return throwVMTypeError(exec);

    DateInstance* thisDateObj = asDateInstance(thisValue); 

    const GregorianDateTime* gregorianDateTime = thisDateObj->gregorianDateTime(exec);
    if (!gregorianDateTime)
        return JSValue::encode(jsNaN());
    return JSValue::encode(jsNumber(gregorianDateTime->hour()));
}

EncodedJSValue JSC_HOST_CALL dateProtoFuncGetUTCHours(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    if (!thisValue.inherits(&DateInstance::s_info))
        return throwVMTypeError(exec);

    DateInstance* thisDateObj = asDateInstance(thisValue); 

    const GregorianDateTime* gregorianDateTime = thisDateObj->gregorianDateTimeUTC(exec);
    if (!gregorianDateTime)
        return JSValue::encode(jsNaN());
    return JSValue::encode(jsNumber(gregorianDateTime->hour()));
}

EncodedJSValue JSC_HOST_CALL dateProtoFuncGetMinutes(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    if (!thisValue.inherits(&DateInstance::s_info))
        return throwVMTypeError(exec);

    DateInstance* thisDateObj = asDateInstance(thisValue); 

    const GregorianDateTime* gregorianDateTime = thisDateObj->gregorianDateTime(exec);
    if (!gregorianDateTime)
        return JSValue::encode(jsNaN());
    return JSValue::encode(jsNumber(gregorianDateTime->minute()));
}

EncodedJSValue JSC_HOST_CALL dateProtoFuncGetUTCMinutes(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    if (!thisValue.inherits(&DateInstance::s_info))
        return throwVMTypeError(exec);

    DateInstance* thisDateObj = asDateInstance(thisValue); 

    const GregorianDateTime* gregorianDateTime = thisDateObj->gregorianDateTimeUTC(exec);
    if (!gregorianDateTime)
        return JSValue::encode(jsNaN());
    return JSValue::encode(jsNumber(gregorianDateTime->minute()));
}

EncodedJSValue JSC_HOST_CALL dateProtoFuncGetSeconds(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    if (!thisValue.inherits(&DateInstance::s_info))
        return throwVMTypeError(exec);

    DateInstance* thisDateObj = asDateInstance(thisValue); 

    const GregorianDateTime* gregorianDateTime = thisDateObj->gregorianDateTime(exec);
    if (!gregorianDateTime)
        return JSValue::encode(jsNaN());
    return JSValue::encode(jsNumber(gregorianDateTime->second()));
}

EncodedJSValue JSC_HOST_CALL dateProtoFuncGetUTCSeconds(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    if (!thisValue.inherits(&DateInstance::s_info))
        return throwVMTypeError(exec);

    DateInstance* thisDateObj = asDateInstance(thisValue); 

    const GregorianDateTime* gregorianDateTime = thisDateObj->gregorianDateTimeUTC(exec);
    if (!gregorianDateTime)
        return JSValue::encode(jsNaN());
    return JSValue::encode(jsNumber(gregorianDateTime->second()));
}

EncodedJSValue JSC_HOST_CALL dateProtoFuncGetMilliSeconds(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    if (!thisValue.inherits(&DateInstance::s_info))
        return throwVMTypeError(exec);

    DateInstance* thisDateObj = asDateInstance(thisValue); 
    double milli = thisDateObj->internalNumber();
    if (std::isnan(milli))
        return JSValue::encode(jsNaN());

    double secs = floor(milli / msPerSecond);
    double ms = milli - secs * msPerSecond;
    return JSValue::encode(jsNumber(ms));
}

EncodedJSValue JSC_HOST_CALL dateProtoFuncGetUTCMilliseconds(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    if (!thisValue.inherits(&DateInstance::s_info))
        return throwVMTypeError(exec);

    DateInstance* thisDateObj = asDateInstance(thisValue); 
    double milli = thisDateObj->internalNumber();
    if (std::isnan(milli))
        return JSValue::encode(jsNaN());

    double secs = floor(milli / msPerSecond);
    double ms = milli - secs * msPerSecond;
    return JSValue::encode(jsNumber(ms));
}

EncodedJSValue JSC_HOST_CALL dateProtoFuncGetTimezoneOffset(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    if (!thisValue.inherits(&DateInstance::s_info))
        return throwVMTypeError(exec);

    DateInstance* thisDateObj = asDateInstance(thisValue); 

    const GregorianDateTime* gregorianDateTime = thisDateObj->gregorianDateTime(exec);
    if (!gregorianDateTime)
        return JSValue::encode(jsNaN());
    return JSValue::encode(jsNumber(-gregorianDateTime->utcOffset() / minutesPerHour));
}

EncodedJSValue JSC_HOST_CALL dateProtoFuncSetTime(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    if (!thisValue.inherits(&DateInstance::s_info))
        return throwVMTypeError(exec);

    DateInstance* thisDateObj = asDateInstance(thisValue); 

    double milli = timeClip(exec->argument(0).toNumber(exec));
    JSValue result = jsNumber(milli);
    thisDateObj->setInternalValue(exec->vm(), result);
    return JSValue::encode(result);
}

static EncodedJSValue setNewValueFromTimeArgs(ExecState* exec, int numArgsToUse, bool inputIsUTC)
{
    JSValue thisValue = exec->hostThisValue();
    if (!thisValue.inherits(&DateInstance::s_info))
        return throwVMTypeError(exec);

    DateInstance* thisDateObj = asDateInstance(thisValue);
    double milli = thisDateObj->internalNumber();
    
    if (!exec->argumentCount() || std::isnan(milli)) {
        JSValue result = jsNaN();
        thisDateObj->setInternalValue(exec->vm(), result);
        return JSValue::encode(result);
    }
     
    double secs = floor(milli / msPerSecond);
    double ms = milli - secs * msPerSecond;

    const GregorianDateTime* other = inputIsUTC 
        ? thisDateObj->gregorianDateTimeUTC(exec)
        : thisDateObj->gregorianDateTime(exec);
    if (!other)
        return JSValue::encode(jsNaN());

    GregorianDateTime gregorianDateTime;
    gregorianDateTime.copyFrom(*other);
    if (!fillStructuresUsingTimeArgs(exec, numArgsToUse, &ms, &gregorianDateTime)) {
        JSValue result = jsNaN();
        thisDateObj->setInternalValue(exec->vm(), result);
        return JSValue::encode(result);
    } 
    
    JSValue result = jsNumber(gregorianDateTimeToMS(exec, gregorianDateTime, ms, inputIsUTC));
    thisDateObj->setInternalValue(exec->vm(), result);
    return JSValue::encode(result);
}

static EncodedJSValue setNewValueFromDateArgs(ExecState* exec, int numArgsToUse, bool inputIsUTC)
{
    JSValue thisValue = exec->hostThisValue();
    if (!thisValue.inherits(&DateInstance::s_info))
        return throwVMTypeError(exec);

    DateInstance* thisDateObj = asDateInstance(thisValue);
    if (!exec->argumentCount()) {
        JSValue result = jsNaN();
        thisDateObj->setInternalValue(exec->vm(), result);
        return JSValue::encode(result);
    }      
    
    double milli = thisDateObj->internalNumber();
    double ms = 0; 

    GregorianDateTime gregorianDateTime; 
    if (numArgsToUse == 3 && std::isnan(milli)) 
        msToGregorianDateTime(exec, 0, true, gregorianDateTime); 
    else { 
        ms = milli - floor(milli / msPerSecond) * msPerSecond; 
        const GregorianDateTime* other = inputIsUTC 
            ? thisDateObj->gregorianDateTimeUTC(exec)
            : thisDateObj->gregorianDateTime(exec);
        if (!other)
            return JSValue::encode(jsNaN());
        gregorianDateTime.copyFrom(*other);
    }
    
    if (!fillStructuresUsingDateArgs(exec, numArgsToUse, &ms, &gregorianDateTime)) {
        JSValue result = jsNaN();
        thisDateObj->setInternalValue(exec->vm(), result);
        return JSValue::encode(result);
    } 
           
    JSValue result = jsNumber(gregorianDateTimeToMS(exec, gregorianDateTime, ms, inputIsUTC));
    thisDateObj->setInternalValue(exec->vm(), result);
    return JSValue::encode(result);
}

EncodedJSValue JSC_HOST_CALL dateProtoFuncSetMilliSeconds(ExecState* exec)
{
    const bool inputIsUTC = false;
    return setNewValueFromTimeArgs(exec, 1, inputIsUTC);
}

EncodedJSValue JSC_HOST_CALL dateProtoFuncSetUTCMilliseconds(ExecState* exec)
{
    const bool inputIsUTC = true;
    return setNewValueFromTimeArgs(exec, 1, inputIsUTC);
}

EncodedJSValue JSC_HOST_CALL dateProtoFuncSetSeconds(ExecState* exec)
{
    const bool inputIsUTC = false;
    return setNewValueFromTimeArgs(exec, 2, inputIsUTC);
}

EncodedJSValue JSC_HOST_CALL dateProtoFuncSetUTCSeconds(ExecState* exec)
{
    const bool inputIsUTC = true;
    return setNewValueFromTimeArgs(exec, 2, inputIsUTC);
}

EncodedJSValue JSC_HOST_CALL dateProtoFuncSetMinutes(ExecState* exec)
{
    const bool inputIsUTC = false;
    return setNewValueFromTimeArgs(exec, 3, inputIsUTC);
}

EncodedJSValue JSC_HOST_CALL dateProtoFuncSetUTCMinutes(ExecState* exec)
{
    const bool inputIsUTC = true;
    return setNewValueFromTimeArgs(exec, 3, inputIsUTC);
}

EncodedJSValue JSC_HOST_CALL dateProtoFuncSetHours(ExecState* exec)
{
    const bool inputIsUTC = false;
    return setNewValueFromTimeArgs(exec, 4, inputIsUTC);
}

EncodedJSValue JSC_HOST_CALL dateProtoFuncSetUTCHours(ExecState* exec)
{
    const bool inputIsUTC = true;
    return setNewValueFromTimeArgs(exec, 4, inputIsUTC);
}

EncodedJSValue JSC_HOST_CALL dateProtoFuncSetDate(ExecState* exec)
{
    const bool inputIsUTC = false;
    return setNewValueFromDateArgs(exec, 1, inputIsUTC);
}

EncodedJSValue JSC_HOST_CALL dateProtoFuncSetUTCDate(ExecState* exec)
{
    const bool inputIsUTC = true;
    return setNewValueFromDateArgs(exec, 1, inputIsUTC);
}

EncodedJSValue JSC_HOST_CALL dateProtoFuncSetMonth(ExecState* exec)
{
    const bool inputIsUTC = false;
    return setNewValueFromDateArgs(exec, 2, inputIsUTC);
}

EncodedJSValue JSC_HOST_CALL dateProtoFuncSetUTCMonth(ExecState* exec)
{
    const bool inputIsUTC = true;
    return setNewValueFromDateArgs(exec, 2, inputIsUTC);
}

EncodedJSValue JSC_HOST_CALL dateProtoFuncSetFullYear(ExecState* exec)
{
    const bool inputIsUTC = false;
    return setNewValueFromDateArgs(exec, 3, inputIsUTC);
}

EncodedJSValue JSC_HOST_CALL dateProtoFuncSetUTCFullYear(ExecState* exec)
{
    const bool inputIsUTC = true;
    return setNewValueFromDateArgs(exec, 3, inputIsUTC);
}

EncodedJSValue JSC_HOST_CALL dateProtoFuncSetYear(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    if (!thisValue.inherits(&DateInstance::s_info))
        return throwVMTypeError(exec);

    DateInstance* thisDateObj = asDateInstance(thisValue);
    if (!exec->argumentCount()) { 
        JSValue result = jsNaN();
        thisDateObj->setInternalValue(exec->vm(), result);
        return JSValue::encode(result);
    }

    double milli = thisDateObj->internalNumber();
    double ms = 0;

    GregorianDateTime gregorianDateTime;
    if (std::isnan(milli))
        // Based on ECMA 262 B.2.5 (setYear)
        // the time must be reset to +0 if it is NaN.
        msToGregorianDateTime(exec, 0, true, gregorianDateTime);
    else {
        double secs = floor(milli / msPerSecond);
        ms = milli - secs * msPerSecond;
        if (const GregorianDateTime* other = thisDateObj->gregorianDateTime(exec))
            gregorianDateTime.copyFrom(*other);
    }

    double year = exec->argument(0).toIntegerPreserveNaN(exec);
    if (!std::isfinite(year)) {
        JSValue result = jsNaN();
        thisDateObj->setInternalValue(exec->vm(), result);
        return JSValue::encode(result);
    }

    gregorianDateTime.setYear(toInt32((year >= 0 && year <= 99) ? (year + 1900) : year));
    JSValue result = jsNumber(gregorianDateTimeToMS(exec, gregorianDateTime, ms, false));
    thisDateObj->setInternalValue(exec->vm(), result);
    return JSValue::encode(result);
}

EncodedJSValue JSC_HOST_CALL dateProtoFuncGetYear(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    if (!thisValue.inherits(&DateInstance::s_info))
        return throwVMTypeError(exec);

    DateInstance* thisDateObj = asDateInstance(thisValue); 

    const GregorianDateTime* gregorianDateTime = thisDateObj->gregorianDateTime(exec);
    if (!gregorianDateTime)
        return JSValue::encode(jsNaN());

    // NOTE: IE returns the full year even in getYear.
    return JSValue::encode(jsNumber(gregorianDateTime->year() - 1900));
}

EncodedJSValue JSC_HOST_CALL dateProtoFuncToJSON(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    JSObject* object = thisValue.toThisObject(exec);
    if (exec->hadException())
        return JSValue::encode(jsNull());
    
    JSValue toISOValue = object->get(exec, exec->vm().propertyNames->toISOString);
    if (exec->hadException())
        return JSValue::encode(jsNull());

    CallData callData;
    CallType callType = getCallData(toISOValue, callData);
    if (callType == CallTypeNone)
        return throwVMError(exec, createTypeError(exec, ASCIILiteral("toISOString is not a function")));

    JSValue result = call(exec, asObject(toISOValue), callType, callData, object, exec->emptyList());
    if (exec->hadException())
        return JSValue::encode(jsNull());
    if (result.isObject())
        return throwVMError(exec, createTypeError(exec, ASCIILiteral("toISOString did not return a primitive value")));
    return JSValue::encode(result);
}

} // namespace JSC
