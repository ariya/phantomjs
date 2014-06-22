/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qlocale_p.h"

#include "qstringlist.h"
#include "qvariant.h"
#include "qdatetime.h"

#if !defined(QWS) && defined(Q_OS_MAC)
#   include "private/qcore_mac_p.h"
#   include <CoreFoundation/CoreFoundation.h>
#endif

QT_BEGIN_NAMESPACE

/******************************************************************************
** Wrappers for Mac locale system functions
*/

static QByteArray envVarLocale()
{
    static QByteArray lang = 0;
#ifdef Q_OS_UNIX
    lang = qgetenv("LC_ALL");
    if (lang.isEmpty())
        lang = qgetenv("LC_NUMERIC");
    if (lang.isEmpty())
#endif
        lang = qgetenv("LANG");
    return lang;
}

static QByteArray getMacLocaleName()
{
    QByteArray result = envVarLocale();

    QString lang, script, cntry;
    if (result.isEmpty()
        || (result != "C" && !qt_splitLocaleName(QString::fromLocal8Bit(result), lang, script, cntry))) {
        QCFType<CFLocaleRef> l = CFLocaleCopyCurrent();
        CFStringRef locale = CFLocaleGetIdentifier(l);
        result = QCFString::toQString(locale).toUtf8();
    }
    return result;
}

static QString macMonthName(int month, bool short_format)
{
    month -= 1;
    if (month < 0 || month > 11)
        return QString();

    QCFType<CFDateFormatterRef> formatter
        = CFDateFormatterCreate(0, QCFType<CFLocaleRef>(CFLocaleCopyCurrent()),
                                kCFDateFormatterNoStyle,  kCFDateFormatterNoStyle);
    QCFType<CFArrayRef> values
        = static_cast<CFArrayRef>(CFDateFormatterCopyProperty(formatter,
                                  short_format ? kCFDateFormatterShortMonthSymbols
                                               : kCFDateFormatterMonthSymbols));
    if (values != 0) {
        CFStringRef cfstring = static_cast<CFStringRef>(CFArrayGetValueAtIndex(values, month));
        return QCFString::toQString(cfstring);
    }
    return QString();
}

static QString macDayName(int day, bool short_format)
{
    if (day < 1 || day > 7)
        return QString();

    QCFType<CFDateFormatterRef> formatter
        = CFDateFormatterCreate(0, QCFType<CFLocaleRef>(CFLocaleCopyCurrent()),
                                kCFDateFormatterNoStyle,  kCFDateFormatterNoStyle);
    QCFType<CFArrayRef> values = static_cast<CFArrayRef>(CFDateFormatterCopyProperty(formatter,
                                            short_format ? kCFDateFormatterShortWeekdaySymbols
                                                         : kCFDateFormatterWeekdaySymbols));
    if (values != 0) {
        CFStringRef cfstring = static_cast<CFStringRef>(CFArrayGetValueAtIndex(values, day % 7));
        return QCFString::toQString(cfstring);
    }
    return QString();
}

static QString macDateToString(const QDate &date, bool short_format)
{
    CFGregorianDate macGDate;
    macGDate.year = date.year();
    macGDate.month = date.month();
    macGDate.day = date.day();
    macGDate.hour = 0;
    macGDate.minute = 0;
    macGDate.second = 0.0;
    QCFType<CFDateRef> myDate
        = CFDateCreate(0, CFGregorianDateGetAbsoluteTime(macGDate,
                                                         QCFType<CFTimeZoneRef>(CFTimeZoneCopyDefault())));
    QCFType<CFLocaleRef> mylocale = CFLocaleCopyCurrent();
    CFDateFormatterStyle style = short_format ? kCFDateFormatterShortStyle : kCFDateFormatterLongStyle;
    QCFType<CFDateFormatterRef> myFormatter
        = CFDateFormatterCreate(kCFAllocatorDefault,
                                mylocale, style,
                                kCFDateFormatterNoStyle);
    return QCFString(CFDateFormatterCreateStringWithDate(0, myFormatter, myDate));
}

static QString macTimeToString(const QTime &time, bool short_format)
{
    CFGregorianDate macGDate;
    // Assume this is local time and the current date
    QDate dt = QDate::currentDate();
    macGDate.year = dt.year();
    macGDate.month = dt.month();
    macGDate.day = dt.day();
    macGDate.hour = time.hour();
    macGDate.minute = time.minute();
    macGDate.second = time.second();
    QCFType<CFDateRef> myDate
        = CFDateCreate(0, CFGregorianDateGetAbsoluteTime(macGDate,
                                                         QCFType<CFTimeZoneRef>(CFTimeZoneCopyDefault())));

    QCFType<CFLocaleRef> mylocale = CFLocaleCopyCurrent();
    CFDateFormatterStyle style = short_format ? kCFDateFormatterShortStyle :  kCFDateFormatterLongStyle;
    QCFType<CFDateFormatterRef> myFormatter = CFDateFormatterCreate(kCFAllocatorDefault,
                                                                    mylocale,
                                                                    kCFDateFormatterNoStyle,
                                                                    style);
    return QCFString(CFDateFormatterCreateStringWithDate(0, myFormatter, myDate));
}

// Mac uses the Unicode CLDR format codes
// http://www.unicode.org/reports/tr35/tr35-dates.html#Date_Field_Symbol_Table
// See also qtbase/util/local_database/dateconverter.py
// Makes the assumption that input formats are always well formed and consecutive letters
// never exceed the maximum for the format code.
static QString macToQtFormat(const QString &sys_fmt)
{
    QString result;
    int i = 0;

    while (i < sys_fmt.size()) {
        if (sys_fmt.at(i).unicode() == '\'') {
            QString text = qt_readEscapedFormatString(sys_fmt, &i);
            if (text == QLatin1String("'"))
                result += QLatin1String("''");
            else
                result += QLatin1Char('\'') + text + QLatin1Char('\'');
            continue;
        }

        QChar c = sys_fmt.at(i);
        int repeat = qt_repeatCount(sys_fmt, i);

        switch (c.unicode()) {
            // Qt does not support the following options
            case 'G': // Era (1..5): 4 = long, 1..3 = short, 5 = narrow
            case 'Y': // Year of Week (1..n): 1..n = padded number
            case 'U': // Cyclic Year Name (1..5): 4 = long, 1..3 = short, 5 = narrow
            case 'Q': // Quarter (1..4): 4 = long, 3 = short, 1..2 = padded number
            case 'q': // Standalone Quarter (1..4): 4 = long, 3 = short, 1..2 = padded number
            case 'w': // Week of Year (1..2): 1..2 = padded number
            case 'W': // Week of Month (1): 1 = number
            case 'D': // Day of Year (1..3): 1..3 = padded number
            case 'F': // Day of Week in Month (1): 1 = number
            case 'g': // Modified Julian Day (1..n): 1..n = padded number
            case 'A': // Milliseconds in Day (1..n): 1..n = padded number
                break;

            case 'y': // Year (1..n): 2 = short year, 1 & 3..n = padded number
            case 'u': // Extended Year (1..n): 2 = short year, 1 & 3..n = padded number
                // Qt only supports long (4) or short (2) year, use long for all others
                if (repeat == 2)
                    result += QLatin1String("yy");
                else
                    result += QLatin1String("yyyy");
                break;
            case 'M': // Month (1..5): 4 = long, 3 = short, 1..2 = number, 5 = narrow
            case 'L': // Standalone Month (1..5): 4 = long, 3 = short, 1..2 = number, 5 = narrow
                // Qt only supports long, short and number, use short for narrow
                if (repeat == 5)
                    result += QLatin1String("MMM");
                else
                    result += QString(repeat, QLatin1Char('M'));
                break;
            case 'd': // Day of Month (1..2): 1..2 padded number
                result += QString(repeat, c);
                break;
            case 'E': // Day of Week (1..6): 4 = long, 1..3 = short, 5..6 = narrow
                // Qt only supports long, short and padded number, use short for narrow
                if (repeat == 4)
                    result += QLatin1String("dddd");
                else
                    result += QLatin1String("ddd");
                break;
            case 'e': // Local Day of Week (1..6): 4 = long, 3 = short, 5..6 = narrow, 1..2 padded number
            case 'c': // Standalone Local Day of Week (1..6): 4 = long, 3 = short, 5..6 = narrow, 1..2 padded number
                // Qt only supports long, short and padded number, use short for narrow
                if (repeat >= 5)
                    result += QLatin1String("ddd");
                else
                    result += QString(repeat, QLatin1Char('d'));
                break;
            case 'a': // AM/PM (1): 1 = short
                // Translate to Qt uppercase AM/PM
                result += QLatin1String("AP");
                break;
            case 'h': // Hour [1..12] (1..2): 1..2 = padded number
            case 'K': // Hour [0..11] (1..2): 1..2 = padded number
            case 'j': // Local Hour [12 or 24] (1..2): 1..2 = padded number
                // Qt h is local hour
                result += QString(repeat, QLatin1Char('h'));
                break;
            case 'H': // Hour [0..23] (1..2): 1..2 = padded number
            case 'k': // Hour [1..24] (1..2): 1..2 = padded number
                // Qt H is 0..23 hour
                result += QString(repeat, QLatin1Char('H'));
                break;
            case 'm': // Minutes (1..2): 1..2 = padded number
            case 's': // Seconds (1..2): 1..2 = padded number
                result += QString(repeat, c);
                break;
            case 'S': // Fractional second (1..n): 1..n = truncates to decimal places
                // Qt uses msecs either unpadded or padded to 3 places
                if (repeat < 3)
                    result += QLatin1Char('z');
                else
                    result += QLatin1String("zzz");
                break;
            case 'z': // Time Zone (1..4)
            case 'Z': // Time Zone (1..5)
            case 'O': // Time Zone (1, 4)
            case 'v': // Time Zone (1, 4)
            case 'V': // Time Zone (1..4)
            case 'X': // Time Zone (1..5)
            case 'x': // Time Zone (1..5)
                result += QLatin1Char('t');
                break;
            default:
                // a..z and A..Z are reserved for format codes, so any occurrence of these not
                // already processed are not known and so unsupported formats to be ignored.
                // All other chars are allowed as literals.
                if (c < QLatin1Char('A') || c > QLatin1Char('z') ||
                    (c > QLatin1Char('Z') && c < QLatin1Char('a'))) {
                    result += QString(repeat, c);
                }
                break;
        }

        i += repeat;
    }

    return result;
}

QString getMacDateFormat(CFDateFormatterStyle style)
{
    QCFType<CFLocaleRef> l = CFLocaleCopyCurrent();
    QCFType<CFDateFormatterRef> formatter = CFDateFormatterCreate(kCFAllocatorDefault,
                                                                  l, style, kCFDateFormatterNoStyle);
    return macToQtFormat(QCFString::toQString(CFDateFormatterGetFormat(formatter)));
}

static QString getMacTimeFormat(CFDateFormatterStyle style)
{
    QCFType<CFLocaleRef> l = CFLocaleCopyCurrent();
    QCFType<CFDateFormatterRef> formatter = CFDateFormatterCreate(kCFAllocatorDefault,
                                                                  l, kCFDateFormatterNoStyle, style);
    return macToQtFormat(QCFString::toQString(CFDateFormatterGetFormat(formatter)));
}

static QString getCFLocaleValue(CFStringRef key)
{
    QCFType<CFLocaleRef> locale = CFLocaleCopyCurrent();
    CFTypeRef value = CFLocaleGetValue(locale, key);
    return QCFString::toQString(CFStringRef(static_cast<CFTypeRef>(value)));
}

static QLocale::MeasurementSystem macMeasurementSystem()
{
    QCFType<CFLocaleRef> locale = CFLocaleCopyCurrent();
    CFStringRef system = static_cast<CFStringRef>(CFLocaleGetValue(locale, kCFLocaleMeasurementSystem));
    if (QCFString::toQString(system) == QLatin1String("Metric")) {
        return QLocale::MetricSystem;
    } else {
        return QLocale::ImperialSystem;
    }
}


static quint8 macFirstDayOfWeek()
{
    QCFType<CFCalendarRef> calendar = CFCalendarCopyCurrent();
    quint8 day = static_cast<quint8>(CFCalendarGetFirstWeekday(calendar))-1;
    if (day == 0)
        day = 7;
    return day;
}

static QString macCurrencySymbol(QLocale::CurrencySymbolFormat format)
{
    QCFType<CFLocaleRef> locale = CFLocaleCopyCurrent();
    switch (format) {
    case QLocale::CurrencyIsoCode:
        return QCFString::toQString(static_cast<CFStringRef>(CFLocaleGetValue(locale, kCFLocaleCurrencyCode)));
    case QLocale::CurrencySymbol:
        return QCFString::toQString(static_cast<CFStringRef>(CFLocaleGetValue(locale, kCFLocaleCurrencySymbol)));
    case QLocale::CurrencyDisplayName: {
        CFStringRef code = static_cast<CFStringRef>(CFLocaleGetValue(locale, kCFLocaleCurrencyCode));
        QCFType<CFStringRef> value = CFLocaleCopyDisplayNameForPropertyValue(locale, kCFLocaleCurrencyCode, code);
        return QCFString::toQString(value);
    }
    default:
        break;
    }
    return QString();
}

#ifndef QT_NO_SYSTEMLOCALE
static QString macFormatCurrency(const QSystemLocale::CurrencyToStringArgument &arg)
{
    QCFType<CFNumberRef> value;
    switch (arg.value.type()) {
    case QVariant::Int:
    case QVariant::UInt: {
        int v = arg.value.toInt();
        value = CFNumberCreate(NULL, kCFNumberIntType, &v);
        break;
    }
    case QVariant::Double: {
        double v = arg.value.toDouble();
        value = CFNumberCreate(NULL, kCFNumberDoubleType, &v);
        break;
    }
    case QVariant::LongLong:
    case QVariant::ULongLong: {
        qint64 v = arg.value.toLongLong();
        value = CFNumberCreate(NULL, kCFNumberLongLongType, &v);
        break;
    }
    default:
        return QString();
    }

    QCFType<CFLocaleRef> locale = CFLocaleCopyCurrent();
    QCFType<CFNumberFormatterRef> currencyFormatter =
            CFNumberFormatterCreate(NULL, locale, kCFNumberFormatterCurrencyStyle);
    if (!arg.symbol.isEmpty()) {
        CFNumberFormatterSetProperty(currencyFormatter, kCFNumberFormatterCurrencySymbol,
                                     QCFString::toCFStringRef(arg.symbol));
    }
    QCFType<CFStringRef> result = CFNumberFormatterCreateStringWithNumber(NULL, currencyFormatter, value);
    return QCFString::toQString(result);
}

static QVariant macQuoteString(QSystemLocale::QueryType type, const QStringRef &str)
{
    if (QSysInfo::MacintoshVersion < QSysInfo::MV_10_6)
        return QVariant();

    QString begin, end;
    QCFType<CFLocaleRef> locale = CFLocaleCopyCurrent();
    switch (type) {
    case QSystemLocale::StringToStandardQuotation:
        begin = QCFString::toQString(static_cast<CFStringRef>(CFLocaleGetValue(locale, kCFLocaleQuotationBeginDelimiterKey)));
        end = QCFString::toQString(static_cast<CFStringRef>(CFLocaleGetValue(locale, kCFLocaleQuotationEndDelimiterKey)));
        return QString(begin % str % end);
    case QSystemLocale::StringToAlternateQuotation:
        begin = QCFString::toQString(static_cast<CFStringRef>(CFLocaleGetValue(locale, kCFLocaleAlternateQuotationBeginDelimiterKey)));
        end = QCFString::toQString(static_cast<CFStringRef>(CFLocaleGetValue(locale, kCFLocaleAlternateQuotationEndDelimiterKey)));
        return QString(begin % str % end);
     default:
        break;
    }
    return QVariant();
}
#endif //QT_NO_SYSTEMLOCALE

#ifndef QT_NO_SYSTEMLOCALE

QLocale QSystemLocale::fallbackUiLocale() const
{
    return QLocale(QString::fromUtf8(getMacLocaleName().constData()));
}

QVariant QSystemLocale::query(QueryType type, QVariant in = QVariant()) const
{
    switch(type) {
//     case Name:
//         return getMacLocaleName();
    case DecimalPoint: {
        QString value = getCFLocaleValue(kCFLocaleDecimalSeparator);
        return value.isEmpty() ? QVariant() : value;
    }
    case GroupSeparator: {
        QString value = getCFLocaleValue(kCFLocaleGroupingSeparator);
        return value.isEmpty() ? QVariant() : value;
    }
    case DateFormatLong:
    case DateFormatShort:
        return getMacDateFormat(type == DateFormatShort
                                ? kCFDateFormatterShortStyle
                                : kCFDateFormatterLongStyle);
    case TimeFormatLong:
    case TimeFormatShort:
        return getMacTimeFormat(type == TimeFormatShort
                                ? kCFDateFormatterShortStyle
                                : kCFDateFormatterLongStyle);
    case DayNameLong:
    case DayNameShort:
        return macDayName(in.toInt(), (type == DayNameShort));
    case MonthNameLong:
    case MonthNameShort:
    case StandaloneMonthNameLong:
    case StandaloneMonthNameShort:
        return macMonthName(in.toInt(), (type == MonthNameShort || type == StandaloneMonthNameShort));
    case DateToStringShort:
    case DateToStringLong:
        return macDateToString(in.toDate(), (type == DateToStringShort));
    case TimeToStringShort:
    case TimeToStringLong:
        return macTimeToString(in.toTime(), (type == TimeToStringShort));

    case NegativeSign:
    case PositiveSign:
    case ZeroDigit:
        break;

    case MeasurementSystem:
        return QVariant(static_cast<int>(macMeasurementSystem()));

    case AMText:
    case PMText: {
        QCFType<CFLocaleRef> locale = CFLocaleCopyCurrent();
        QCFType<CFDateFormatterRef> formatter = CFDateFormatterCreate(NULL, locale, kCFDateFormatterLongStyle, kCFDateFormatterLongStyle);
        QCFType<CFStringRef> value = static_cast<CFStringRef>(CFDateFormatterCopyProperty(formatter,
            (type == AMText ? kCFDateFormatterAMSymbol : kCFDateFormatterPMSymbol)));
        return QCFString::toQString(value);
    }
    case FirstDayOfWeek:
        return QVariant(macFirstDayOfWeek());
    case CurrencySymbol:
        return QVariant(macCurrencySymbol(QLocale::CurrencySymbolFormat(in.toUInt())));
    case CurrencyToString:
        return macFormatCurrency(in.value<QSystemLocale::CurrencyToStringArgument>());
    case UILanguages: {
        QCFType<CFPropertyListRef> languages = CFPreferencesCopyValue(
                 CFSTR("AppleLanguages"),
                 kCFPreferencesAnyApplication,
                 kCFPreferencesCurrentUser,
                 kCFPreferencesAnyHost);
        QStringList result;
        if (!languages)
            return QVariant(result);

        CFTypeID typeId = CFGetTypeID(languages);
        if (typeId == CFArrayGetTypeID()) {
            const int cnt = CFArrayGetCount(languages.as<CFArrayRef>());
            result.reserve(cnt);
            for (int i = 0; i < cnt; ++i) {
                const QString lang = QCFString::toQString(
                            static_cast<CFStringRef>(CFArrayGetValueAtIndex(languages.as<CFArrayRef>(), i)));
                result.append(lang);
            }
        } else if (typeId == CFStringGetTypeID()) {
            result = QStringList(QCFString::toQString(languages.as<CFStringRef>()));
        } else {
            qWarning("QLocale::uiLanguages(): CFPreferencesCopyValue returned unhandled type \"%s\"; please report to http://bugreports.qt-project.org",
                     qPrintable(QCFString::toQString(CFCopyTypeIDDescription(typeId))));
        }
        return QVariant(result);
    }
    case StringToStandardQuotation:
    case StringToAlternateQuotation:
        return macQuoteString(type, in.value<QStringRef>());
    default:
        break;
    }
    return QVariant();
}

#endif // QT_NO_SYSTEMLOCALE

QT_END_NAMESPACE
