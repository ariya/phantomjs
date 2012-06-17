/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
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
    if (result.isEmpty() || result != "C"
            && !qt_splitLocaleName(QString::fromLocal8Bit(result), lang, script, cntry)) {
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
            case 'G': // Qt doesn't support these :(
            case 'Y':
            case 'D':
            case 'F':
            case 'w':
            case 'W':
            case 'g':
                break;

            case 'u': // extended year - use 'y'
                if (repeat < 4)
                    result += QLatin1String("yy");
                else
                    result += QLatin1String("yyyy");
                break;
            case 'S': // fractional second
                if (repeat < 3)
                    result += QLatin1Char('z');
                else
                    result += QLatin1String("zzz");
                break;
            case 'E':
                if (repeat <= 3)
                    result += QLatin1String("ddd");
                else
                    result += QLatin1String("dddd");
                break;
            case 'e':
                if (repeat >= 2)
                    result += QLatin1String("dd");
                else
                    result += QLatin1Char('d');
                break;
            case 'a':
                result += QLatin1String("AP");
                break;
            case 'k':
                result += QString(repeat, QLatin1Char('H'));
                break;
            case 'K':
                result += QString(repeat, QLatin1Char('h'));
                break;
            case 'z':
            case 'Z':
            case 'v':
                result += QLatin1Char('t');
                break;
            default:
                result += QString(repeat, c);
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
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_6
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
#endif
    return QVariant();
}
#endif //QT_NO_SYSTEMLOCALE

#ifndef QT_NO_SYSTEMLOCALE

QLocale QSystemLocale::fallbackLocale() const
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
        return macMonthName(in.toInt(), (type == MonthNameShort));
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
        QCFType<CFArrayRef> languages = (CFArrayRef)CFPreferencesCopyValue(
                 CFSTR("AppleLanguages"),
                 kCFPreferencesAnyApplication,
                 kCFPreferencesCurrentUser,
                 kCFPreferencesAnyHost);
        const int cnt = CFArrayGetCount(languages);
        QStringList result;
        result.reserve(cnt);
        for (int i = 0; i < cnt; ++i) {
            const QString lang = QCFString::toQString(
                        static_cast<CFStringRef>(CFArrayGetValueAtIndex(languages, i)));
            result.append(lang);
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
