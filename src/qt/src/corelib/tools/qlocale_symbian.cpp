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

#include <QDate>
#include <QLocale>
#include <QTime>
#include <QVariant>
#include <QThread>
#include <QStringList>

#include <e32std.h>
#include <e32const.h>
#include <e32base.h>
#include <e32property.h>
#include <numberconversion.h>
#include <bacntf.h>
#include "private/qcore_symbian_p.h"
#include "private/qcoreapplication_p.h"
#include "private/qlocale_p.h"
#include <qdebug.h>

QT_BEGIN_NAMESPACE

static TExtendedLocale _s60Locale;

/*
  Definition of struct for mapping Symbian to ISO locale
*/
struct symbianToISO {
    int symbian_language;
    char iso_name[8];
    char uilanguage[8];
};


/*
  Mapping from Symbian to ISO locale.
  NOTE: This array should be sorted by the first column!
*/
static const symbianToISO symbian_to_iso_list[] = {
    { ELangEnglish,             "en_GB", "en" },    // 1
    { ELangFrench,              "fr_FR", "fr" },    // 2
    { ELangGerman,              "de_DE", "de" },    // 3
    { ELangSpanish,             "es_ES", "es" },    // 4
    { ELangItalian,             "it_IT", "it" },    // 5
    { ELangSwedish,             "sv_SE", "sv" },    // 6
    { ELangDanish,              "da_DK", "da" },    // 7
    { ELangNorwegian,           "nb_NO", "nb" },    // 8
    { ELangFinnish,             "fi_FI", "fi" },    // 9
    { ELangAmerican,            "en_US", "en-US" }, // 10
    { ELangPortuguese,          "pt_PT", "pt" },    // 13
    { ELangTurkish,             "tr_TR", "tr" },    // 14
    { ELangIcelandic,           "is_IS", "is" },    // 15
    { ELangRussian,             "ru_RU", "ru" },    // 16
    { ELangHungarian,           "hu_HU", "hu" },    // 17
    { ELangDutch,               "nl_NL", "nl" },    // 18
    { ELangCzech,               "cs_CZ", "cs" },    // 25
    { ELangSlovak,              "sk_SK", "sk" },    // 26
    { ELangPolish,              "pl_PL", "pl" },    // 27
    { ELangSlovenian,           "sl_SI", "sl" },    // 28
    { ELangTaiwanChinese,       "zh_TW", "zh-TW" }, // 29
    { ELangHongKongChinese,     "zh_HK", "zh-HK" }, // 30
    { ELangPrcChinese,          "zh_CN", "zh" },    // 31
    { ELangJapanese,            "ja_JP", "ja" },    // 32
    { ELangThai,                "th_TH", "th" },    // 33
    { ELangArabic,              "ar_AE", "ar" },    // 37
    { ELangTagalog,             "tl_PH", "tl" },    // 39
    { ELangBulgarian,           "bg_BG", "bg" },    // 42
    { ELangCatalan,             "ca_ES", "ca" },    // 44
    { ELangCroatian,            "hr_HR", "hr" },    // 45
    { ELangEstonian,            "et_EE", "et" },    // 49
    { ELangFarsi,               "fa_IR", "fa" },    // 50
    { ELangCanadianFrench,      "fr_CA", "fr-CA" }, // 51
    { ELangGreek,               "el_GR", "el" },    // 54
    { ELangHebrew,              "he_IL", "he" },    // 57
    { ELangHindi,               "hi_IN", "hi" },    // 58
    { ELangIndonesian,          "id_ID", "id" },    // 59
    { 63/*ELangKazakh*/,        "kk_KZ", "kk" },    // 63
    { ELangKorean,              "ko_KO", "ko" },    // 65
    { ELangLatvian,             "lv_LV", "lv" },    // 67
    { ELangLithuanian,          "lt_LT", "lt" },    // 68
    { ELangMalay,               "ms_MY", "ms" },    // 70
    { ELangNorwegianNynorsk,    "nn_NO", "nn" },    // 75
    { ELangBrazilianPortuguese, "pt_BR", "pt-BR" }, // 76
    { ELangRomanian,            "ro_RO", "ro" },    // 78
    { ELangSerbian,             "sr_RS", "sr" },    // 79
    { ELangLatinAmericanSpanish,"es_419", "es-419" },// 83
    { ELangUkrainian,           "uk_UA", "uk" },    // 93
    { ELangUrdu,                "ur_PK", "ur" },    // 94 - India/Pakistan
    { ELangVietnamese,          "vi_VN", "vi" },    // 96
#ifdef __E32LANG_H__
// 5.0
    { ELangBasque,              "eu_ES", "eu" },    // 102
    { ELangGalician,            "gl_ES", "gl" },    // 103
#endif
#if !defined(__SERIES60_31__)
    { ELangEnglish_Apac,        "en_GB", "en" },     // 129
    { ELangEnglish_Taiwan,      "en_TW", "en-TW" },  // 157 ### Not supported by CLDR
    { ELangEnglish_HongKong,    "en_HK", "en-HK" },  // 158
    { ELangEnglish_Prc,         "en_CN", "en-CN" },  // 159 ### Not supported by CLDR
    { ELangEnglish_Japan,       "en_JP", "en" },     // 160 ### Not supported by CLDR
    { ELangEnglish_Thailand,    "en_TH", "en" },     // 161 ### Not supported by CLDR
    { 230/*ELangEnglish_India*/,"en_IN", "en" },     // 230
    { ELangMalay_Apac,          "ms_MY", "ms" },     // 326
#endif
    { 327/*ELangIndonesian_Apac*/, "id_ID", "id" }   // 327 - appeared in Symbian^3
};

enum LocaleNameType {
    ISO,
    UILanguage
};

QByteArray qt_resolveSymbianLocaleName(int code, LocaleNameType type)
{
    //Number of Symbian to ISO locale mappings
    static const int symbian_to_iso_count
        = sizeof(symbian_to_iso_list)/sizeof(symbianToISO);

    int cmp = code - symbian_to_iso_list[0].symbian_language;
    if (cmp < 0)
        return 0;

    if (cmp == 0) {
        if (type == ISO)
            return symbian_to_iso_list[0].iso_name;
        return symbian_to_iso_list[0].uilanguage;
    }

    int begin = 0;
    int end = symbian_to_iso_count;

    while (end - begin > 1) {
        uint mid = (begin + end)/2;

        const symbianToISO *elt = symbian_to_iso_list + mid;
        int cmp = code - elt->symbian_language;
        if (cmp < 0) {
            end = mid;
        } else if (cmp > 0) {
            begin = mid;
        } else {
            if (type == ISO)
                return elt->iso_name;
            return elt->uilanguage;
        }
    }

    return 0;
}

/*!
    Returns ISO name corresponding to the Symbian locale code \a sys_fmt.
*/
QByteArray qt_symbianLocaleName(int code)
{
    return qt_resolveSymbianLocaleName(code, ISO);
}

// Rows are: normal, abbr, nmode, nmode+abbr
// First three values on a row are used for three component date,
// while the last two are used for two component date (i.e. no year).
static const char *us_locale_dep[] = {
    "MM", "dd", "yyyy", "MM", "dd",
    "M", "d", "yy", "M", "d",
    "MMMM", "dd", "yyyy", "MMMM", "dd",
    "MMM", "d", "yy", "MMM", "d" };

static const char *eu_locale_dep[] = {
    "dd", "MM", "yyyy", "dd", "MM",
    "d", "M", "yy", "d", "M",
    "dd", "MMMM", "yyyy", "dd", "MMMM",
    "d", "MMM", "yy", "d", "MMM" };

static const char *jp_locale_dep[] = {
    "yyyy", "MM", "dd", "MM", "dd",
    "yy", "M", "d", "M", "d",
    "yyyy", "MMMM", "dd", "MMMM", "dd",
    "yy", "MMM", "d", "MMM", "d" };

// 0 = day, 1 = month, 2 = year
static const int digit_map[] = {
    1, 0, 2, 1, 0, // American
    0, 1, 2, 0, 1, // European
    2, 1, 0, 1, 0  // Japanese
};

/*!
    Returns a Qt version of the given \a sys_fmt Symbian locale format string.
*/
static QString s60ToQtFormat(const QString &sys_fmt)
{
    TLocale *locale = _s60Locale.GetLocale();

    QString result;
    QString other;
    QString qtformatchars = QString::fromLatin1("adhmsyzAHM");

    QChar c;
    int i = 0;
    bool open_escape = false;
    bool abbrev_next = false;
    bool abbrev_day = false;
    bool abbrev_month = false;
    bool abbrev_year = false;
    bool locale_indep_ordering = false;
    bool minus_mode = false;
    bool plus_mode = false;
    bool n_mode = false;
    TTimeFormat tf = locale->TimeFormat();

    while (i < sys_fmt.size()) {

        c = sys_fmt.at(i);

        // let formatting thru
        if (c.unicode() == '%') {
            // if we have gathered string, concat it
            if (!other.isEmpty()) {
                result += other;
                other.clear();
            }
            // if we have open escape, end it
            if (open_escape) {
                result += QLatin1Char('\'');
                open_escape = false;
            }

            ++i;
            if (i >= sys_fmt.size())
                break;

            c = sys_fmt.at(i);

            // process specials
            abbrev_next = c.unicode() == '*';
            plus_mode = c.unicode() == '+';
            minus_mode = c.unicode() == '-';

            if (abbrev_next || plus_mode || minus_mode) {
                ++i;
                if (i >= sys_fmt.size())
                    break;

                c = sys_fmt.at(i);

                if (plus_mode || minus_mode) {
                    // break on undefined plus/minus mode
                    if (c.unicode() != 'A' && c.unicode() != 'B')
                        break;
                }
            }

            switch (c.unicode()) {
                case 'F':
                {
                    // locale indep mode on
                    locale_indep_ordering = true;
                    break;
                }

                case '/':
                {
                    // date sep 0-3
                    ++i;
                    if (i >= sys_fmt.size())
                        break;

                    c = sys_fmt.at(i);
                    if (c.isDigit() && c.digitValue() <= 3) {
                        TChar s = locale->DateSeparator(c.digitValue());
                        TUint val = s;
                        // some indexes return zero for empty
                        if (val > 0)
                            result += QChar(val);
                    }
                    break;
                }

                case 'D':
                {
                    if (!locale_indep_ordering) {
                        if (abbrev_next)
                            abbrev_day = true;
                        break;
                    }

                    if (!abbrev_next)
                        result += QLatin1String("dd");
                    else
                        result += QLatin1Char('d');

                    break;
                }

                case 'M':
                {
                    if (!locale_indep_ordering) {
                        if (abbrev_next)
                            abbrev_month = true;
                        break;
                    }

                    if (!n_mode) {
                        if (!abbrev_next)
                            result += QLatin1String("MM");
                        else
                            result += QLatin1String("M");
                    } else {
                        if (!abbrev_next)
                            result += QLatin1String("MMMM");
                        else
                            result += QLatin1String("MMM");
                    }

                    break;
                }

                case 'N':
                {
                    n_mode = true;

                    if (!locale_indep_ordering) {
                        if (abbrev_next)
                            abbrev_month = true;
                        break;
                    }

                    if (!abbrev_next)
                        result += QLatin1String("MMMM");
                    else
                        result += QLatin1String("MMM");

                    break;
                }

                case 'Y':
                {
                    if (!locale_indep_ordering) {
                        if (abbrev_next)
                            abbrev_year = true;
                        break;
                    }

                    if (!abbrev_next)
                        result += QLatin1String("yyyy");
                    else
                        result += QLatin1String("yy");

                    break;
                }

                case 'E':
                {
                    if (!abbrev_next)
                        result += QLatin1String("dddd");
                    else
                        result += QLatin1String("ddd");

                    break;
                }

                case ':':
                {
                    // timesep 0-3
                    ++i;
                    if (i >= sys_fmt.size())
                        break;

                    c = sys_fmt.at(i);
                    if (c.isDigit() && c.digitValue() <= 3) {
                        TChar s = locale->TimeSeparator(c.digitValue());
                        TUint val = s;
                        // some indexes return zero for empty
                        if (val > 0)
                            result += QChar(val);
                    }

                    break;
                }

                case 'J':
                {
                    if (tf == ETime24 && !abbrev_next)
                        result += QLatin1String("hh");
                    else
                        result += QLatin1Char('h');

                    break;
                }

                case 'H':
                {
                    if (!abbrev_next)
                        result += QLatin1String("hh");
                    else
                        result += QLatin1Char('h');

                    break;
                }

                case 'I':
                {
                    result += QLatin1Char('h');
                    break;
                }

                case 'T':
                {
                    if (!abbrev_next)
                        result += QLatin1String("mm");
                    else
                        result += QLatin1Char('m');

                    break;
                }

                case 'S':
                {
                    if (!abbrev_next)
                        result += QLatin1String("ss");
                    else
                        result += QLatin1Char('s');

                    break;
                }

                case 'B':
                {
                    // only done for 12h clock
                    if (tf == ETime24)
                        break;
                }

                    // fallthru to A
                case 'A': {
                    // quickie to get capitalization, can't use s60 string as is because Qt 'hh' format's am/pm logic
                    TAmPmName ampm = TAmPmName();
                    TChar first(ampm[0]);
                    QString qtampm = QString::fromLatin1(first.IsUpper() ? "AP" : "ap");

                    int pos = locale->AmPmSymbolPosition();

                    if ((minus_mode && pos != ELocaleBefore) ||
                        (plus_mode && pos != ELocaleAfter))
                        break;

                    if (!abbrev_next && locale->AmPmSpaceBetween()) {
                        if (pos == ELocaleBefore)
                            qtampm.append(QLatin1Char(' '));
                        else
                            qtampm.prepend(QLatin1Char(' '));
                    }

                    result += qtampm;
                    }
                    break;

                case '.': {
                        // decimal sep
                        TChar s = locale->DecimalSeparator();
                        TUint val = s;
                        if (val > 0)
                            result += QChar(val);
                    }
                    break;

                case 'C':
                {
                    // six digits in s60, three digits in qt
                    if (!abbrev_next) {
                        result += QLatin1String("zzz");
                    } else {
                        // next char is number from 0-6, how many digits to display
                        ++i;
                        if (i >= sys_fmt.size())
                            break;

                        c = sys_fmt.at(i);

                        if (c.isDigit()) {
                            // try to match wanted digits
                            QChar val(c.digitValue());

                            if (val >= 3) {
                                result += QLatin1String("zzz");
                            } else if (val > 0) {
                                result += QLatin1Char('z');
                            }
                        }
                    }
                    break;
                }

                // these cases fallthru
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                {

                    // shouldn't parse these with %F
                    if (locale_indep_ordering)
                        break;

                    TDateFormat df = locale->DateFormat();

                    const char **locale_dep;
                    switch (df) {
                        default:
                            df = EDateAmerican;
                            // fallthru to american
                        case EDateAmerican:
                            locale_dep = us_locale_dep;
                            break;
                        case EDateEuropean:
                            locale_dep = eu_locale_dep;
                            break;
                        case EDateJapanese:
                            locale_dep = jp_locale_dep;
                            break;
                    }
                    int offset = 0;
                    int adjustedDigit = c.digitValue() - 1;

                    bool abbrev_this = abbrev_next;
                    // If abbreviation specified for this digit, use that.
                    // Otherwise abbreviate according to %D, %M, and %Y specified previously.
                    if (!abbrev_this) {
                        switch (digit_map[adjustedDigit + (static_cast<int>(df) * 5)]) {
                            case 0:
                                abbrev_this = abbrev_day;
                                break;
                            case 1:
                                abbrev_this = abbrev_month;
                                break;
                            case 2:
                                abbrev_this = abbrev_year;
                                break;
                            default:
                                break; // never happens
                        }
                    }

                    if (abbrev_this)
                        offset += 5;
                    if (n_mode)
                        offset += 10;

                    result += QLatin1String(locale_dep[offset + (adjustedDigit)]);
                    break;
                }

                case '%': // fallthru percent
                {
                // any junk gets copied as is
                }
                default:
                {
                    result += c;
                    break;
                }

                case 'Z': // Qt doesn't support these :(
                case 'X':
                case 'W':
                {
                    break;
                }
            }
        } else {
            // double any single quotes, don't begin escape
            if (c.unicode() == '\'') {
                // end open escape
                if (open_escape) {
                    result += other;
                    other.clear();
                    result += QLatin1Char('\'');
                    open_escape = false;
                }

                other += c;
            }

            // gather chars and escape them in one go if any format chars are found
            if (!open_escape && qtformatchars.indexOf(c) != -1) {
                result += QLatin1Char('\'');
                open_escape = true;
            }
            other += c;
        }

        ++i;
    }

    if (!other.isEmpty())
        result += other;
    if (open_escape)
        result += QLatin1Char('\'');

    return result;
}

/*!
    Retrieves Symbian locale decimal separator.
*/
static QString symbianDecimalPoint()
{
    TLocale *locale = _s60Locale.GetLocale();

    TChar decPoint = locale->DecimalSeparator();
    int val = decPoint;
    return QChar(val);
}

/*!
    Retrieves Symbian locale group separator.
*/
static QString symbianGroupSeparator()
{
    TLocale *locale = _s60Locale.GetLocale();

    TChar grpSep = locale->ThousandsSeparator();
    int val = grpSep;
    return QChar(val);
}

/*!
    Retrieves Symbian locale zero digit.
*/
static QString symbianZeroDigit()
{
    TLocale *locale = _s60Locale.GetLocale();

    // TDigitType enumeration value returned by TLocale
    // will always correspond to zero digit unicode value.
    TDigitType digit = locale->DigitType();
    return QChar(digit);
}

/*!
    Retrieves a day name from Symbian locale. The \a day is an integer
    from 1 to 7. When \a short_format is true the method returns
    the day in short format. Otherwise it returns the day in a long format.
*/
static QString symbianDayName(int day, bool short_format)
{
    day -= 1;

    if (day < 0 || day > 6)
        return QString();

    if (short_format) {
        return qt_TDes2QString(TDayNameAbb(TDay(day)));
    } else {
        return qt_TDes2QString(TDayName(TDay(day)));
    }
}

/*!
    Retrieves a month name from Symbian locale. The \a month is an integer
    from 1 to 12. When \a short_format is true the method returns
    the month in short format. Otherwise it returns the month in a long format.
*/
static QString symbianMonthName(int month, bool short_format)
{
    month -= 1;
    if (month < 0 || month > 11)
        return QString();

    if (short_format) {
        return qt_TDes2QString(TMonthNameAbb(TMonth(month)));
    } else {
        return qt_TDes2QString(TMonthName(TMonth(month)));
    }
}

/*!
    Retrieves date format from Symbian locale and
    transforms it to Qt format.

    When \a short_format is true the method returns
    short date format. Otherwise it returns the long format.
*/
static QString symbianDateFormat(bool short_format)
{
    TPtrC dateFormat;

    if (short_format) {
        dateFormat.Set(_s60Locale.GetShortDateFormatSpec());
    } else {
        dateFormat.Set(_s60Locale.GetLongDateFormatSpec());
    }

    return s60ToQtFormat(qt_TDesC2QString(dateFormat));
}

/*!
    Retrieves time format from Symbian locale and
    transforms it to Qt format.
*/
static QString symbianTimeFormat()
{
    return s60ToQtFormat(qt_TDesC2QString(_s60Locale.GetTimeFormatSpec()));
}

/*!
    Returns localized string representation of given \a date
    formatted with Symbian locale date format.

    If \a short_format is true the format will be a short version.
    Otherwise it uses a longer version.
*/
static QString symbianDateToString(const QDate &date, bool short_format)
{
    int month = date.month() - 1;
    int day = date.day() - 1;
    int year = date.year();

    TDateTime dateTime;
    dateTime.Set(year, TMonth(month), day, 0, 0, 0, 0);

    TTime timeStr(dateTime);
    TBuf<KMaxLongDateFormatSpec*2> buffer;

    TPtrC dateFormat;
    if (short_format) {
        dateFormat.Set(_s60Locale.GetShortDateFormatSpec());
    } else {
        dateFormat.Set(_s60Locale.GetLongDateFormatSpec());
    }

    TLocale *formatLocale = _s60Locale.GetLocale();
    TRAPD(err, timeStr.FormatL(buffer, dateFormat, *formatLocale);)

    if (err == KErrNone) {
        NumberConversion::ConvertDigits(buffer, formatLocale->DigitType());
        return qt_TDes2QString(buffer);
    } else {
        return QString();
    }
}

/*!
    Returns localized string representation of given \a time
    formatted with Symbian locale time format.
*/
static QString symbianTimeToString(const QTime &time)
{
    int hour = time.hour();
    int minute = time.minute();
    int second = time.second();
    int milliseconds = 0;

    TDateTime dateTime;
    dateTime.Set(0, TMonth(0), 0, hour, minute, second, milliseconds);

    TTime timeStr(dateTime);
    TBuf<KMaxTimeFormatSpec*2> buffer;

    TLocale *formatLocale = _s60Locale.GetLocale();
    TRAPD(err, timeStr.FormatL(buffer, _s60Locale.GetTimeFormatSpec(), *formatLocale);)

    if (err == KErrNone) {
        NumberConversion::ConvertDigits(buffer, formatLocale->DigitType());
        return qt_TDes2QString(buffer);
    } else {
        return QString();
    }
}

/*!
    Returns the measurement system stored in Symbian locale

    \sa QLocale::MeasurementSystem
*/
static QLocale::MeasurementSystem symbianMeasurementSystem()
{
    TLocale *locale = _s60Locale.GetLocale();

    TUnitsFormat unitFormat = locale->UnitsGeneral();
    if (unitFormat == EUnitsImperial)
        return QLocale::ImperialSystem;
    else
        return QLocale::MetricSystem;
}

void qt_symbianUpdateSystemPrivate()
{
    // load system data before query calls
    _s60Locale.LoadSystemSettings();
}

QLocale QSystemLocale::fallbackLocale() const
{
    TLanguage lang = User::Language();
    QString locale = QLatin1String(qt_symbianLocaleName(lang));
    return QLocale(locale);
}

static QStringList symbianUILanguages()
{
    TLanguage lang = User::Language();
    QString s = QLatin1String(qt_resolveSymbianLocaleName(lang, UILanguage));
    return QStringList(s);
}

QVariant QSystemLocale::query(QueryType type, QVariant in = QVariant()) const
{
    switch(type) {
        case DecimalPoint:
            return symbianDecimalPoint();
        case GroupSeparator:
            return symbianGroupSeparator();

        case ZeroDigit:
             return symbianZeroDigit();

        case DayNameLong:
        case DayNameShort:
            return symbianDayName(in.toInt(), (type == DayNameShort) );

        case MonthNameLong:
        case MonthNameShort:
            return symbianMonthName(in.toInt(), (type == MonthNameShort) );

        case DateFormatLong:
        case DateFormatShort:
            return symbianDateFormat( (type == DateFormatShort) );
        case TimeFormatLong:
        case TimeFormatShort:
            return symbianTimeFormat();
        case DateTimeFormatLong:
        case DateTimeFormatShort:
            return QString(symbianDateFormat( (type == DateTimeFormatShort) ) + QLatin1Char(' ') + symbianTimeFormat());
        case DateToStringShort:
        case DateToStringLong:
             return symbianDateToString(in.toDate(), (type == DateToStringShort) );
        case TimeToStringShort:
        case TimeToStringLong:
             return symbianTimeToString(in.toTime());
        case DateTimeToStringShort:
        case DateTimeToStringLong: {
                const QDateTime dt = in.toDateTime();
                return QString(symbianDateToString(dt.date(), (type == DateTimeToStringShort) )
                       + QLatin1Char(' ') + symbianTimeToString(dt.time()));
            }
        case MeasurementSystem:
            return static_cast<int>(symbianMeasurementSystem());
        case LanguageId:
        case CountryId: {
                TLanguage language = User::Language();
                QString locale = QLatin1String(qt_symbianLocaleName(language));
                QLocale::Language lang;
                QLocale::Script script;
                QLocale::Country cntry;
                QLocalePrivate::getLangAndCountry(locale, lang, script, cntry);
                if (type == LanguageId)
                    return lang;
                // few iso codes have no country and will use this
                if (cntry == QLocale::AnyCountry)
                    return fallbackLocale().country();

                return cntry;
            }
        case ScriptId:
            return QVariant(QLocale::AnyScript);
        case NegativeSign:
        case PositiveSign:
            break;
        case AMText:
            return qt_TDes2QString(TAmPmName(TAmPm(EAm)));
        case PMText:
            return qt_TDes2QString(TAmPmName(TAmPm(EPm)));
        case UILanguages:
            return QVariant(symbianUILanguages());
        default:
            break;
    }
    return QVariant();
}

#if !defined(QT_NO_SYSTEMLOCALE)
QEnvironmentChangeNotifier::QEnvironmentChangeNotifier()
{
    // Create the change notifier and install the callback function
    const TCallBack callback(&QEnvironmentChangeNotifier::localeChanged, this);
    QT_TRAP_THROWING(iChangeNotifier = CEnvironmentChangeNotifier::NewL(CActive::EPriorityStandard, callback));
    iChangeNotifier->Start();
}

TInt QEnvironmentChangeNotifier::localeChanged(TAny *data)
{
    QEnvironmentChangeNotifier *that = reinterpret_cast<QEnvironmentChangeNotifier *>(data);

    TInt flag = that->iChangeNotifier->Change();
    if (flag & EChangesLocale) {
        static bool first = true;
        if (!first) { // skip the first notification on app startup
            QT_TRYCATCH_LEAVING(QLocalePrivate::updateSystemPrivate());
            QT_TRYCATCH_LEAVING(QCoreApplication::postEvent(qApp, new QEvent(QEvent::LocaleChange)));
        }
        first = false;
    }
    return KErrNone;
}

QEnvironmentChangeNotifier::~QEnvironmentChangeNotifier()
{
    delete iChangeNotifier;
}
#endif

QT_END_NAMESPACE
