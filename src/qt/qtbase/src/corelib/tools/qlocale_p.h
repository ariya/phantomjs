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

#ifndef QLOCALE_P_H
#define QLOCALE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of internal files.  This header file may change from version to version
// without notice, or even be removed.
//
// We mean it.
//

#include "QtCore/qstring.h"
#include "QtCore/qvarlengtharray.h"
#include "QtCore/qvariant.h"
#include "QtCore/qnumeric.h"

#include "qlocale.h"

#include <limits>
#include <cmath>

QT_BEGIN_NAMESPACE

#ifndef QT_NO_SYSTEMLOCALE
class Q_CORE_EXPORT QSystemLocale
{
public:
    QSystemLocale();
    virtual ~QSystemLocale();

    struct CurrencyToStringArgument
    {
        CurrencyToStringArgument() { }
        CurrencyToStringArgument(const QVariant &v, const QString &s)
            : value(v), symbol(s) { }
        QVariant value;
        QString symbol;
    };

    enum QueryType {
        LanguageId, // uint
        CountryId, // uint
        DecimalPoint, // QString
        GroupSeparator, // QString
        ZeroDigit, // QString
        NegativeSign, // QString
        DateFormatLong, // QString
        DateFormatShort, // QString
        TimeFormatLong, // QString
        TimeFormatShort, // QString
        DayNameLong, // QString, in: int
        DayNameShort, // QString, in: int
        MonthNameLong, // QString, in: int
        MonthNameShort, // QString, in: int
        DateToStringLong, // QString, in: QDate
        DateToStringShort, // QString in: QDate
        TimeToStringLong, // QString in: QTime
        TimeToStringShort, // QString in: QTime
        DateTimeFormatLong, // QString
        DateTimeFormatShort, // QString
        DateTimeToStringLong, // QString in: QDateTime
        DateTimeToStringShort, // QString in: QDateTime
        MeasurementSystem, // uint
        PositiveSign, // QString
        AMText, // QString
        PMText, // QString
        FirstDayOfWeek, // Qt::DayOfWeek
        Weekdays, // QList<Qt::DayOfWeek>
        CurrencySymbol, // QString in: CurrencyToStringArgument
        CurrencyToString, // QString in: qlonglong, qulonglong or double
        UILanguages, // QStringList
        StringToStandardQuotation, // QString in: QStringRef to quote
        StringToAlternateQuotation, // QString in: QStringRef to quote
        ScriptId, // uint
        ListToSeparatedString, // QString
        LocaleChanged, // system locale changed
        NativeLanguageName, // QString
        NativeCountryName, // QString
        StandaloneMonthNameLong, // QString, in: int
        StandaloneMonthNameShort // QString, in: int
    };
    virtual QVariant query(QueryType type, QVariant in) const;
    virtual QLocale fallbackUiLocale() const;

private:
    QSystemLocale(bool);
    friend class QSystemLocaleSingleton;
};
#endif

#ifdef QT_USE_ICU
namespace QIcu {
    QString toUpper(const QByteArray &localeId, const QString &str, bool *ok);
    QString toLower(const QByteArray &localeId, const QString &str, bool *ok);
}
#endif


struct QLocaleId
{
    // bypass constructors
    static inline QLocaleId fromIds(ushort language, ushort script, ushort country)
    {
        const QLocaleId localeId = { language, script, country };
        return localeId;
    }

    inline bool operator==(QLocaleId other) const
    { return language_id == other.language_id && script_id == other.script_id && country_id == other.country_id; }
    inline bool operator!=(QLocaleId other) const
    { return !operator==(other); }

    QLocaleId withLikelySubtagsAdded() const;
    QLocaleId withLikelySubtagsRemoved() const;

    QByteArray name(char separator = '-') const;

    ushort language_id, script_id, country_id;
};

struct QLocaleData
{
public:
    static const QLocaleData *findLocaleData(QLocale::Language language,
                                             QLocale::Script script,
                                             QLocale::Country country);
    static const QLocaleData *c();

    enum DoubleForm {
        DFExponent = 0,
        DFDecimal,
        DFSignificantDigits,
        _DFMax = DFSignificantDigits
    };

    enum Flags {
        NoFlags             = 0,
        Alternate           = 0x01,
        ZeroPadded          = 0x02,
        LeftAdjusted        = 0x04,
        BlankBeforePositive = 0x08,
        AlwaysShowSign      = 0x10,
        ThousandsGroup      = 0x20,
        CapitalEorX         = 0x40,

        ShowBase            = 0x80,
        UppercaseBase       = 0x100,
        ForcePoint          = Alternate
    };

    enum GroupSeparatorMode {
        FailOnGroupSeparators,
        ParseGroupSeparators
    };

    enum NumberMode { IntegerMode, DoubleStandardMode, DoubleScientificMode };

    typedef QVarLengthArray<char, 256> CharBuff;

    static QString doubleToString(const QChar zero, const QChar plus,
                                  const QChar minus, const QChar exponent,
                                  const QChar group, const QChar decimal,
                                  double d, int precision,
                                  DoubleForm form,
                                  int width, unsigned flags);
    static QString longLongToString(const QChar zero, const QChar group,
                                    const QChar plus, const QChar minus,
                                    qint64 l, int precision, int base,
                                    int width, unsigned flags);
    static QString unsLongLongToString(const QChar zero, const QChar group,
                                       const QChar plus,
                                       quint64 l, int precision,
                                       int base, int width,
                                       unsigned flags);

    QString doubleToString(double d,
                           int precision = -1,
                           DoubleForm form = DFSignificantDigits,
                           int width = -1,
                           unsigned flags = NoFlags) const;
    QString longLongToString(qint64 l, int precision = -1,
                             int base = 10,
                             int width = -1,
                             unsigned flags = NoFlags) const;
    QString unsLongLongToString(quint64 l, int precision = -1,
                                int base = 10,
                                int width = -1,
                                unsigned flags = NoFlags) const;

    // this function is meant to be called with the result of stringToDouble or bytearrayToDouble
    static float convertDoubleToFloat(double d, bool *ok)
    {
        if (qIsInf(d))
            return float(d);
        if (std::fabs(d) > std::numeric_limits<float>::max()) {
            if (ok != 0)
                *ok = false;
            return 0.0f;
        }
        return float(d);
    }

    double stringToDouble(const QChar *begin, int len, bool *ok, GroupSeparatorMode group_sep_mode) const;
    qint64 stringToLongLong(const QChar *begin, int len, int base, bool *ok, GroupSeparatorMode group_sep_mode) const;
    quint64 stringToUnsLongLong(const QChar *begin, int len, int base, bool *ok, GroupSeparatorMode group_sep_mode) const;

    // these functions are used in QIntValidator (QtGui)
    Q_CORE_EXPORT static double bytearrayToDouble(const char *num, bool *ok, bool *overflow = 0);
    Q_CORE_EXPORT static qint64 bytearrayToLongLong(const char *num, int base, bool *ok, bool *overflow = 0);
    Q_CORE_EXPORT static quint64 bytearrayToUnsLongLong(const char *num, int base, bool *ok);

    bool numberToCLocale(const QChar *str, int len,
                          GroupSeparatorMode group_sep_mode,
                          CharBuff *result) const;
    inline char digitToCLocale(QChar c) const;

    // this function is used in QIntValidator (QtGui)
    Q_CORE_EXPORT bool validateChars(const QString &str, NumberMode numMode, QByteArray *buff, int decDigits = -1) const;

public:
    quint16 m_language_id, m_script_id, m_country_id;

    quint16 m_decimal, m_group, m_list, m_percent, m_zero, m_minus, m_plus, m_exponential;
    quint16 m_quotation_start, m_quotation_end;
    quint16 m_alternate_quotation_start, m_alternate_quotation_end;

    quint16 m_list_pattern_part_start_idx, m_list_pattern_part_start_size;
    quint16 m_list_pattern_part_mid_idx, m_list_pattern_part_mid_size;
    quint16 m_list_pattern_part_end_idx, m_list_pattern_part_end_size;
    quint16 m_list_pattern_part_two_idx, m_list_pattern_part_two_size;
    quint16 m_short_date_format_idx, m_short_date_format_size;
    quint16 m_long_date_format_idx, m_long_date_format_size;
    quint16 m_short_time_format_idx, m_short_time_format_size;
    quint16 m_long_time_format_idx, m_long_time_format_size;
    quint16 m_standalone_short_month_names_idx, m_standalone_short_month_names_size;
    quint16 m_standalone_long_month_names_idx, m_standalone_long_month_names_size;
    quint16 m_standalone_narrow_month_names_idx, m_standalone_narrow_month_names_size;
    quint16 m_short_month_names_idx, m_short_month_names_size;
    quint16 m_long_month_names_idx, m_long_month_names_size;
    quint16 m_narrow_month_names_idx, m_narrow_month_names_size;
    quint16 m_standalone_short_day_names_idx, m_standalone_short_day_names_size;
    quint16 m_standalone_long_day_names_idx, m_standalone_long_day_names_size;
    quint16 m_standalone_narrow_day_names_idx, m_standalone_narrow_day_names_size;
    quint16 m_short_day_names_idx, m_short_day_names_size;
    quint16 m_long_day_names_idx, m_long_day_names_size;
    quint16 m_narrow_day_names_idx, m_narrow_day_names_size;
    quint16 m_am_idx, m_am_size;
    quint16 m_pm_idx, m_pm_size;
    char    m_currency_iso_code[3];
    quint16 m_currency_symbol_idx, m_currency_symbol_size;
    quint16 m_currency_display_name_idx, m_currency_display_name_size;
    quint8  m_currency_format_idx, m_currency_format_size;
    quint8  m_currency_negative_format_idx, m_currency_negative_format_size;
    quint16 m_language_endonym_idx, m_language_endonym_size;
    quint16 m_country_endonym_idx, m_country_endonym_size;
    quint16 m_currency_digits : 2;
    quint16 m_currency_rounding : 3;
    quint16 m_first_day_of_week : 3;
    quint16 m_weekend_start : 3;
    quint16 m_weekend_end : 3;
};

class Q_CORE_EXPORT QLocalePrivate
{
public:
    static QLocalePrivate *create(const QLocaleData *data, int numberOptions = 0)
    {
        QLocalePrivate *retval = new QLocalePrivate;
        retval->m_data = data;
        retval->ref.store(0);
        retval->m_numberOptions = numberOptions;
        return retval;
    }

    QChar decimal() const { return QChar(m_data->m_decimal); }
    QChar group() const { return QChar(m_data->m_group); }
    QChar list() const { return QChar(m_data->m_list); }
    QChar percent() const { return QChar(m_data->m_percent); }
    QChar zero() const { return QChar(m_data->m_zero); }
    QChar plus() const { return QChar(m_data->m_plus); }
    QChar minus() const { return QChar(m_data->m_minus); }
    QChar exponential() const { return QChar(m_data->m_exponential); }

    quint16 languageId() const { return m_data->m_language_id; }
    quint16 countryId() const { return m_data->m_country_id; }

    QByteArray bcp47Name(char separator = '-') const;

    // ### QByteArray::fromRawData would be more optimal
    inline QString languageCode() const { return QLocalePrivate::languageToCode(QLocale::Language(m_data->m_language_id)); }
    inline QString scriptCode() const { return QLocalePrivate::scriptToCode(QLocale::Script(m_data->m_script_id)); }
    inline QString countryCode() const { return QLocalePrivate::countryToCode(QLocale::Country(m_data->m_country_id)); }

    static QString languageToCode(QLocale::Language language);
    static QString scriptToCode(QLocale::Script script);
    static QString countryToCode(QLocale::Country country);
    static QLocale::Language codeToLanguage(const QString &code);
    static QLocale::Script codeToScript(const QString &code);
    static QLocale::Country codeToCountry(const QString &code);
    static void getLangAndCountry(const QString &name, QLocale::Language &lang,
                                  QLocale::Script &script, QLocale::Country &cntry);

    QLocale::MeasurementSystem measurementSystem() const;

    static void updateSystemPrivate();

    QString dateTimeToString(const QString &format, const QDateTime &datetime,
                             const QDate &dateOnly, const QTime &timeOnly,
                             const QLocale *q) const;

    const QLocaleData *m_data;
    QBasicAtomicInt ref;
    quint16 m_numberOptions;
};

template <>
inline QLocalePrivate *QSharedDataPointer<QLocalePrivate>::clone()
{
    // cannot use QLocalePrivate's copy constructor
    // since it is deleted in C++11
    return QLocalePrivate::create(d->m_data, d->m_numberOptions);
}

inline char QLocaleData::digitToCLocale(QChar in) const
{
    const ushort tenUnicode = m_zero + 10;

    if (in.unicode() >= m_zero && in.unicode() < tenUnicode)
        return '0' + in.unicode() - m_zero;

    if (in.unicode() >= '0' && in.unicode() <= '9')
        return in.toLatin1();

    if (in == m_plus || in == QLatin1Char('+'))
        return '+';

    if (in == m_minus || in == QLatin1Char('-') || in == QChar(0x2212))
        return '-';

    if (in == m_decimal)
        return '.';

    if (in == m_group)
        return ',';

    if (in == m_exponential || in == QChar::toUpper(m_exponential))
        return 'e';

    // In several languages group() is the char 0xA0, which looks like a space.
    // People use a regular space instead of it and complain it doesn't work.
    if (m_group == 0xA0 && in.unicode() == ' ')
        return ',';

    return 0;
}

QString qt_readEscapedFormatString(const QString &format, int *idx);
bool qt_splitLocaleName(const QString &name, QString &lang, QString &script, QString &cntry);
int qt_repeatCount(const QString &s, int i);

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QStringRef)
Q_DECLARE_METATYPE(QList<Qt::DayOfWeek>)
#ifndef QT_NO_SYSTEMLOCALE
Q_DECLARE_METATYPE(QSystemLocale::CurrencyToStringArgument)
#endif

#endif // QLOCALE_P_H
