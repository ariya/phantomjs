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
#include "QtCore/qmetatype.h"

#include "qlocale.h"

#if defined(Q_OS_SYMBIAN) && !defined(QT_NO_SYSTEMLOCALE)
class CEnvironmentChangeNotifier;
#endif

QT_BEGIN_NAMESPACE

struct Q_CORE_EXPORT QLocalePrivate
{
public:
    QChar decimal() const { return QChar(m_decimal); }
    QChar group() const { return QChar(m_group); }
    QChar list() const { return QChar(m_list); }
    QChar percent() const { return QChar(m_percent); }
    QChar zero() const { return QChar(m_zero); }
    QChar plus() const { return QChar(m_plus); }
    QChar minus() const { return QChar(m_minus); }
    QChar exponential() const { return QChar(m_exponential); }

    quint16 languageId() const { return m_language_id; }
    quint16 countryId() const { return m_country_id; }

    QString bcp47Name() const;

    QString languageCode() const; // ### QByteArray::fromRawData would be more optimal
    QString scriptCode() const;
    QString countryCode() const;

    static QLocale::Language codeToLanguage(const QString &code);
    static QLocale::Script codeToScript(const QString &code);
    static QLocale::Country codeToCountry(const QString &code);
    static void getLangAndCountry(const QString &name, QLocale::Language &lang,
                                  QLocale::Script &script, QLocale::Country &cntry);
    static const QLocalePrivate *findLocale(QLocale::Language language,
                                            QLocale::Script script,
                                            QLocale::Country country);


    QLocale::MeasurementSystem measurementSystem() const;

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
    double stringToDouble(const QString &num, bool *ok, GroupSeparatorMode group_sep_mode) const;
    qint64 stringToLongLong(const QString &num, int base, bool *ok, GroupSeparatorMode group_sep_mode) const;
    quint64 stringToUnsLongLong(const QString &num, int base, bool *ok, GroupSeparatorMode group_sep_mode) const;


    static double bytearrayToDouble(const char *num, bool *ok, bool *overflow = 0);
    static qint64 bytearrayToLongLong(const char *num, int base, bool *ok, bool *overflow = 0);
    static quint64 bytearrayToUnsLongLong(const char *num, int base, bool *ok);

    typedef QVarLengthArray<char, 256> CharBuff;
    bool numberToCLocale(const QString &num,
    	    	    	  GroupSeparatorMode group_sep_mode,
                          CharBuff *result) const;
    inline char digitToCLocale(const QChar &c) const;

    static void updateSystemPrivate();

    enum NumberMode { IntegerMode, DoubleStandardMode, DoubleScientificMode };
    bool validateChars(const QString &str, NumberMode numMode, QByteArray *buff, int decDigits = -1) const;

    QString dateTimeToString(const QString &format, const QDate *date, const QTime *time,
                             const QLocale *q) const;

    quint16 m_language_id, m_script_id, m_country_id;

    quint16 m_decimal, m_group, m_list, m_percent,
        m_zero, m_minus, m_plus, m_exponential;
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
    char m_currency_iso_code[3];
    quint16 m_currency_symbol_idx, m_currency_symbol_size;
    quint16 m_currency_display_name_idx, m_currency_display_name_size;
    quint8 m_currency_format_idx, m_currency_format_size;
    quint8 m_currency_negative_format_idx, m_currency_negative_format_size;
    quint16 m_language_endonym_idx, m_language_endonym_size;
    quint16 m_country_endonym_idx, m_country_endonym_size;
    quint16 m_currency_digits : 2;
    quint16 m_currency_rounding : 3;
    quint16 m_first_day_of_week : 3;
    quint16 m_weekend_start : 3;
    quint16 m_weekend_end : 3;

};

inline char QLocalePrivate::digitToCLocale(const QChar &in) const
{
    const QChar _zero = zero();
    const QChar _group = group();
    const ushort zeroUnicode = _zero.unicode();
    const ushort tenUnicode = zeroUnicode + 10;

    if (in.unicode() >= zeroUnicode && in.unicode() < tenUnicode)
        return '0' + in.unicode() - zeroUnicode;

    if (in.unicode() >= '0' && in.unicode() <= '9')
        return in.toLatin1();

    if (in == plus())
        return '+';

    if (in == minus())
        return '-';

    if (in == decimal())
        return '.';

    if (in == group())
        return ',';

    if (in == exponential() || in == exponential().toUpper())
        return 'e';

    // In several languages group() is the char 0xA0, which looks like a space.
    // People use a regular space instead of it and complain it doesn't work.
    if (_group.unicode() == 0xA0 && in.unicode() == ' ')
        return ',';

    return 0;
}

#if defined(Q_OS_SYMBIAN) && !defined(QT_NO_SYSTEMLOCALE)
class QEnvironmentChangeNotifier
{
public:
    QEnvironmentChangeNotifier();
    ~QEnvironmentChangeNotifier();

    static TInt localeChanged(TAny *data);

private:
    CEnvironmentChangeNotifier *iChangeNotifier;
};
#endif

QString qt_readEscapedFormatString(const QString &format, int *idx);
bool qt_splitLocaleName(const QString &name, QString &lang, QString &script, QString &cntry);
int qt_repeatCount(const QString &s, int i);

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QStringRef)
Q_DECLARE_METATYPE(QList<Qt::DayOfWeek>)

#endif // QLOCALE_P_H
