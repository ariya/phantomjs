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
#include "LocaleWin.h"

#include "DateComponents.h"
#include "DateTimeFormat.h"
#include "Language.h"
#include "LocalizedStrings.h"
#include <limits>
#include <windows.h>
#include <wtf/CurrentTime.h>
#include <wtf/DateMath.h>
#include <wtf/HashMap.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/text/StringBuilder.h>
#include <wtf/text/StringHash.h>

using namespace std;

namespace WebCore {

typedef LCID (WINAPI* LocaleNameToLCIDPtr)(LPCWSTR, DWORD);
typedef HashMap<String, LCID> NameToLCIDMap;

static String extractLanguageCode(const String& locale)
{
    size_t dashPosition = locale.find('-');
    if (dashPosition == notFound)
        return locale;
    return locale.left(dashPosition);
}

static String removeLastComponent(const String& name)
{
    size_t lastSeparator = name.reverseFind('-');
    if (lastSeparator == notFound)
        return emptyString();
    return name.left(lastSeparator);
}

static void ensureNameToLCIDMap(NameToLCIDMap& map)
{
    if (!map.isEmpty())
        return;
    // http://www.microsoft.com/resources/msdn/goglobal/default.mspx
    // We add only locales used in layout tests for now.
    map.add("ar", 0x0001);
    map.add("ar-eg", 0x0C01);
    map.add("de", 0x0007);
    map.add("de-de", 0x0407);
    map.add("el", 0x0008);
    map.add("el-gr", 0x0408);
    map.add("en", 0x0009);
    map.add("en-gb", 0x0809);
    map.add("en-us", 0x0409);
    map.add("fr", 0x000C);
    map.add("fr-fr", 0x040C);
    map.add("he", 0x000D);
    map.add("he-il", 0x040D);
    map.add("hi", 0x0039);
    map.add("hi-in", 0x0439);
    map.add("ja", 0x0011);
    map.add("ja-jp", 0x0411);
    map.add("ko", 0x0012);
    map.add("ko-kr", 0x0412);
    map.add("ru", 0x0019);
    map.add("ru-ru", 0x0419);
    map.add("zh-cn", 0x0804);
    map.add("zh-tw", 0x0404);
}

// Fallback implementation of LocaleNameToLCID API. This is used for
// testing on Windows XP.
// FIXME: Remove this, ensureNameToLCIDMap, and removeLastComponent when we drop
// Windows XP support.
static LCID WINAPI convertLocaleNameToLCID(LPCWSTR name, DWORD)
{
    if (!name || !name[0])
        return LOCALE_USER_DEFAULT;
    DEFINE_STATIC_LOCAL(NameToLCIDMap, map, ());
    ensureNameToLCIDMap(map);
    String localeName = String(name).replace('_', '-');
    localeName.makeLower();
    do {
        NameToLCIDMap::const_iterator iterator = map.find(localeName);
        if (iterator != map.end())
            return iterator->value;
        localeName = removeLastComponent(localeName);
    } while (!localeName.isEmpty());
    return LOCALE_USER_DEFAULT;
}

static LCID LCIDFromLocaleInternal(LCID userDefaultLCID, const String& userDefaultLanguageCode, LocaleNameToLCIDPtr localeNameToLCID, String& locale)
{
    String localeLanguageCode = extractLanguageCode(locale);
    if (equalIgnoringCase(localeLanguageCode, userDefaultLanguageCode))
        return userDefaultLCID;
    return localeNameToLCID(locale.charactersWithNullTermination().data(), 0);
}

static LCID LCIDFromLocale(const AtomicString& locale)
{
    // LocaleNameToLCID() is available since Windows Vista.
    LocaleNameToLCIDPtr localeNameToLCID = reinterpret_cast<LocaleNameToLCIDPtr>(::GetProcAddress(::GetModuleHandle(L"kernel32"), "LocaleNameToLCID"));
    if (!localeNameToLCID)
        localeNameToLCID = convertLocaleNameToLCID;

    // According to MSDN, 9 is enough for LOCALE_SISO639LANGNAME.
    const size_t languageCodeBufferSize = 9;
    WCHAR lowercaseLanguageCode[languageCodeBufferSize];
    ::GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SISO639LANGNAME, lowercaseLanguageCode, languageCodeBufferSize);
    String userDefaultLanguageCode = String(lowercaseLanguageCode);

    LCID lcid = LCIDFromLocaleInternal(LOCALE_USER_DEFAULT, userDefaultLanguageCode, localeNameToLCID, String(locale));
    if (!lcid)
        lcid = LCIDFromLocaleInternal(LOCALE_USER_DEFAULT, userDefaultLanguageCode, localeNameToLCID, defaultLanguage());
    return lcid;
}

PassOwnPtr<Locale> Locale::create(const AtomicString& locale)
{
    return LocaleWin::create(LCIDFromLocale(locale));
}

inline LocaleWin::LocaleWin(LCID lcid)
    : m_lcid(lcid)
    , m_didInitializeNumberData(false)
{
}

PassOwnPtr<LocaleWin> LocaleWin::create(LCID lcid)
{
    return adoptPtr(new LocaleWin(lcid));
}

LocaleWin::~LocaleWin()
{
}

String LocaleWin::getLocaleInfoString(LCTYPE type)
{
    int bufferSizeWithNUL = ::GetLocaleInfo(m_lcid, type, 0, 0);
    if (bufferSizeWithNUL <= 0)
        return String();
    Vector<UChar> buffer(bufferSizeWithNUL);
    ::GetLocaleInfo(m_lcid, type, buffer.data(), bufferSizeWithNUL);
    buffer.shrink(bufferSizeWithNUL - 1);
    return String::adopt(buffer);
}

void LocaleWin::getLocaleInfo(LCTYPE type, DWORD& result)
{
    ::GetLocaleInfo(m_lcid, type | LOCALE_RETURN_NUMBER, reinterpret_cast<LPWSTR>(&result), sizeof(DWORD) / sizeof(TCHAR));
}

void LocaleWin::ensureShortMonthLabels()
{
#if ENABLE(DATE_AND_TIME_INPUT_TYPES)
    if (!m_shortMonthLabels.isEmpty())
        return;
    const LCTYPE types[12] = {
        LOCALE_SABBREVMONTHNAME1,
        LOCALE_SABBREVMONTHNAME2,
        LOCALE_SABBREVMONTHNAME3,
        LOCALE_SABBREVMONTHNAME4,
        LOCALE_SABBREVMONTHNAME5,
        LOCALE_SABBREVMONTHNAME6,
        LOCALE_SABBREVMONTHNAME7,
        LOCALE_SABBREVMONTHNAME8,
        LOCALE_SABBREVMONTHNAME9,
        LOCALE_SABBREVMONTHNAME10,
        LOCALE_SABBREVMONTHNAME11,
        LOCALE_SABBREVMONTHNAME12,
    };
    m_shortMonthLabels.reserveCapacity(WTF_ARRAY_LENGTH(types));
    for (unsigned i = 0; i < WTF_ARRAY_LENGTH(types); ++i) {
        m_shortMonthLabels.append(getLocaleInfoString(types[i]));
        if (m_shortMonthLabels.last().isEmpty()) {
            m_shortMonthLabels.shrink(0);
            m_shortMonthLabels.reserveCapacity(WTF_ARRAY_LENGTH(WTF::monthName));
            for (unsigned m = 0; m < WTF_ARRAY_LENGTH(WTF::monthName); ++m)
                m_shortMonthLabels.append(WTF::monthName[m]);
            return;
        }
    }
#endif
}

// -------------------------------- Tokenized date format

#if ENABLE(DATE_AND_TIME_INPUT_TYPES)
static unsigned countContinuousLetters(const String& format, unsigned index)
{
    unsigned count = 1;
    UChar reference = format[index];
    while (index + 1 < format.length()) {
        if (format[++index] != reference)
            break;
        ++count;
    }
    return count;
}

static void commitLiteralToken(StringBuilder& literalBuffer, StringBuilder& converted)
{
    if (literalBuffer.length() <= 0)
        return;
    DateTimeFormat::quoteAndAppendLiteral(literalBuffer.toString(), converted);
    literalBuffer.clear();
}

// This function converts Windows date/time pattern format [1][2] into LDML date
// format pattern [3].
//
// i.e.
//   We set h, H, m, s, d, dd, M, or y as is. They have same meaning in both of
//   Windows and LDML.
//   We need to convert the following patterns:
//     t -> a
//     tt -> a
//     ddd -> EEE
//     dddd -> EEEE
//     g -> G
//     gg -> ignore
//
// [1] http://msdn.microsoft.com/en-us/library/dd317787(v=vs.85).aspx
// [2] http://msdn.microsoft.com/en-us/library/dd318148(v=vs.85).aspx
// [3] LDML http://unicode.org/reports/tr35/tr35-6.html#Date_Format_Patterns
static String convertWindowsDateTimeFormat(const String& format)
{
    StringBuilder converted;
    StringBuilder literalBuffer;
    bool inQuote = false;
    bool lastQuoteCanBeLiteral = false;
    for (unsigned i = 0; i < format.length(); ++i) {
        UChar ch = format[i];
        if (inQuote) {
            if (ch == '\'') {
                inQuote = false;
                ASSERT(i);
                if (lastQuoteCanBeLiteral && format[i - 1] == '\'') {
                    literalBuffer.append('\'');
                    lastQuoteCanBeLiteral = false;
                } else
                    lastQuoteCanBeLiteral = true;
            } else
                literalBuffer.append(ch);
            continue;
        }

        if (ch == '\'') {
            inQuote = true;
            if (lastQuoteCanBeLiteral && i > 0 && format[i - 1] == '\'') {
                literalBuffer.append(ch);
                lastQuoteCanBeLiteral = false;
            } else
                lastQuoteCanBeLiteral = true;
        } else if (isASCIIAlpha(ch)) {
            commitLiteralToken(literalBuffer, converted);
            unsigned symbolStart = i;
            unsigned count = countContinuousLetters(format, i);
            i += count - 1;
            if (ch == 'h' || ch == 'H' || ch == 'm' || ch == 's' || ch == 'M' || ch == 'y')
                converted.append(format, symbolStart, count);
            else if (ch == 'd') {
                if (count <= 2)
                    converted.append(format, symbolStart, count);
                else if (count == 3)
                    converted.append("EEE");
                else
                    converted.append("EEEE");
            } else if (ch == 'g') {
                if (count == 1)
                    converted.append('G');
                else {
                    // gg means imperial era in Windows.
                    // Just ignore it.
                }
            } else if (ch == 't')
                converted.append('a');
            else
                literalBuffer.append(format, symbolStart, count);
        } else
            literalBuffer.append(ch);
    }
    commitLiteralToken(literalBuffer, converted);
    return converted.toString();
}
#endif

void LocaleWin::ensureMonthLabels()
{
#if ENABLE(DATE_AND_TIME_INPUT_TYPES)
    if (!m_monthLabels.isEmpty())
        return;
    const LCTYPE types[12] = {
        LOCALE_SMONTHNAME1,
        LOCALE_SMONTHNAME2,
        LOCALE_SMONTHNAME3,
        LOCALE_SMONTHNAME4,
        LOCALE_SMONTHNAME5,
        LOCALE_SMONTHNAME6,
        LOCALE_SMONTHNAME7,
        LOCALE_SMONTHNAME8,
        LOCALE_SMONTHNAME9,
        LOCALE_SMONTHNAME10,
        LOCALE_SMONTHNAME11,
        LOCALE_SMONTHNAME12,
    };
    m_monthLabels.reserveCapacity(WTF_ARRAY_LENGTH(types));
    for (unsigned i = 0; i < WTF_ARRAY_LENGTH(types); ++i) {
        m_monthLabels.append(getLocaleInfoString(types[i]));
        if (m_monthLabels.last().isEmpty()) {
            m_monthLabels.shrink(0);
            m_monthLabels.reserveCapacity(WTF_ARRAY_LENGTH(WTF::monthFullName));
            for (unsigned m = 0; m < WTF_ARRAY_LENGTH(WTF::monthFullName); ++m)
                m_monthLabels.append(WTF::monthFullName[m]);
            return;
        }
    }
#endif
}

#if ENABLE(DATE_AND_TIME_INPUT_TYPES)
const Vector<String>& LocaleWin::monthLabels()
{
    ensureMonthLabels();
    return m_monthLabels;
}

String LocaleWin::dateFormat()
{
    if (m_dateFormat.isNull())
        m_dateFormat = convertWindowsDateTimeFormat(getLocaleInfoString(LOCALE_SSHORTDATE));
    return m_dateFormat;
}

String LocaleWin::dateFormat(const String& windowsFormat)
{
    return convertWindowsDateTimeFormat(windowsFormat);
}

String LocaleWin::monthFormat()
{
    if (m_monthFormat.isNull())
        m_monthFormat = convertWindowsDateTimeFormat(getLocaleInfoString(LOCALE_SYEARMONTH));
    return m_monthFormat;
}

String LocaleWin::shortMonthFormat()
{
    if (m_shortMonthFormat.isNull())
        m_shortMonthFormat = convertWindowsDateTimeFormat(getLocaleInfoString(LOCALE_SYEARMONTH)).replace("MMMM", "MMM");
    return m_shortMonthFormat;
}

String LocaleWin::timeFormat()
{
    if (m_timeFormatWithSeconds.isNull())
        m_timeFormatWithSeconds = convertWindowsDateTimeFormat(getLocaleInfoString(LOCALE_STIMEFORMAT));
    return m_timeFormatWithSeconds;
}

String LocaleWin::shortTimeFormat()
{
    if (!m_timeFormatWithoutSeconds.isNull())
        return m_timeFormatWithoutSeconds;
    String format = getLocaleInfoString(LOCALE_SSHORTTIME);
    // Vista or older Windows doesn't support LOCALE_SSHORTTIME.
    if (format.isEmpty()) {
        format = getLocaleInfoString(LOCALE_STIMEFORMAT);
        StringBuilder builder;
        builder.append(getLocaleInfoString(LOCALE_STIME));
        builder.append("ss");
        size_t pos = format.reverseFind(builder.toString());
        if (pos != notFound)
            format.remove(pos, builder.length());
    }
    m_timeFormatWithoutSeconds = convertWindowsDateTimeFormat(format);
    return m_timeFormatWithoutSeconds;
}

String LocaleWin::dateTimeFormatWithSeconds()
{
    if (!m_dateTimeFormatWithSeconds.isNull())
        return m_dateTimeFormatWithSeconds;
    StringBuilder builder;
    builder.append(dateFormat());
    builder.append(' ');
    builder.append(timeFormat());
    m_dateTimeFormatWithSeconds = builder.toString();
    return m_dateTimeFormatWithSeconds;
}

String LocaleWin::dateTimeFormatWithoutSeconds()
{
    if (!m_dateTimeFormatWithoutSeconds.isNull())
        return m_dateTimeFormatWithoutSeconds;
    StringBuilder builder;
    builder.append(dateFormat());
    builder.append(' ');
    builder.append(shortTimeFormat());
    m_dateTimeFormatWithoutSeconds = builder.toString();
    return m_dateTimeFormatWithoutSeconds;
}

const Vector<String>& LocaleWin::shortMonthLabels()
{
    ensureShortMonthLabels();
    return m_shortMonthLabels;
}

const Vector<String>& LocaleWin::standAloneMonthLabels()
{
    // Windows doesn't provide a way to get stand-alone month labels.
    return monthLabels();
}

const Vector<String>& LocaleWin::shortStandAloneMonthLabels()
{
    // Windows doesn't provide a way to get stand-alone month labels.
    return shortMonthLabels();
}

const Vector<String>& LocaleWin::timeAMPMLabels()
{
    if (m_timeAMPMLabels.isEmpty()) {
        m_timeAMPMLabels.append(getLocaleInfoString(LOCALE_S1159));
        m_timeAMPMLabels.append(getLocaleInfoString(LOCALE_S2359));
    }
    return m_timeAMPMLabels;
}
#endif

void LocaleWin::initializeLocaleData()
{
    if (m_didInitializeNumberData)
        return;

    Vector<String, DecimalSymbolsSize> symbols;
    enum DigitSubstitution {
        DigitSubstitutionContext = 0,
        DigitSubstitution0to9 = 1,
        DigitSubstitutionNative = 2,
    };
    DWORD digitSubstitution = DigitSubstitution0to9;
    getLocaleInfo(LOCALE_IDIGITSUBSTITUTION, digitSubstitution);
    if (digitSubstitution == DigitSubstitution0to9) {
        symbols.append("0");
        symbols.append("1");
        symbols.append("2");
        symbols.append("3");
        symbols.append("4");
        symbols.append("5");
        symbols.append("6");
        symbols.append("7");
        symbols.append("8");
        symbols.append("9");
    } else {
        String digits = getLocaleInfoString(LOCALE_SNATIVEDIGITS);
        ASSERT(digits.length() >= 10);
        for (unsigned i = 0; i < 10; ++i)
            symbols.append(digits.substring(i, 1));
    }
    ASSERT(symbols.size() == DecimalSeparatorIndex);
    symbols.append(getLocaleInfoString(LOCALE_SDECIMAL));
    ASSERT(symbols.size() == GroupSeparatorIndex);
    symbols.append(getLocaleInfoString(LOCALE_STHOUSAND));
    ASSERT(symbols.size() == DecimalSymbolsSize);

    String negativeSign = getLocaleInfoString(LOCALE_SNEGATIVESIGN);
    enum NegativeFormat {
        NegativeFormatParenthesis = 0,
        NegativeFormatSignPrefix = 1,
        NegativeFormatSignSpacePrefix = 2,
        NegativeFormatSignSuffix = 3,
        NegativeFormatSpaceSignSuffix = 4,
    };
    DWORD negativeFormat = NegativeFormatSignPrefix;
    getLocaleInfo(LOCALE_INEGNUMBER, negativeFormat);
    String negativePrefix = emptyString();
    String negativeSuffix = emptyString();
    switch (negativeFormat) {
    case NegativeFormatParenthesis:
        negativePrefix = "(";
        negativeSuffix = ")";
        break;
    case NegativeFormatSignSpacePrefix:
        negativePrefix = negativeSign + " ";
        break;
    case NegativeFormatSignSuffix:
        negativeSuffix = negativeSign;
        break;
    case NegativeFormatSpaceSignSuffix:
        negativeSuffix = " " + negativeSign;
        break;
    case NegativeFormatSignPrefix: // Fall through.
    default:
        negativePrefix = negativeSign;
        break;
    }
    m_didInitializeNumberData = true;
    setLocaleData(symbols, emptyString(), emptyString(), negativePrefix, negativeSuffix);
}

}
