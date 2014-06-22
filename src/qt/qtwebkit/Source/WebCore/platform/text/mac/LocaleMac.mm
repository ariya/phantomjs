/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
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
#include "LocaleMac.h"

#import <Foundation/NSDateFormatter.h>
#import <Foundation/NSLocale.h>
#include "Language.h"
#include "LocalizedStrings.h"
#include <wtf/DateMath.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/RetainPtr.h>
#include <wtf/text/StringBuilder.h>

using namespace std;

namespace WebCore {

static inline String languageFromLocale(const String& locale)
{
    String normalizedLocale = locale;
    normalizedLocale.replace('-', '_');
    size_t separatorPosition = normalizedLocale.find('_');
    if (separatorPosition == notFound)
        return normalizedLocale;
    return normalizedLocale.left(separatorPosition);
}

static RetainPtr<NSLocale> determineLocale(const String& locale)
{
    RetainPtr<NSLocale> currentLocale = [NSLocale currentLocale];
    String currentLocaleLanguage = languageFromLocale(String([currentLocale.get() localeIdentifier]));
    String localeLanguage = languageFromLocale(locale);
    if (equalIgnoringCase(currentLocaleLanguage, localeLanguage))
        return currentLocale;
    // It seems initWithLocaleIdentifier accepts dash-separated locale identifier.
     return adoptNS([[NSLocale alloc] initWithLocaleIdentifier:locale]);
}

PassOwnPtr<Locale> Locale::create(const AtomicString& locale)
{
    return LocaleMac::create(determineLocale(locale.string()).get());
}

static RetainPtr<NSDateFormatter> createDateTimeFormatter(NSLocale* locale, NSCalendar* calendar, NSDateFormatterStyle dateStyle, NSDateFormatterStyle timeStyle)
{
    NSDateFormatter *formatter = [[NSDateFormatter alloc] init];
    [formatter setLocale:locale];
    [formatter setDateStyle:dateStyle];
    [formatter setTimeStyle:timeStyle];
    [formatter setTimeZone:[NSTimeZone timeZoneWithAbbreviation:@"UTC"]];
    [formatter setCalendar:calendar];
    return adoptNS(formatter);
}

LocaleMac::LocaleMac(NSLocale* locale)
    : m_locale(locale)
    , m_gregorianCalendar(adoptNS([[NSCalendar alloc] initWithCalendarIdentifier:NSGregorianCalendar]))
    , m_didInitializeNumberData(false)
{
    NSArray* availableLanguages = [NSLocale ISOLanguageCodes];
    // NSLocale returns a lower case NSLocaleLanguageCode so we don't have care about case.
    NSString* language = [m_locale.get() objectForKey:NSLocaleLanguageCode];
    if ([availableLanguages indexOfObject:language] == NSNotFound)
        m_locale = adoptNS([[NSLocale alloc] initWithLocaleIdentifier:defaultLanguage()]);
    [m_gregorianCalendar.get() setLocale:m_locale.get()];
}

LocaleMac::~LocaleMac()
{
}

PassOwnPtr<LocaleMac> LocaleMac::create(const String& localeIdentifier)
{
    RetainPtr<NSLocale> locale = [[NSLocale alloc] initWithLocaleIdentifier:localeIdentifier];
    return adoptPtr(new LocaleMac(locale.get()));
}

PassOwnPtr<LocaleMac> LocaleMac::create(NSLocale* locale)
{
    return adoptPtr(new LocaleMac(locale));
}

RetainPtr<NSDateFormatter> LocaleMac::shortDateFormatter()
{
    return createDateTimeFormatter(m_locale.get(), m_gregorianCalendar.get(), NSDateFormatterShortStyle, NSDateFormatterNoStyle);
}

#if ENABLE(DATE_AND_TIME_INPUT_TYPES)
const Vector<String>& LocaleMac::monthLabels()
{
    if (!m_monthLabels.isEmpty())
        return m_monthLabels;
    m_monthLabels.reserveCapacity(12);
    NSArray *array = [shortDateFormatter().get() monthSymbols];
    if ([array count] == 12) {
        for (unsigned i = 0; i < 12; ++i)
            m_monthLabels.append(String([array objectAtIndex:i]));
        return m_monthLabels;
    }
    for (unsigned i = 0; i < WTF_ARRAY_LENGTH(WTF::monthFullName); ++i)
        m_monthLabels.append(WTF::monthFullName[i]);
    return m_monthLabels;
}

RetainPtr<NSDateFormatter> LocaleMac::timeFormatter()
{
    return createDateTimeFormatter(m_locale.get(), m_gregorianCalendar.get(), NSDateFormatterNoStyle, NSDateFormatterMediumStyle);
}

RetainPtr<NSDateFormatter> LocaleMac::shortTimeFormatter()
{
    return createDateTimeFormatter(m_locale.get(), m_gregorianCalendar.get(), NSDateFormatterNoStyle, NSDateFormatterShortStyle);
}

RetainPtr<NSDateFormatter> LocaleMac::dateTimeFormatterWithSeconds()
{
    return createDateTimeFormatter(m_locale.get(), m_gregorianCalendar.get(), NSDateFormatterShortStyle, NSDateFormatterMediumStyle);
}

RetainPtr<NSDateFormatter> LocaleMac::dateTimeFormatterWithoutSeconds()
{
    return createDateTimeFormatter(m_locale.get(), m_gregorianCalendar.get(), NSDateFormatterShortStyle, NSDateFormatterShortStyle);
}

String LocaleMac::dateFormat()
{
    if (!m_dateFormat.isNull())
        return m_dateFormat;
    m_dateFormat = [shortDateFormatter().get() dateFormat];
    return m_dateFormat;
}

String LocaleMac::monthFormat()
{
    if (!m_monthFormat.isNull())
        return m_monthFormat;
    // Gets a format for "MMMM" because Windows API always provides formats for
    // "MMMM" in some locales.
    m_monthFormat = [NSDateFormatter dateFormatFromTemplate:@"yyyyMMMM" options:0 locale:m_locale.get()];
    return m_monthFormat;
}

String LocaleMac::shortMonthFormat()
{
    if (!m_shortMonthFormat.isNull())
        return m_shortMonthFormat;
    m_shortMonthFormat = [NSDateFormatter dateFormatFromTemplate:@"yyyyMMM" options:0 locale:m_locale.get()];
    return m_shortMonthFormat;
}

String LocaleMac::timeFormat()
{
    if (!m_timeFormatWithSeconds.isNull())
        return m_timeFormatWithSeconds;
    m_timeFormatWithSeconds = [timeFormatter().get() dateFormat];
    return m_timeFormatWithSeconds;
}

String LocaleMac::shortTimeFormat()
{
    if (!m_timeFormatWithoutSeconds.isNull())
        return m_timeFormatWithoutSeconds;
    m_timeFormatWithoutSeconds = [shortTimeFormatter().get() dateFormat];
    return m_timeFormatWithoutSeconds;
}

String LocaleMac::dateTimeFormatWithSeconds()
{
    if (!m_dateTimeFormatWithSeconds.isNull())
        return m_dateTimeFormatWithSeconds;
    m_dateTimeFormatWithSeconds = [dateTimeFormatterWithSeconds().get() dateFormat];
    return m_dateTimeFormatWithSeconds;
}

String LocaleMac::dateTimeFormatWithoutSeconds()
{
    if (!m_dateTimeFormatWithoutSeconds.isNull())
        return m_dateTimeFormatWithoutSeconds;
    m_dateTimeFormatWithoutSeconds = [dateTimeFormatterWithoutSeconds().get() dateFormat];
    return m_dateTimeFormatWithoutSeconds;
}

const Vector<String>& LocaleMac::shortMonthLabels()
{
    if (!m_shortMonthLabels.isEmpty())
        return m_shortMonthLabels;
    m_shortMonthLabels.reserveCapacity(12);
    NSArray *array = [shortDateFormatter().get() shortMonthSymbols];
    if ([array count] == 12) {
        for (unsigned i = 0; i < 12; ++i)
            m_shortMonthLabels.append([array objectAtIndex:i]);
        return m_shortMonthLabels;
    }
    for (unsigned i = 0; i < WTF_ARRAY_LENGTH(WTF::monthName); ++i)
        m_shortMonthLabels.append(WTF::monthName[i]);
    return m_shortMonthLabels;
}

const Vector<String>& LocaleMac::standAloneMonthLabels()
{
    if (!m_standAloneMonthLabels.isEmpty())
        return m_standAloneMonthLabels;
    NSArray *array = [shortDateFormatter().get() standaloneMonthSymbols];
    if ([array count] == 12) {
        m_standAloneMonthLabels.reserveCapacity(12);
        for (unsigned i = 0; i < 12; ++i)
            m_standAloneMonthLabels.append([array objectAtIndex:i]);
        return m_standAloneMonthLabels;
    }
    m_standAloneMonthLabels = shortMonthLabels();
    return m_standAloneMonthLabels;
}

const Vector<String>& LocaleMac::shortStandAloneMonthLabels()
{
    if (!m_shortStandAloneMonthLabels.isEmpty())
        return m_shortStandAloneMonthLabels;
    NSArray *array = [shortDateFormatter().get() shortStandaloneMonthSymbols];
    if ([array count] == 12) {
        m_shortStandAloneMonthLabels.reserveCapacity(12);
        for (unsigned i = 0; i < 12; ++i)
            m_shortStandAloneMonthLabels.append([array objectAtIndex:i]);
        return m_shortStandAloneMonthLabels;
    }
    m_shortStandAloneMonthLabels = shortMonthLabels();
    return m_shortStandAloneMonthLabels;
}

const Vector<String>& LocaleMac::timeAMPMLabels()
{
    if (!m_timeAMPMLabels.isEmpty())
        return m_timeAMPMLabels;
    m_timeAMPMLabels.reserveCapacity(2);
    RetainPtr<NSDateFormatter> formatter = shortTimeFormatter();
    m_timeAMPMLabels.append([formatter.get() AMSymbol]);
    m_timeAMPMLabels.append([formatter.get() PMSymbol]);
    return m_timeAMPMLabels;
}
#endif

void LocaleMac::initializeLocaleData()
{
    if (m_didInitializeNumberData)
        return;
    m_didInitializeNumberData = true;

    RetainPtr<NSNumberFormatter> formatter = adoptNS([[NSNumberFormatter alloc] init]);
    [formatter.get() setLocale:m_locale.get()];
    [formatter.get() setNumberStyle:NSNumberFormatterDecimalStyle];
    [formatter.get() setUsesGroupingSeparator:NO];

    RetainPtr<NSNumber> sampleNumber = adoptNS([[NSNumber alloc] initWithDouble:9876543210]);
    String nineToZero([formatter.get() stringFromNumber:sampleNumber.get()]);
    if (nineToZero.length() != 10)
        return;
    Vector<String, DecimalSymbolsSize> symbols;
    for (unsigned i = 0; i < 10; ++i)
        symbols.append(nineToZero.substring(9 - i, 1));
    ASSERT(symbols.size() == DecimalSeparatorIndex);
    symbols.append([formatter.get() decimalSeparator]);
    ASSERT(symbols.size() == GroupSeparatorIndex);
    symbols.append([formatter.get() groupingSeparator]);
    ASSERT(symbols.size() == DecimalSymbolsSize);

    String positivePrefix([formatter.get() positivePrefix]);
    String positiveSuffix([formatter.get() positiveSuffix]);
    String negativePrefix([formatter.get() negativePrefix]);
    String negativeSuffix([formatter.get() negativeSuffix]);
    setLocaleData(symbols, positivePrefix, positiveSuffix, negativePrefix, negativeSuffix);
}

}
