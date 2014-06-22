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
#include "DateConversion.h"

#include <wtf/Assertions.h>
#include <wtf/DateMath.h>
#include <wtf/text/StringBuilder.h>
#include <wtf/text/WTFString.h>

#if OS(WINDOWS)
#include <windows.h>
#endif

using namespace WTF;

namespace JSC {

template<int width>
static inline void appendNumber(StringBuilder& builder, int value)
{
    int fillingZerosCount = width;
    if (value < 0) {
        builder.append('-');
        value = -value;
        --fillingZerosCount;
    }
    String valueString = String::number(value);
    fillingZerosCount -= valueString.length();
    for (int i = 0; i < fillingZerosCount; ++i)
        builder.append('0');
    builder.append(valueString);
}

template<>
void appendNumber<2>(StringBuilder& builder, int value)
{
    ASSERT(0 <= value && value <= 99);
    builder.append(static_cast<char>('0' + value / 10));
    builder.append(static_cast<char>('0' + value % 10));
}

String formatDateTime(const GregorianDateTime& t, DateTimeFormat format, bool asUTCVariant)
{
    bool appendDate = format & DateTimeFormatDate;
    bool appendTime = format & DateTimeFormatTime;

    StringBuilder builder;

    if (appendDate) {
        builder.append(weekdayName[(t.weekDay() + 6) % 7]);

        if (asUTCVariant) {
            builder.appendLiteral(", ");
            appendNumber<2>(builder, t.monthDay());
            builder.append(' ');
            builder.append(monthName[t.month()]);
        } else {
            builder.append(' ');
            builder.append(monthName[t.month()]);
            builder.append(' ');
            appendNumber<2>(builder, t.monthDay());
        }
        builder.append(' ');
        appendNumber<4>(builder, t.year());
    }

    if (appendDate && appendTime)
        builder.append(' ');

    if (appendTime) {
        appendNumber<2>(builder, t.hour());
        builder.append(':');
        appendNumber<2>(builder, t.minute());
        builder.append(':');
        appendNumber<2>(builder, t.second());
        builder.appendLiteral(" GMT");

        if (!asUTCVariant) {
            int offset = abs(t.utcOffset()) / 60;
            builder.append(t.utcOffset() < 0 ? '-' : '+');
            appendNumber<2>(builder, offset / 60);
            appendNumber<2>(builder, offset % 60);

#if OS(WINDOWS)
            TIME_ZONE_INFORMATION timeZoneInformation;
            GetTimeZoneInformation(&timeZoneInformation);
            const WCHAR* timeZoneName = t.isDST() ? timeZoneInformation.DaylightName : timeZoneInformation.StandardName;
#else
            struct tm gtm = t;
            char timeZoneName[70];
            strftime(timeZoneName, sizeof(timeZoneName), "%Z", &gtm);
#endif
            if (timeZoneName[0]) {
                builder.appendLiteral(" (");
                builder.append(timeZoneName);
                builder.append(')');
            }
        }
    }

    return builder.toString().impl();
}

} // namespace JSC
