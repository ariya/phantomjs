/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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
#include "qlocale_tools_p.h"

#include "qstringlist.h"
#include "qvariant.h"
#include "qdatetime.h"

#include "private/qsystemlibrary_p.h"

#include "qdebug.h"

#if defined(Q_OS_WIN)
#   include "qt_windows.h"
#   include <time.h>
#endif

QT_BEGIN_NAMESPACE

static QByteArray getWinLocaleName(LCID id = LOCALE_USER_DEFAULT);
static const char *winLangCodeToIsoName(int code);
static QString winIso639LangName(LCID id = LOCALE_USER_DEFAULT);
static QString winIso3116CtryName(LCID id = LOCALE_USER_DEFAULT);

#ifndef QT_NO_SYSTEMLOCALE

#ifndef MUI_LANGUAGE_NAME
#define MUI_LANGUAGE_NAME 0x8
#endif
#ifndef LOCALE_SSHORTESTDAYNAME1
#  define LOCALE_SSHORTESTDAYNAME1 0x0060
#  define LOCALE_SSHORTESTDAYNAME2 0x0061
#  define LOCALE_SSHORTESTDAYNAME3 0x0062
#  define LOCALE_SSHORTESTDAYNAME4 0x0063
#  define LOCALE_SSHORTESTDAYNAME5 0x0064
#  define LOCALE_SSHORTESTDAYNAME6 0x0065
#  define LOCALE_SSHORTESTDAYNAME7 0x0066
#endif
#ifndef LOCALE_SNATIVELANGUAGENAME
#  define LOCALE_SNATIVELANGUAGENAME 0x00000004
#endif
#ifndef LOCALE_SNATIVECOUNTRYNAME
#  define LOCALE_SNATIVECOUNTRYNAME 0x00000008
#endif

struct QSystemLocalePrivate
{
    QSystemLocalePrivate();

    QChar zeroDigit();
    QChar decimalPoint();
    QChar groupSeparator();
    QChar negativeSign();
    QChar positiveSign();
    QVariant dateFormat(QLocale::FormatType);
    QVariant timeFormat(QLocale::FormatType);
    QVariant dateTimeFormat(QLocale::FormatType);
    QVariant dayName(int, QLocale::FormatType);
    QVariant monthName(int, QLocale::FormatType);
    QVariant toString(const QDate &, QLocale::FormatType);
    QVariant toString(const QTime &, QLocale::FormatType);
    QVariant toString(const QDateTime &, QLocale::FormatType);
    QVariant measurementSystem();
    QVariant amText();
    QVariant pmText();
    QVariant firstDayOfWeek();
    QVariant currencySymbol(QLocale::CurrencySymbolFormat);
    QVariant toCurrencyString(const QSystemLocale::CurrencyToStringArgument &);
    QVariant uiLanguages();
    QVariant nativeLanguageName();
    QVariant nativeCountryName();

    void update();

private:
    QByteArray langEnvVar;

    enum SubstitutionType {
        SUnknown,
        SContext,
        SAlways,
        SNever
    };

    // cached values:
    LCID lcid;
    SubstitutionType substitutionType;
    QChar zero;

    QString getLocaleInfo(LCTYPE type, int maxlen = 0);
    int getLocaleInfo_int(LCTYPE type, int maxlen = 0);
    QChar getLocaleInfo_qchar(LCTYPE type);

    SubstitutionType substitution();
    QString &substituteDigits(QString &string);

    static QString winToQtFormat(const QString &sys_fmt);

};
Q_GLOBAL_STATIC(QSystemLocalePrivate, systemLocalePrivate)

QSystemLocalePrivate::QSystemLocalePrivate()
    : substitutionType(SUnknown)
{
    langEnvVar = qgetenv("LANG");
    lcid = GetUserDefaultLCID();
}

QString QSystemLocalePrivate::getLocaleInfo(LCTYPE type, int maxlen)
{
    QVarLengthArray<wchar_t, 64> buf(maxlen ? maxlen : 64);
    if (!GetLocaleInfo(lcid, type, buf.data(), buf.size()))
        return QString();
    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
        int cnt = GetLocaleInfo(lcid, type, 0, 0);
        if (cnt == 0)
            return QString();
        buf.resize(cnt);
        if (!GetLocaleInfo(lcid, type, buf.data(), buf.size()))
            return QString();
    }
    return QString::fromWCharArray(buf.data());
}

int QSystemLocalePrivate::getLocaleInfo_int(LCTYPE type, int maxlen)
{
    QString str = getLocaleInfo(type, maxlen);
    bool ok = false;
    int v = str.toInt(&ok);
    return ok ? v : 0;
}

QChar QSystemLocalePrivate::getLocaleInfo_qchar(LCTYPE type)
{
    QString str = getLocaleInfo(type);
    return str.isEmpty() ? QChar() : str.at(0);
}

QSystemLocalePrivate::SubstitutionType QSystemLocalePrivate::substitution()
{
    if (substitutionType == SUnknown) {
        wchar_t buf[8];
        if (!GetLocaleInfo(lcid, LOCALE_IDIGITSUBSTITUTION, buf, 8)) {
            substitutionType = QSystemLocalePrivate::SNever;
            return substitutionType;
        }
        if (buf[0] == '1')
            substitutionType = QSystemLocalePrivate::SNever;
        else if (buf[0] == '0')
            substitutionType = QSystemLocalePrivate::SContext;
        else if (buf[0] == '2')
            substitutionType = QSystemLocalePrivate::SAlways;
        else {
            wchar_t digits[11];
            if (!GetLocaleInfo(lcid, LOCALE_SNATIVEDIGITS, digits, 11)) {
                substitutionType = QSystemLocalePrivate::SNever;
                return substitutionType;
            }
            const wchar_t zero = digits[0];
            if (buf[0] == zero + 2)
                substitutionType = QSystemLocalePrivate::SAlways;
            else
                substitutionType = QSystemLocalePrivate::SNever;
        }
    }
    return substitutionType;
}

QString &QSystemLocalePrivate::substituteDigits(QString &string)
{
    ushort zero = zeroDigit().unicode();
    ushort *qch = (ushort *)string.data();
    for (ushort *end = qch + string.size(); qch != end; ++qch) {
        if (*qch >= '0' && *qch <= '9')
            *qch = zero + (*qch - '0');
    }
    return string;
}

QChar QSystemLocalePrivate::zeroDigit()
{
    if (zero.isNull())
        zero = getLocaleInfo_qchar(LOCALE_SNATIVEDIGITS);
    return zero;
}

QChar QSystemLocalePrivate::decimalPoint()
{
    return getLocaleInfo_qchar(LOCALE_SDECIMAL);
}

QChar QSystemLocalePrivate::groupSeparator()
{
    return getLocaleInfo_qchar(LOCALE_STHOUSAND);
}

QChar QSystemLocalePrivate::negativeSign()
{
    return getLocaleInfo_qchar(LOCALE_SNEGATIVESIGN);
}

QChar QSystemLocalePrivate::positiveSign()
{
    return getLocaleInfo_qchar(LOCALE_SPOSITIVESIGN);
}

QVariant QSystemLocalePrivate::dateFormat(QLocale::FormatType type)
{
    switch (type) {
    case QLocale::ShortFormat:
        return winToQtFormat(getLocaleInfo(LOCALE_SSHORTDATE));
    case QLocale::LongFormat:
        return winToQtFormat(getLocaleInfo(LOCALE_SLONGDATE));
    case QLocale::NarrowFormat:
        break;
    }
    return QVariant();
}

QVariant QSystemLocalePrivate::timeFormat(QLocale::FormatType type)
{
    switch (type) {
    case QLocale::ShortFormat:
        return winToQtFormat(getLocaleInfo(LOCALE_STIMEFORMAT)); //###
    case QLocale::LongFormat:
        return winToQtFormat(getLocaleInfo(LOCALE_STIMEFORMAT));
    case QLocale::NarrowFormat:
        break;
    }
    return QVariant();
}

QVariant QSystemLocalePrivate::dateTimeFormat(QLocale::FormatType type)
{
    return QString(dateFormat(type).toString() + QLatin1Char(' ') + timeFormat(type).toString());
}

QVariant QSystemLocalePrivate::dayName(int day, QLocale::FormatType type)
{
    static const LCTYPE short_day_map[]
        = { LOCALE_SABBREVDAYNAME1, LOCALE_SABBREVDAYNAME2,
            LOCALE_SABBREVDAYNAME3, LOCALE_SABBREVDAYNAME4, LOCALE_SABBREVDAYNAME5,
            LOCALE_SABBREVDAYNAME6, LOCALE_SABBREVDAYNAME7 };

    static const LCTYPE long_day_map[]
        = { LOCALE_SDAYNAME1, LOCALE_SDAYNAME2,
            LOCALE_SDAYNAME3, LOCALE_SDAYNAME4, LOCALE_SDAYNAME5,
            LOCALE_SDAYNAME6, LOCALE_SDAYNAME7 };

    static const LCTYPE narrow_day_map[]
        = { LOCALE_SSHORTESTDAYNAME1, LOCALE_SSHORTESTDAYNAME2,
            LOCALE_SSHORTESTDAYNAME3, LOCALE_SSHORTESTDAYNAME4,
            LOCALE_SSHORTESTDAYNAME5, LOCALE_SSHORTESTDAYNAME6,
            LOCALE_SSHORTESTDAYNAME7 };

    day -= 1;

    if (type == QLocale::LongFormat)
        return getLocaleInfo(long_day_map[day]);
    else if (type == QLocale::NarrowFormat && QSysInfo::windowsVersion() >= QSysInfo::WV_VISTA)
        return getLocaleInfo(narrow_day_map[day]);
    return getLocaleInfo(short_day_map[day]);
}

QVariant QSystemLocalePrivate::monthName(int month, QLocale::FormatType type)
{
    static const LCTYPE short_month_map[]
        = { LOCALE_SABBREVMONTHNAME1, LOCALE_SABBREVMONTHNAME2, LOCALE_SABBREVMONTHNAME3,
            LOCALE_SABBREVMONTHNAME4, LOCALE_SABBREVMONTHNAME5, LOCALE_SABBREVMONTHNAME6,
            LOCALE_SABBREVMONTHNAME7, LOCALE_SABBREVMONTHNAME8, LOCALE_SABBREVMONTHNAME9,
            LOCALE_SABBREVMONTHNAME10, LOCALE_SABBREVMONTHNAME11, LOCALE_SABBREVMONTHNAME12 };

    static const LCTYPE long_month_map[]
        = { LOCALE_SMONTHNAME1, LOCALE_SMONTHNAME2, LOCALE_SMONTHNAME3,
            LOCALE_SMONTHNAME4, LOCALE_SMONTHNAME5, LOCALE_SMONTHNAME6,
            LOCALE_SMONTHNAME7, LOCALE_SMONTHNAME8, LOCALE_SMONTHNAME9,
            LOCALE_SMONTHNAME10, LOCALE_SMONTHNAME11, LOCALE_SMONTHNAME12 };

    month -= 1;
    if (month < 0 || month > 11)
    return QString();

    LCTYPE lctype = (type == QLocale::ShortFormat || type == QLocale::NarrowFormat)
            ? short_month_map[month] : long_month_map[month];
    return getLocaleInfo(lctype);
}

QVariant QSystemLocalePrivate::toString(const QDate &date, QLocale::FormatType type)
{
    SYSTEMTIME st;
    memset(&st, 0, sizeof(SYSTEMTIME));
    st.wYear = date.year();
    st.wMonth = date.month();
    st.wDay = date.day();

    DWORD flags = (type == QLocale::LongFormat ? DATE_LONGDATE : DATE_SHORTDATE);
    wchar_t buf[255];
    if (GetDateFormat(lcid, flags, &st, NULL, buf, 255)) {
        QString format = QString::fromWCharArray(buf);
        if (substitution() == SAlways)
            substituteDigits(format);
        return format;
    }
    return QString();
}

QVariant QSystemLocalePrivate::toString(const QTime &time, QLocale::FormatType)
{
    SYSTEMTIME st;
    memset(&st, 0, sizeof(SYSTEMTIME));
    st.wHour = time.hour();
    st.wMinute = time.minute();
    st.wSecond = time.second();
    st.wMilliseconds = 0;

    DWORD flags = 0;

    wchar_t buf[255];
    if (GetTimeFormat(lcid, flags, &st, NULL, buf, 255)) {
        QString format = QString::fromWCharArray(buf);
        if (substitution() == SAlways)
            substituteDigits(format);
        return format;
    }
    return QString();
}

QVariant QSystemLocalePrivate::toString(const QDateTime &dt, QLocale::FormatType type)
{
    return QString(toString(dt.date(), type).toString() + QLatin1Char(' ') + toString(dt.time(), type).toString());
}

QVariant QSystemLocalePrivate::measurementSystem()
{
    wchar_t output[2];

    if (GetLocaleInfo(lcid, LOCALE_IMEASURE, output, 2)) {
        QString iMeasure = QString::fromWCharArray(output);
        if (iMeasure == QLatin1String("1")) {
            return QLocale::ImperialSystem;
        }
    }

    return QLocale::MetricSystem;
}

QVariant QSystemLocalePrivate::amText()
{
    wchar_t output[15]; // maximum length including  terminating zero character for Win2003+

    if (GetLocaleInfo(lcid, LOCALE_S1159, output, 15)) {
        return QString::fromWCharArray(output);
    }

    return QVariant();
}

QVariant QSystemLocalePrivate::pmText()
{
    wchar_t output[15]; // maximum length including  terminating zero character for Win2003+

    if (GetLocaleInfo(lcid, LOCALE_S2359, output, 15)) {
        return QString::fromWCharArray(output);
    }

    return QVariant();
}

QVariant QSystemLocalePrivate::firstDayOfWeek()
{
    wchar_t output[4]; // maximum length including  terminating zero character for Win2003+

    if (GetLocaleInfo(lcid, LOCALE_IFIRSTDAYOFWEEK, output, 4))
        return QString::fromWCharArray(output).toUInt()+1;

    return 1;
}

QVariant QSystemLocalePrivate::currencySymbol(QLocale::CurrencySymbolFormat format)
{
    wchar_t buf[13];
    switch (format) {
    case QLocale::CurrencySymbol:
        if (GetLocaleInfo(lcid, LOCALE_SCURRENCY, buf, 13))
            return QString::fromWCharArray(buf);
        break;
    case QLocale::CurrencyIsoCode:
        if (GetLocaleInfo(lcid, LOCALE_SINTLSYMBOL, buf, 9))
            return QString::fromWCharArray(buf);
        break;
    case QLocale::CurrencyDisplayName: {
        QVarLengthArray<wchar_t, 64> buf(64);
        if (!GetLocaleInfo(lcid, LOCALE_SNATIVECURRNAME, buf.data(), buf.size())) {
            if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
                break;
            buf.resize(255); // should be large enough, right?
            if (!GetLocaleInfo(lcid, LOCALE_SNATIVECURRNAME, buf.data(), buf.size()))
                break;
        }
        return QString::fromWCharArray(buf.data());
    }
    default:
        break;
    }
    return QVariant();
}

QVariant QSystemLocalePrivate::toCurrencyString(const QSystemLocale::CurrencyToStringArgument &arg)
{
    QString value;
    switch (arg.value.type()) {
    case QVariant::Int:
        value = QLocalePrivate::longLongToString(QLatin1Char('0'), QLatin1Char(','), QLatin1Char('+'), QLatin1Char('-'),
                                                 arg.value.toInt(), -1, 10, -1, QLocale::OmitGroupSeparator);
        break;
    case QVariant::UInt:
        value = QLocalePrivate::unsLongLongToString(QLatin1Char('0'), QLatin1Char(','), QLatin1Char('+'),
                                                    arg.value.toUInt(), -1, 10, -1, QLocale::OmitGroupSeparator);
        break;
    case QVariant::Double:
        value = QLocalePrivate::doubleToString(QLatin1Char('0'), QLatin1Char('+'), QLatin1Char('-'),
                                               QLatin1Char(' '), QLatin1Char(','), QLatin1Char('.'),
                                               arg.value.toDouble(), -1, QLocalePrivate::DFDecimal, -1, QLocale::OmitGroupSeparator);
        break;
    case QVariant::LongLong:
        value = QLocalePrivate::longLongToString(QLatin1Char('0'), QLatin1Char(','), QLatin1Char('+'), QLatin1Char('-'),
                                                 arg.value.toLongLong(), -1, 10, -1, QLocale::OmitGroupSeparator);
        break;
    case QVariant::ULongLong:
        value = QLocalePrivate::unsLongLongToString(QLatin1Char('0'), QLatin1Char(','), QLatin1Char('+'),
                                                    arg.value.toULongLong(), -1, 10, -1, QLocale::OmitGroupSeparator);
        break;
    default:
        return QVariant();
    }

    QVarLengthArray<wchar_t, 64> out(64);

    QString decimalSep;
    QString thousandSep;
    CURRENCYFMT format;
    CURRENCYFMT *pformat = NULL;
    if (!arg.symbol.isEmpty()) {
        format.NumDigits = getLocaleInfo_int(lcid, LOCALE_ICURRDIGITS);
        format.LeadingZero = getLocaleInfo_int(lcid, LOCALE_ILZERO);
        decimalSep = getLocaleInfo(lcid, LOCALE_SMONDECIMALSEP);
        format.lpDecimalSep = (wchar_t *)decimalSep.utf16();
        thousandSep = getLocaleInfo(lcid, LOCALE_SMONTHOUSANDSEP);
        format.lpThousandSep = (wchar_t *)thousandSep.utf16();
        format.NegativeOrder = getLocaleInfo_int(lcid, LOCALE_INEGCURR);
        format.PositiveOrder = getLocaleInfo_int(lcid, LOCALE_ICURRENCY);
        format.lpCurrencySymbol = (wchar_t *)arg.symbol.utf16();

        // grouping is complicated and ugly:
        // int(0)  == "123456789.00"    == string("0")
        // int(3)  == "123,456,789.00"  == string("3;0")
        // int(30) == "123456,789.00"   == string("3;0;0")
        // int(32) == "12,34,56,789.00" == string("3;2;0")
        // int(320)== "1234,56,789.00"  == string("3;2")
        QString groupingStr = getLocaleInfo(lcid, LOCALE_SMONGROUPING);
        format.Grouping = groupingStr.remove(QLatin1Char(';')).toInt();
        if (format.Grouping % 10 == 0) // magic
            format.Grouping /= 10;
        else
            format.Grouping *= 10;
        pformat = &format;
    }

    int ret = ::GetCurrencyFormat(lcid, 0, reinterpret_cast<const wchar_t *>(value.utf16()),
                                  pformat, out.data(), out.size());
    if (ret == 0 && GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
        ret = ::GetCurrencyFormat(lcid, 0, reinterpret_cast<const wchar_t *>(value.utf16()),
                                  pformat, out.data(), 0);
        out.resize(ret);
        ::GetCurrencyFormat(lcid, 0, reinterpret_cast<const wchar_t *>(value.utf16()),
                            pformat, out.data(), out.size());
    }

    value = QString::fromWCharArray(out.data());
    if (substitution() == SAlways)
        substituteDigits( value);
    return value;
}

QVariant QSystemLocalePrivate::uiLanguages()
{
    if (QSysInfo::windowsVersion() >= QSysInfo::WV_VISTA) {
        typedef BOOL (WINAPI *GetUserPreferredUILanguagesFunc) (
                    DWORD dwFlags,
                    PULONG pulNumLanguages,
                    PWSTR pwszLanguagesBuffer,
                    PULONG pcchLanguagesBuffer);
        static GetUserPreferredUILanguagesFunc GetUserPreferredUILanguages_ptr = 0;
        if (!GetUserPreferredUILanguages_ptr) {
            QSystemLibrary lib(QLatin1String("kernel32"));
            if (lib.load())
                GetUserPreferredUILanguages_ptr = (GetUserPreferredUILanguagesFunc)lib.resolve("GetUserPreferredUILanguages");
        }
        if (GetUserPreferredUILanguages_ptr) {
            unsigned long cnt = 0;
            QVarLengthArray<wchar_t, 64> buf(64);
            unsigned long size = buf.size();
            if (!GetUserPreferredUILanguages_ptr(MUI_LANGUAGE_NAME, &cnt, buf.data(), &size)) {
                size = 0;
                if (GetLastError() == ERROR_INSUFFICIENT_BUFFER &&
                    GetUserPreferredUILanguages_ptr(MUI_LANGUAGE_NAME, &cnt, NULL, &size)) {
                    buf.resize(size);
                    if (!GetUserPreferredUILanguages_ptr(MUI_LANGUAGE_NAME, &cnt, buf.data(), &size))
                        return QStringList();
                }
            }
            QStringList result;
            result.reserve(cnt);
            const wchar_t *str = buf.constData();
            for (; cnt > 0; --cnt) {
                QString s = QString::fromWCharArray(str);
                if (s.isEmpty())
                    break; // something is wrong
                result.append(s);
                str += s.size()+1;
            }
            return result;
        }
    }

    // old Windows before Vista
    return QStringList(QString::fromLatin1(winLangCodeToIsoName(GetUserDefaultUILanguage())));
}

QVariant QSystemLocalePrivate::nativeLanguageName()
{
    if (QSysInfo::windowsVersion() < QSysInfo::WV_WINDOWS7)
        return getLocaleInfo(LOCALE_SNATIVELANGNAME);
    return getLocaleInfo(LOCALE_SNATIVELANGUAGENAME);
}

QVariant QSystemLocalePrivate::nativeCountryName()
{
    if (QSysInfo::windowsVersion() < QSysInfo::WV_WINDOWS7)
        return getLocaleInfo(LOCALE_SNATIVECTRYNAME);
    return getLocaleInfo(LOCALE_SNATIVECOUNTRYNAME);
}


void QSystemLocalePrivate::update()
{
    lcid = GetUserDefaultLCID();
    substitutionType = SUnknown;
    zero = QChar();
}

QString QSystemLocalePrivate::winToQtFormat(const QString &sys_fmt)
{
    QString result;
    int i = 0;

    while (i < sys_fmt.size()) {
        if (sys_fmt.at(i).unicode() == QLatin1Char('\'')) {
            QString text = qt_readEscapedFormatString(sys_fmt, &i);
            if (text == QLatin1String("'"))
                result += QLatin1String("''");
            else
                result += QString(QLatin1Char('\'') + text + QLatin1Char('\''));
            continue;
        }

        QChar c = sys_fmt.at(i);
        int repeat = qt_repeatCount(sys_fmt, i);

        switch (c.unicode()) {
            // Date
            case 'y':
                if (repeat > 5)
                    repeat = 5;
                else if (repeat == 3)
                    repeat = 2;
                switch (repeat) {
                    case 1:
                        result += QLatin1String("yy"); // "y" unsupported by Qt, use "yy"
                        break;
                    case 5:
                        result += QLatin1String("yyyy"); // "yyyyy" same as "yyyy" on Windows
                        break;
                    default:
                        result += QString(repeat, QLatin1Char('y'));
                        break;
                }
                break;
            case 'g':
                if (repeat > 2)
                    repeat = 2;
                switch (repeat) {
                    case 2:
                        break; // no equivalent of "gg" in Qt
                    default:
                        result += QLatin1Char('g');
                        break;
                }
                break;
            case 't':
                if (repeat > 2)
                    repeat = 2;
                result += QLatin1String("AP"); // "t" unsupported, use "AP"
                break;
            default:
                result += QString(repeat, c);
                break;
        }

        i += repeat;
    }

    return result;
}

QLocale QSystemLocale::fallbackLocale() const
{
    return QLocale(QString::fromLatin1(getWinLocaleName()));
}

QVariant QSystemLocale::query(QueryType type, QVariant in = QVariant()) const
{
    QSystemLocalePrivate *d = systemLocalePrivate();
    switch(type) {
    case DecimalPoint:
        return d->decimalPoint();
    case GroupSeparator:
        return d->groupSeparator();
    case NegativeSign:
        return d->negativeSign();
    case PositiveSign:
        return d->positiveSign();
    case DateFormatLong:
        return d->dateFormat(QLocale::LongFormat);
    case DateFormatShort:
        return d->dateFormat(QLocale::ShortFormat);
    case TimeFormatLong:
        return d->timeFormat(QLocale::LongFormat);
    case TimeFormatShort:
        return d->timeFormat(QLocale::ShortFormat);
    case DateTimeFormatLong:
        return d->dateTimeFormat(QLocale::LongFormat);
    case DateTimeFormatShort:
        return d->dateTimeFormat(QLocale::ShortFormat);
    case DayNameLong:
        return d->dayName(in.toInt(), QLocale::LongFormat);
    case DayNameShort:
        return d->dayName(in.toInt(), QLocale::ShortFormat);
    case MonthNameLong:
        return d->monthName(in.toInt(), QLocale::LongFormat);
    case MonthNameShort:
        return d->monthName(in.toInt(), QLocale::ShortFormat);
    case DateToStringShort:
        return d->toString(in.toDate(), QLocale::ShortFormat);
    case DateToStringLong:
        return d->toString(in.toDate(), QLocale::LongFormat);
    case TimeToStringShort:
        return d->toString(in.toTime(), QLocale::ShortFormat);
    case TimeToStringLong:
        return d->toString(in.toTime(), QLocale::LongFormat);
    case DateTimeToStringShort:
        return d->toString(in.toDateTime(), QLocale::ShortFormat);
    case DateTimeToStringLong:
        return d->toString(in.toDateTime(), QLocale::LongFormat);
    case ZeroDigit:
        return d->zeroDigit();
    case LanguageId:
    case CountryId: {
        QString locale = QString::fromLatin1(getWinLocaleName());
        QLocale::Language lang;
        QLocale::Script script;
        QLocale::Country cntry;
        QLocalePrivate::getLangAndCountry(locale, lang, script, cntry);
        if (type == LanguageId)
            return lang;
        if (cntry == QLocale::AnyCountry)
            return fallbackLocale().country();
        return cntry;
    }
    case ScriptId:
        return QVariant(QLocale::AnyScript);
    case MeasurementSystem:
        return d->measurementSystem();
    case AMText:
        return d->amText();
    case PMText:
        return d->pmText();
    case FirstDayOfWeek:
        return d->firstDayOfWeek();
    case CurrencySymbol:
        return d->currencySymbol(QLocale::CurrencySymbolFormat(in.toUInt()));
    case CurrencyToString:
        return d->toCurrencyString(in.value<QSystemLocale::CurrencyToStringArgument>());
    case UILanguages:
        return d->uiLanguages();
    case LocaleChanged:
        d->update();
        break;
    case NativeLanguageName:
        return d->nativeLanguageName();
    case NativeCountryName:
        return d->nativeCountryName();
    default:
        break;
    }
    return QVariant();
}
#endif // QT_NO_SYSTEMLOCALE

struct WindowsToISOListElt {
    ushort windows_code;
    char iso_name[6];
};

/* NOTE: This array should be sorted by the first column! */
static const WindowsToISOListElt windows_to_iso_list[] = {
    { 0x0401, "ar_SA" },
    { 0x0402, "bg\0  " },
    { 0x0403, "ca\0  " },
    { 0x0404, "zh_TW" },
    { 0x0405, "cs\0  " },
    { 0x0406, "da\0  " },
    { 0x0407, "de\0  " },
    { 0x0408, "el\0  " },
    { 0x0409, "en_US" },
    { 0x040a, "es\0  " },
    { 0x040b, "fi\0  " },
    { 0x040c, "fr\0  " },
    { 0x040d, "he\0  " },
    { 0x040e, "hu\0  " },
    { 0x040f, "is\0  " },
    { 0x0410, "it\0  " },
    { 0x0411, "ja\0  " },
    { 0x0412, "ko\0  " },
    { 0x0413, "nl\0  " },
    { 0x0414, "no\0  " },
    { 0x0415, "pl\0  " },
    { 0x0416, "pt_BR" },
    { 0x0418, "ro\0  " },
    { 0x0419, "ru\0  " },
    { 0x041a, "hr\0  " },
    { 0x041c, "sq\0  " },
    { 0x041d, "sv\0  " },
    { 0x041e, "th\0  " },
    { 0x041f, "tr\0  " },
    { 0x0420, "ur\0  " },
    { 0x0421, "in\0  " },
    { 0x0422, "uk\0  " },
    { 0x0423, "be\0  " },
    { 0x0425, "et\0  " },
    { 0x0426, "lv\0  " },
    { 0x0427, "lt\0  " },
    { 0x0429, "fa\0  " },
    { 0x042a, "vi\0  " },
    { 0x042d, "eu\0  " },
    { 0x042f, "mk\0  " },
    { 0x0436, "af\0  " },
    { 0x0438, "fo\0  " },
    { 0x0439, "hi\0  " },
    { 0x043e, "ms\0  " },
    { 0x0458, "mt\0  " },
    { 0x0801, "ar_IQ" },
    { 0x0804, "zh_CN" },
    { 0x0807, "de_CH" },
    { 0x0809, "en_GB" },
    { 0x080a, "es_MX" },
    { 0x080c, "fr_BE" },
    { 0x0810, "it_CH" },
    { 0x0812, "ko\0  " },
    { 0x0813, "nl_BE" },
    { 0x0814, "no\0  " },
    { 0x0816, "pt\0  " },
    { 0x081a, "sr\0  " },
    { 0x081d, "sv_FI" },
    { 0x0c01, "ar_EG" },
    { 0x0c04, "zh_HK" },
    { 0x0c07, "de_AT" },
    { 0x0c09, "en_AU" },
    { 0x0c0a, "es\0  " },
    { 0x0c0c, "fr_CA" },
    { 0x0c1a, "sr\0  " },
    { 0x1001, "ar_LY" },
    { 0x1004, "zh_SG" },
    { 0x1007, "de_LU" },
    { 0x1009, "en_CA" },
    { 0x100a, "es_GT" },
    { 0x100c, "fr_CH" },
    { 0x1401, "ar_DZ" },
    { 0x1407, "de_LI" },
    { 0x1409, "en_NZ" },
    { 0x140a, "es_CR" },
    { 0x140c, "fr_LU" },
    { 0x1801, "ar_MA" },
    { 0x1809, "en_IE" },
    { 0x180a, "es_PA" },
    { 0x1c01, "ar_TN" },
    { 0x1c09, "en_ZA" },
    { 0x1c0a, "es_DO" },
    { 0x2001, "ar_OM" },
    { 0x2009, "en_JM" },
    { 0x200a, "es_VE" },
    { 0x2401, "ar_YE" },
    { 0x2409, "en\0  " },
    { 0x240a, "es_CO" },
    { 0x2801, "ar_SY" },
    { 0x2809, "en_BZ" },
    { 0x280a, "es_PE" },
    { 0x2c01, "ar_JO" },
    { 0x2c09, "en_TT" },
    { 0x2c0a, "es_AR" },
    { 0x3001, "ar_LB" },
    { 0x300a, "es_EC" },
    { 0x3401, "ar_KW" },
    { 0x340a, "es_CL" },
    { 0x3801, "ar_AE" },
    { 0x380a, "es_UY" },
    { 0x3c01, "ar_BH" },
    { 0x3c0a, "es_PY" },
    { 0x4001, "ar_QA" },
    { 0x400a, "es_BO" },
    { 0x440a, "es_SV" },
    { 0x480a, "es_HN" },
    { 0x4c0a, "es_NI" },
    { 0x500a, "es_PR" }
};

static const int windows_to_iso_count
    = sizeof(windows_to_iso_list)/sizeof(WindowsToISOListElt);

static const char *winLangCodeToIsoName(int code)
{
    int cmp = code - windows_to_iso_list[0].windows_code;
    if (cmp < 0)
        return 0;

    if (cmp == 0)
        return windows_to_iso_list[0].iso_name;

    int begin = 0;
    int end = windows_to_iso_count;

    while (end - begin > 1) {
        uint mid = (begin + end)/2;

        const WindowsToISOListElt *elt = windows_to_iso_list + mid;
        int cmp = code - elt->windows_code;
        if (cmp < 0)
            end = mid;
        else if (cmp > 0)
            begin = mid;
        else
            return elt->iso_name;
    }

    return 0;

}

static QString winIso639LangName(LCID id)
{
    QString result;

    // Windows returns the wrong ISO639 for some languages, we need to detect them here using
    // the language code
    QString lang_code;
    wchar_t out[256];
    if (GetLocaleInfo(id, LOCALE_ILANGUAGE, out, 255)) // ### shouldn't use them according to msdn
        lang_code = QString::fromWCharArray(out);

    if (!lang_code.isEmpty()) {
        const char *endptr;
        bool ok;
        QByteArray latin1_lang_code = lang_code.toLatin1();
        int i = qstrtoull(latin1_lang_code, &endptr, 16, &ok);
        if (ok && *endptr == '\0') {
            switch (i) {
                case 0x814:
                    result = QLatin1String("nn"); // Nynorsk
                    break;
                default:
                    break;
            }
        }
    }

    if (!result.isEmpty())
        return result;

    // not one of the problematic languages - do the usual lookup
    if (GetLocaleInfo(id, LOCALE_SISO639LANGNAME , out, 255))
        result = QString::fromWCharArray(out);

    return result;
}

static QString winIso3116CtryName(LCID id)
{
    QString result;

    wchar_t out[256];
    if (GetLocaleInfo(id, LOCALE_SISO3166CTRYNAME, out, 255))
        result = QString::fromWCharArray(out);

    return result;
}

static QByteArray getWinLocaleName(LCID id)
{
    QByteArray result;
    if (id == LOCALE_USER_DEFAULT) {
        static QByteArray langEnvVar = qgetenv("LANG");
        result = langEnvVar;
        QString lang, script, cntry;
        if ( result == "C" || (!result.isEmpty()
                && qt_splitLocaleName(QString::fromLocal8Bit(result), lang, script, cntry)) ) {
            long id = 0;
            bool ok = false;
            id = qstrtoll(result.data(), 0, 0, &ok);
            if ( !ok || id == 0 || id < INT_MIN || id > INT_MAX )
                return result;
            else
                return winLangCodeToIsoName( (int)id );
        }
    }

#if defined(Q_OS_WINCE)
    result = winLangCodeToIsoName(id != LOCALE_USER_DEFAULT ? id : GetUserDefaultLCID());
#else
    if (id == LOCALE_USER_DEFAULT)
        id = GetUserDefaultLCID();
    QString resultuage = winIso639LangName(id);
    QString country = winIso3116CtryName(id);
    result = resultuage.toLatin1();
    if (!country.isEmpty()) {
        result += '_';
        result += country.toLatin1();
    }
#endif

    return result;
}

Q_CORE_EXPORT QLocale qt_localeFromLCID(LCID id)
{
    return QLocale(QString::fromLatin1(getWinLocaleName(id)));
}

QT_END_NAMESPACE
