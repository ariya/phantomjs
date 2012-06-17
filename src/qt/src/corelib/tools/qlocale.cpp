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

#include "qglobal.h"

#ifndef QT_NO_SYSTEMLOCALE
QT_BEGIN_NAMESPACE
class QSystemLocale;
static QSystemLocale *QSystemLocale_globalSystemLocale();
QT_END_NAMESPACE
#endif

#if !defined(QWS) && defined(Q_OS_MAC)
#   include "private/qcore_mac_p.h"
#   include <CoreFoundation/CoreFoundation.h>
#endif

#include "qplatformdefs.h"

#include "qdatastream.h"
#include "qstring.h"
#include "qlocale.h"
#include "qlocale_p.h"
#include "qlocale_tools_p.h"
#include "qdatetime_p.h"
#include "qnamespace.h"
#include "qdatetime.h"
#include "qstringlist.h"
#include "qvariant.h"
#include "qstringbuilder.h"
#if defined(Q_WS_WIN)
#   include "qt_windows.h"
#   include <time.h>
#endif
#include "private/qnumeric_p.h"
#include "private/qsystemlibrary_p.h"

QT_BEGIN_NAMESPACE

#if defined(Q_OS_SYMBIAN)
void qt_symbianUpdateSystemPrivate();
#endif

#ifndef QT_NO_SYSTEMLOCALE
static QSystemLocale *_systemLocale = 0;
Q_GLOBAL_STATIC_WITH_ARGS(QSystemLocale, QSystemLocale_globalSystemLocale, (true))
static QLocalePrivate *system_lp = 0;
Q_GLOBAL_STATIC(QLocalePrivate, globalLocalePrivate)
#endif

#ifdef QT_USE_ICU
extern bool qt_initIcu(const QString &localeName);
extern bool qt_u_strToUpper(const QString &str, QString *out, const QLocale &locale);
extern bool qt_u_strToLower(const QString &str, QString *out, const QLocale &locale);
#endif

/******************************************************************************
** Helpers for accessing Qt locale database
*/

QT_BEGIN_INCLUDE_NAMESPACE
#include "qlocale_data_p.h"
QT_END_INCLUDE_NAMESPACE

QLocale::Language QLocalePrivate::codeToLanguage(const QString &code)
{
    int len = code.length();
    if (len != 2 && len != 3)
        return QLocale::C;
    ushort uc1 = len-- > 0 ? code[0].toLower().unicode() : 0;
    ushort uc2 = len-- > 0 ? code[1].toLower().unicode() : 0;
    ushort uc3 = len-- > 0 ? code[2].toLower().unicode() : 0;

    if (uc1 == 'n' && uc2 == 'o' && uc3 == 0)
        uc2 = 'b';

    const unsigned char *c = language_code_list;
    for (; *c != 0; c += 3) {
        if (uc1 == c[0] && uc2 == c[1] && uc3 == c[2])
            return QLocale::Language((c - language_code_list)/3);
    }

    return QLocale::C;
}

QLocale::Script QLocalePrivate::codeToScript(const QString &code)
{
    int len = code.length();
    if (len != 4)
        return QLocale::AnyScript;

    // script is titlecased in our data
    unsigned char c0 = code.at(0).toUpper().toLatin1();
    unsigned char c1 = code.at(1).toLower().toLatin1();
    unsigned char c2 = code.at(2).toLower().toLatin1();
    unsigned char c3 = code.at(3).toLower().toLatin1();

    const unsigned char *c = script_code_list;
    for (int i = 0; i < QLocale::LastScript; ++i, c += 4) {
        if (c0 == c[0] && c1 == c[1] && c2 == c[2] && c3 == c[3])
            return QLocale::Script(i);
    }
    return QLocale::AnyScript;
}

QLocale::Country QLocalePrivate::codeToCountry(const QString &code)
{
    int len = code.length();
    if (len != 2 && len != 3)
        return QLocale::AnyCountry;
    ushort uc1 = len-- > 0 ? code[0].toUpper().unicode() : 0;
    ushort uc2 = len-- > 0 ? code[1].toUpper().unicode() : 0;
    ushort uc3 = len-- > 0 ? code[2].toUpper().unicode() : 0;

    const unsigned char *c = country_code_list;
    for (; *c != 0; c += 3) {
        if (uc1 == c[0] && uc2 == c[1] && uc3 == c[2])
            return QLocale::Country((c - country_code_list)/3);
    }

    return QLocale::AnyCountry;
}

QString QLocalePrivate::languageCode() const
{
    if (m_language_id == QLocale::AnyLanguage)
        return QString();
    if (m_language_id == QLocale::C)
        return QLatin1String("C");

    const unsigned char *c = language_code_list + 3*(uint(m_language_id));

    QString code(c[2] == 0 ? 2 : 3, Qt::Uninitialized);

    code[0] = ushort(c[0]);
    code[1] = ushort(c[1]);
    if (c[2] != 0)
        code[2] = ushort(c[2]);

    return code;
}

QString QLocalePrivate::scriptCode() const
{
    if (m_script_id == QLocale::AnyScript || m_script_id > QLocale::LastScript)
        return QString();
    const unsigned char *c = script_code_list + 4*(uint(m_script_id));
    return QString::fromLatin1((const char *)c, 4);
}

QString QLocalePrivate::countryCode() const
{
    if (m_country_id == QLocale::AnyCountry)
        return QString();

    const unsigned char *c = country_code_list + 3*(uint(m_country_id));

    QString code(c[2] == 0 ? 2 : 3, Qt::Uninitialized);

    code[0] = ushort(c[0]);
    code[1] = ushort(c[1]);
    if (c[2] != 0)
        code[2] = ushort(c[2]);

    return code;
}

QString QLocalePrivate::bcp47Name() const
{
    if (m_language_id == QLocale::AnyLanguage)
        return QString();
    if (m_language_id == QLocale::C)
        return QLatin1String("C");
    const unsigned char *lang = language_code_list + 3*(uint(m_language_id));
    const unsigned char *script =
            (m_script_id != QLocale::AnyScript ? script_code_list + 4*(uint(m_script_id)) : 0);
    const unsigned char *country =
            (m_country_id != QLocale::AnyCountry  ? country_code_list + 3*(uint(m_country_id)) : 0);
    char len = (lang[2] != 0 ? 3 : 2) + (script ? 4+1 : 0) + (country ? (country[2] != 0 ? 3 : 2)+1 : 0);
    QString name(len, Qt::Uninitialized);
    QChar *uc = name.data();
    *uc++ = ushort(lang[0]);
    *uc++ = ushort(lang[1]);
    if (lang[2] != 0)
        *uc++ = ushort(lang[2]);
    if (script) {
        *uc++ = QLatin1Char('-');
        *uc++ = ushort(script[0]);
        *uc++ = ushort(script[1]);
        *uc++ = ushort(script[2]);
        *uc++ = ushort(script[3]);
    }
    if (country) {
        *uc++ = QLatin1Char('-');
        *uc++ = ushort(country[0]);
        *uc++ = ushort(country[1]);
        if (country[2] != 0)
            *uc++ = ushort(country[2]);
    }
    return name;
}

const QLocalePrivate *QLocalePrivate::findLocale(QLocale::Language language, QLocale::Script script, QLocale::Country country)
{
    const unsigned language_id = language;
    const unsigned script_id = script;
    const unsigned country_id = country;

    uint idx = locale_index[language_id];

    const QLocalePrivate *d = locale_data + idx;

    if (idx == 0) // default language has no associated country
        return d;

    if (script == QLocale::AnyScript && country == QLocale::AnyCountry)
        return d;

    Q_ASSERT(d->languageId() == language_id);

    if (country == QLocale::AnyCountry) {
        while (d->m_language_id == language_id && d->m_script_id != script_id)
            ++d;
        if (d->m_language_id == language_id && d->m_script_id == script_id)
            return d;
    } else if (script == QLocale::AnyScript) {
        while (d->m_language_id == language_id) {
            if (d->m_script_id == script_id && d->m_country_id == country_id)
                return d;
            ++d;
        }
    } else {
        // both script and country are explicitly specified
        while (d->m_language_id == language_id) {
            if (d->m_script_id == script_id && d->m_country_id == country_id)
                return d;
            ++d;
        }
    }

    return locale_data + idx;
}

static bool parse_locale_tag(const QString &input, int &i, QString *result, const QString &separators)
{
    *result = QString(8, Qt::Uninitialized); // worst case according to BCP47
    QChar *pch = result->data();
    const QChar *uc = input.data() + i;
    const int l = input.length();
    int size = 0;
    for (; i < l && size < 8; ++i, ++size) {
        if (separators.contains(*uc))
            break;
        if (! ((uc->unicode() >= 'a' && uc->unicode() <= 'z') ||
               (uc->unicode() >= 'A' && uc->unicode() <= 'Z') ||
               (uc->unicode() >= '0' && uc->unicode() <= '9')) ) // latin only
            return false;
        *pch++ = *uc++;
    }
    result->truncate(size);
    return true;
}

bool qt_splitLocaleName(const QString &name, QString &lang, QString &script, QString &cntry)
{
    const int length = name.length();

    lang = script = cntry = QString();

    const QString separators = QLatin1String("_-.@");
    enum ParserState { NoState, LangState, ScriptState, CountryState };
    ParserState state = LangState;
    for (int i = 0; i < length && state != NoState; ) {
        QString value;
        if (!parse_locale_tag(name, i, &value, separators) ||value.isEmpty())
            break;
        QChar sep = i < length ? name.at(i) : QChar();
        switch (state) {
        case LangState:
            if (!sep.isNull() && !separators.contains(sep)) {
                state = NoState;
                break;
            }
            lang = value;
            if (i == length) {
                // just language was specified
                state = NoState;
                break;
            }
            state = ScriptState;
            break;
        case ScriptState: {
            QString scripts = QString::fromLatin1((const char *)script_code_list, sizeof(script_code_list));
            if (value.length() == 4 && scripts.indexOf(value) % 4 == 0) {
                // script name is always 4 characters
                script = value;
                state = CountryState;
            } else {
                // it wasn't a script, maybe it is a country then?
                cntry = value;
                state = NoState;
            }
            break;
        }
        case CountryState:
            cntry = value;
            state = NoState;
            break;
        case NoState:
            // shouldn't happen
            qWarning("QLocale: This should never happen");
            break;
        }
        ++i;
    }
    return lang.length() == 2 || lang.length() == 3;
}

void QLocalePrivate::getLangAndCountry(const QString &name, QLocale::Language &lang,
                                       QLocale::Script &script, QLocale::Country &cntry)
{
    lang = QLocale::C;
    script = QLocale::AnyScript;
    cntry = QLocale::AnyCountry;

    QString lang_code;
    QString script_code;
    QString cntry_code;
    if (!qt_splitLocaleName(name, lang_code, script_code, cntry_code))
        return;

    lang = QLocalePrivate::codeToLanguage(lang_code);
    if (lang == QLocale::C)
        return;
    script = QLocalePrivate::codeToScript(script_code);
    cntry = QLocalePrivate::codeToCountry(cntry_code);
}

static const QLocalePrivate *findLocale(const QString &name)
{
    QLocale::Language lang;
    QLocale::Script script;
    QLocale::Country cntry;
    QLocalePrivate::getLangAndCountry(name, lang, script, cntry);

    return QLocalePrivate::findLocale(lang, script, cntry);
}

QString qt_readEscapedFormatString(const QString &format, int *idx)
{
    int &i = *idx;

    Q_ASSERT(format.at(i) == QLatin1Char('\''));
    ++i;
    if (i == format.size())
        return QString();
    if (format.at(i).unicode() == '\'') { // "''" outside of a quoted stirng
        ++i;
        return QLatin1String("'");
    }

    QString result;

    while (i < format.size()) {
        if (format.at(i).unicode() == '\'') {
            if (i + 1 < format.size() && format.at(i + 1).unicode() == '\'') {
                // "''" inside of a quoted string
                result.append(QLatin1Char('\''));
                i += 2;
            } else {
                break;
            }
        } else {
            result.append(format.at(i++));
        }
    }
    if (i < format.size())
        ++i;

    return result;
}

int qt_repeatCount(const QString &s, int i)
{
    QChar c = s.at(i);
    int j = i + 1;
    while (j < s.size() && s.at(j) == c)
        ++j;
    return j - i;
}

static const QLocalePrivate *default_lp = 0;
static uint default_number_options = 0;

#ifndef QT_NO_SYSTEMLOCALE


/******************************************************************************
** Default system locale behavior
*/

/*!
  Constructs a QSystemLocale object. The constructor will automatically
  install this object as the system locale and remove any earlier installed
  system locales.
*/
QSystemLocale::QSystemLocale()
{
    delete _systemLocale;
    _systemLocale = this;

    if (system_lp)
        system_lp->m_language_id = 0;
}

/*! \internal */
QSystemLocale::QSystemLocale(bool)
{ }

/*!
  Deletes the object.
*/
QSystemLocale::~QSystemLocale()
{
    if (_systemLocale == this) {
        _systemLocale = 0;

        if (system_lp)
            system_lp->m_language_id = 0;
    }
}

static const QSystemLocale *systemLocale()
{
    if (_systemLocale)
        return _systemLocale;
    return QSystemLocale_globalSystemLocale();
}

void QLocalePrivate::updateSystemPrivate()
{
    const QSystemLocale *sys_locale = systemLocale();
    if (!system_lp)
        system_lp = globalLocalePrivate();

    // tell the object that the system locale has changed.
    sys_locale->query(QSystemLocale::LocaleChanged, QVariant());

    *system_lp = *sys_locale->fallbackLocale().d();

#if defined(Q_OS_SYMBIAN)
    qt_symbianUpdateSystemPrivate();
#endif

    QVariant res = sys_locale->query(QSystemLocale::LanguageId, QVariant());
    if (!res.isNull()) {
        system_lp->m_language_id = res.toInt();
        system_lp->m_script_id = QLocale::AnyScript; // default for compatibility
    }
    res = sys_locale->query(QSystemLocale::CountryId, QVariant());
    if (!res.isNull()) {
        system_lp->m_country_id = res.toInt();
        system_lp->m_script_id = QLocale::AnyScript; // default for compatibility
    }
    res = sys_locale->query(QSystemLocale::ScriptId, QVariant());
    if (!res.isNull())
        system_lp->m_script_id = res.toInt();

    res = sys_locale->query(QSystemLocale::DecimalPoint, QVariant());
    if (!res.isNull())
        system_lp->m_decimal = res.toString().at(0).unicode();

    res = sys_locale->query(QSystemLocale::GroupSeparator, QVariant());
    if (!res.isNull())
        system_lp->m_group = res.toString().at(0).unicode();

    res = sys_locale->query(QSystemLocale::ZeroDigit, QVariant());
    if (!res.isNull())
        system_lp->m_zero = res.toString().at(0).unicode();

    res = sys_locale->query(QSystemLocale::NegativeSign, QVariant());
    if (!res.isNull())
        system_lp->m_minus = res.toString().at(0).unicode();

    res = sys_locale->query(QSystemLocale::PositiveSign, QVariant());
    if (!res.isNull())
        system_lp->m_plus = res.toString().at(0).unicode();

#ifdef QT_USE_ICU
    if (!default_lp)
        qt_initIcu(system_lp->bcp47Name());
#endif

}
#endif

static const QLocalePrivate *systemPrivate()
{
#ifndef QT_NO_SYSTEMLOCALE
    // copy over the information from the fallback locale and modify
    if (!system_lp || system_lp->m_language_id == 0)
        QLocalePrivate::updateSystemPrivate();

    return system_lp;
#else
    return locale_data;
#endif
}

static const QLocalePrivate *defaultPrivate()
{
    if (!default_lp)
        default_lp = systemPrivate();
    return default_lp;
}

static QString getLocaleListData(const ushort *data, int size, int index)
{
    static const ushort separator = ';';
    while (index && size > 0) {
        while (*data != separator)
            ++data, --size;
        --index;
        ++data;
        --size;
    }
    const ushort *end = data;
    while (size > 0 && *end != separator)
        ++end, --size;
    if (end-data == 0)
        return QString();
    return QString::fromRawData(reinterpret_cast<const QChar*>(data), end-data);
}

static inline QString getLocaleData(const ushort *data, int size)
{
    return size ? QString::fromRawData(reinterpret_cast<const QChar*>(data), size) : QString();
}


#ifndef QT_NO_DATASTREAM
QDataStream &operator<<(QDataStream &ds, const QLocale &l)
{
    ds << l.name();
    return ds;
}

QDataStream &operator>>(QDataStream &ds, QLocale &l)
{
    QString s;
    ds >> s;
    l = QLocale(s);
    return ds;
}
#endif // QT_NO_DATASTREAM


static const int locale_data_size = sizeof(locale_data)/sizeof(QLocalePrivate) - 1;

static const QLocalePrivate *dataPointerHelper(quint16 index)
{
#ifndef QT_NO_SYSTEMLOCALE
    Q_ASSERT(index <= locale_data_size);
    if (index == locale_data_size)
        return system_lp;
#else
    Q_ASSERT(index < locale_data_size);
#endif

    return &locale_data[index];
}

static quint16 localePrivateIndex(const QLocalePrivate *p)
{
#ifndef QT_NO_SYSTEMLOCALE
    Q_ASSERT((p >= locale_data && p - locale_data < locale_data_size)
             || (p != 0 && p == system_lp));
    quint16 index = p == system_lp ? locale_data_size : p - locale_data;
#else
    Q_ASSERT(p >= locale_data && p - locale_data < locale_data_size);
    quint16 index = p - locale_data;
#endif

    return index;
}

/*!
    Constructs a QLocale object with the specified \a name,
    which has the format
    "language[_script][_country][.codeset][@modifier]" or "C", where:

    \list
    \i language is a lowercase, two-letter, ISO 639 language code,
    \i script is a titlecase, four-letter, ISO 15924 script code,
    \i country is an uppercase, two- or three-letter, ISO 3166 country code (also "419" as defined by United Nations),
    \i and codeset and modifier are ignored.
    \endlist

    The separator can be either underscore or a minus sign.

    If the string violates the locale format, or language is not
    a valid ISO 369 code, the "C" locale is used instead. If country
    is not present, or is not a valid ISO 3166 code, the most
    appropriate country is chosen for the specified language.

    The language, script and country codes are converted to their respective
    \c Language, \c Script and \c Country enums. After this conversion is
    performed the constructor behaves exactly like QLocale(Country, Script,
    Language).

    This constructor is much slower than QLocale(Country, Script, Language).

    \sa bcp47Name()
*/

QLocale::QLocale(const QString &name)
    : v(0)
{
    p.numberOptions = 0;
    p.index = localePrivateIndex(findLocale(name));
}

/*!
    Constructs a QLocale object initialized with the default locale. If
    no default locale was set using setDefaultLocale(), this locale will
    be the same as the one returned by system().

    \sa setDefault()
*/

QLocale::QLocale()
    : v(0)
{
    p.numberOptions = default_number_options;
    p.index = localePrivateIndex(defaultPrivate());
}

/*!
    Constructs a QLocale object with the specified \a language and \a
    country.

    \list
    \i If the language/country pair is found in the database, it is used.
    \i If the language is found but the country is not, or if the country
       is \c AnyCountry, the language is used with the most
       appropriate available country (for example, Germany for German),
    \i If neither the language nor the country are found, QLocale
       defaults to the default locale (see setDefault()).
    \endlist

    The language and country that are actually used can be queried
    using language() and country().

    \sa setDefault() language() country()
*/

QLocale::QLocale(Language language, Country country)
    : v(0)
{
    const QLocalePrivate *d = QLocalePrivate::findLocale(language, QLocale::AnyScript, country);

    // If not found, should default to system
    if (d->languageId() == QLocale::C && language != QLocale::C) {
        p.numberOptions = default_number_options;
        p.index = localePrivateIndex(defaultPrivate());
    } else {
        p.numberOptions = 0;
        p.index = localePrivateIndex(d);
    }
}
\
/*!
    \since 4.8

    Constructs a QLocale object with the specified \a language, \a script and
    \a country.

    \list
    \i If the language/script/country is found in the database, it is used.
    \i If both \a script is AnyScript and \a country is AnyCountry, the
       language is used with the most appropriate available script and country
       (for example, Germany for German),
    \i If either \a script is AnyScript or \a country is AnyCountry, the
       language is used with the first locale that matches the given \a script
       and \a country.
    \i If neither the language nor the country are found, QLocale
       defaults to the default locale (see setDefault()).
    \endlist

    The language, script and country that are actually used can be queried
    using language(), script() and country().

    \sa setDefault() language() script() country()
*/

QLocale::QLocale(Language language, Script script, Country country)
    : v(0)
{
    const QLocalePrivate *d = QLocalePrivate::findLocale(language, script, country);

    // If not found, should default to system
    if (d->languageId() == QLocale::C && language != QLocale::C) {
        p.numberOptions = default_number_options;
        p.index = localePrivateIndex(defaultPrivate());
    } else {
        p.numberOptions = 0;
        p.index = localePrivateIndex(d);
    }
}

/*!
    Constructs a QLocale object as a copy of \a other.
*/

QLocale::QLocale(const QLocale &other)
{
    v = other.v;
}

const QLocalePrivate *QLocale::d() const
{
    return dataPointerHelper(p.index);
}

/*!
    Assigns \a other to this QLocale object and returns a reference
    to this QLocale object.
*/

QLocale &QLocale::operator=(const QLocale &other)
{
    v = other.v;
    return *this;
}

/*!
    \since 4.2

    Sets the \a options related to number conversions for this
    QLocale instance.
*/
void QLocale::setNumberOptions(NumberOptions options)
{
    p.numberOptions = options;
}

/*!
    \since 4.2

    Returns the options related to number conversions for this
    QLocale instance.

    By default, no options are set for the standard locales.
*/
QLocale::NumberOptions QLocale::numberOptions() const
{
    return static_cast<NumberOption>(p.numberOptions);
}

/*!
    \since 4.8

    Returns \a str quoted according to the current locale using the given
    quotation \a style.
*/
QString QLocale::quoteString(const QString &str, QuotationStyle style) const
{
    return quoteString(&str, style);
}

/*!
    \since 4.8

    \overload
*/
QString QLocale::quoteString(const QStringRef &str, QuotationStyle style) const
{
#ifndef QT_NO_SYSTEMLOCALE
    if (d() == systemPrivate()) {
        QVariant res;
        if (style == QLocale::AlternateQuotation)
            res = systemLocale()->query(QSystemLocale::StringToAlternateQuotation, QVariant::fromValue(str));
        if (res.isNull() || style == QLocale::StandardQuotation)
            res = systemLocale()->query(QSystemLocale::StringToStandardQuotation, QVariant::fromValue(str));
        if (!res.isNull())
            return res.toString();
    }
#endif

    if (style == QLocale::StandardQuotation)
        return QChar(d()->m_quotation_start) % str % QChar(d()->m_quotation_end);
    else
        return QChar(d()->m_alternate_quotation_start) % str % QChar(d()->m_alternate_quotation_end);
}

/*!
    \since 4.8

    Returns a string that represents a join of a given \a list of strings with
    a separator defined by the locale.
*/
QString QLocale::createSeparatedList(const QStringList &list) const
{
#ifndef QT_NO_SYSTEMLOCALE
    if (d() == systemPrivate()) {
        QVariant res;
        res = systemLocale()->query(QSystemLocale::ListToSeparatedString, QVariant::fromValue(list));

        if (!res.isNull())
            return res.toString();
    }
#endif

    const int size = list.size();
    if (size == 1) {
        return list.at(0);
    } else if (size == 2) {
        QString format = getLocaleData(list_pattern_part_data + d()->m_list_pattern_part_two_idx, d()->m_list_pattern_part_two_size);
        return format.arg(list.at(0), list.at(1));
    } else if (size > 2) {
        QString formatStart = getLocaleData(list_pattern_part_data + d()->m_list_pattern_part_start_idx, d()->m_list_pattern_part_start_size);
        QString formatMid = getLocaleData(list_pattern_part_data + d()->m_list_pattern_part_mid_idx, d()->m_list_pattern_part_mid_size);
        QString formatEnd = getLocaleData(list_pattern_part_data + d()->m_list_pattern_part_end_idx, d()->m_list_pattern_part_end_size);
        QString result = formatStart.arg(list.at(0), list.at(1));
        for (int i = 2; i < size - 1; ++i)
            result = formatMid.arg(result, list.at(i));
        result = formatEnd.arg(result, list.at(size - 1));
        return result;
    }

    return QString();
}

/*!
    \nonreentrant

    Sets the global default locale to \a locale. These
    values are used when a QLocale object is constructed with
    no arguments. If this function is not called, the system's
    locale is used.

    \warning In a multithreaded application, the default locale
    should be set at application startup, before any non-GUI threads
    are created.

    \sa system() c()
*/

void QLocale::setDefault(const QLocale &locale)
{
    default_lp = locale.d();
    default_number_options = locale.numberOptions();

#ifdef QT_USE_ICU
    qt_initIcu(locale.bcp47Name());
#endif
}

/*!
    Returns the language of this locale.

    \sa script(), country(), languageToString(), bcp47Name()
*/
QLocale::Language QLocale::language() const
{
    return Language(d()->languageId());
}

/*!
    \since 4.8

    Returns the script of this locale.

    \sa language(), country(), languageToString(), scriptToString(), bcp47Name()
*/
QLocale::Script QLocale::script() const
{
    return Script(d()->m_script_id);
}

/*!
    Returns the country of this locale.

    \sa language(), script(), countryToString(), bcp47Name()
*/
QLocale::Country QLocale::country() const
{
    return Country(d()->countryId());
}

/*!
    Returns the language and country of this locale as a
    string of the form "language_country", where
    language is a lowercase, two-letter ISO 639 language code,
    and country is an uppercase, two- or three-letter ISO 3166 country code.

    Note that even if QLocale object was constructed with an explicit script,
    name() will not contain it for compatibility reasons. Use bcp47Name() instead
    if you need a full locale name.

    \sa QLocale(), language(), script(), country(), bcp47Name()
*/

QString QLocale::name() const
{
    const QLocalePrivate *dd = d();

    if (dd->m_language_id == QLocale::AnyLanguage)
        return QString();
    if (dd->m_language_id == QLocale::C)
        return QLatin1String("C");

    const unsigned char *c = language_code_list + 3*(uint(dd->m_language_id));

    QString result(7, Qt::Uninitialized);
    ushort *data = (ushort *)result.unicode();
    const ushort *begin = data;

    *data++ = ushort(c[0]);
    *data++ = ushort(c[1]);
    if (c[2] != 0)
        *data++ = ushort(c[2]);
    if (dd->m_country_id != AnyCountry) {
        *data++ = '_';
        const unsigned char *c = country_code_list + 3*(uint(dd->m_country_id));
        *data++ = ushort(c[0]);
        *data++ = ushort(c[1]);
        if (c[2] != 0)
            *data++ = ushort(c[2]);
    }
    result.resize(data - begin);

    return result;
}

/*!
    \since 4.8

    Returns the dash-separated language, script and country (and possibly other BCP47 fields)
    of this locale as a string.

    Unlike the uiLanguages() the returned value of the bcp47Name() represents
    the locale name of the QLocale data but not the language the user-interface
    should be in.

    This function tries to conform the locale name to BCP47.

    \sa language(), country(), script(), uiLanguages()
*/
QString QLocale::bcp47Name() const
{
    return d()->bcp47Name();
}

/*!
    Returns a QString containing the name of \a language.

    \sa countryToString(), scriptToString(), bcp47Name()
*/

QString QLocale::languageToString(Language language)
{
    if (uint(language) > uint(QLocale::LastLanguage))
        return QLatin1String("Unknown");
    return QLatin1String(language_name_list + language_name_index[language]);
}

/*!
    Returns a QString containing the name of \a country.

    \sa languageToString(), scriptToString(), country(), bcp47Name()
*/

QString QLocale::countryToString(Country country)
{
    if (uint(country) > uint(QLocale::LastCountry))
        return QLatin1String("Unknown");
    return QLatin1String(country_name_list + country_name_index[country]);
}

/*!
    \since 4.8

    Returns a QString containing the name of \a script.

    \sa languageToString(), countryToString(), script(), bcp47Name()
*/
QString QLocale::scriptToString(QLocale::Script script)
{
    if (uint(script) > uint(QLocale::LastScript))
        return QLatin1String("Unknown");
    return QLatin1String(script_name_list + script_name_index[script]);
}

/*!
    Returns the short int represented by the localized string \a s,
    using base \a base. If \a base is 0 the base is determined
    automatically using the following rules: If the string begins with
    "0x", it is assumed to be hexadecimal; if it begins with "0", it
    is assumed to be octal; otherwise it is assumed to be decimal.

    If the conversion fails the function returns 0.

    If \a ok is not 0, failure is reported by setting *ok to false, and
    success by setting *ok to true.

    This function ignores leading and trailing whitespace.

    \sa toUShort(), toString()
*/

short QLocale::toShort(const QString &s, bool *ok, int base) const
{
    qlonglong i = toLongLong(s, ok, base);
    if (i < SHRT_MIN || i > SHRT_MAX) {
        if (ok != 0)
            *ok = false;
        return 0;
    }
    return short(i);
}

/*!
    Returns the unsigned short int represented by the localized string
    \a s, using base \a base. If \a base is 0 the base is determined
    automatically using the following rules: If the string begins with
    "0x", it is assumed to be hexadecimal; if it begins with "0", it
    is assumed to be octal; otherwise it is assumed to be decimal.

    If the conversion fails the function returns 0.

    If \a ok is not 0, failure is reported by setting *ok to false, and
    success by setting *ok to true.

    This function ignores leading and trailing whitespace.

    \sa toShort(), toString()
*/

ushort QLocale::toUShort(const QString &s, bool *ok, int base) const
{
    qulonglong i = toULongLong(s, ok, base);
    if (i > USHRT_MAX) {
        if (ok != 0)
            *ok = false;
        return 0;
    }
    return ushort(i);
}

/*!
    Returns the int represented by the localized string \a s, using
    base \a base. If \a base is 0 the base is determined automatically
    using the following rules: If the string begins with "0x", it is
    assumed to be hexadecimal; if it begins with "0", it is assumed to
    be octal; otherwise it is assumed to be decimal.

    If the conversion fails the function returns 0.

    If \a ok is not 0, failure is reported by setting *ok to false, and
    success by setting *ok to true.

    This function ignores leading and trailing whitespace.

    \sa toUInt(), toString()
*/

int QLocale::toInt(const QString &s, bool *ok, int base) const
{
    qlonglong i = toLongLong(s, ok, base);
    if (i < INT_MIN || i > INT_MAX) {
        if (ok != 0)
            *ok = false;
        return 0;
    }
    return int(i);
}

/*!
    Returns the unsigned int represented by the localized string \a s,
    using base \a base. If \a base is 0 the base is determined
    automatically using the following rules: If the string begins with
    "0x", it is assumed to be hexadecimal; if it begins with "0", it
    is assumed to be octal; otherwise it is assumed to be decimal.

    If the conversion fails the function returns 0.

    If \a ok is not 0, failure is reported by setting *ok to false, and
    success by setting *ok to true.

    This function ignores leading and trailing whitespace.

    \sa toInt(), toString()
*/

uint QLocale::toUInt(const QString &s, bool *ok, int base) const
{
    qulonglong i = toULongLong(s, ok, base);
    if (i > UINT_MAX) {
        if (ok != 0)
            *ok = false;
        return 0;
    }
    return uint(i);
}

/*!
    Returns the long long int represented by the localized string \a
    s, using base \a base. If \a base is 0 the base is determined
    automatically using the following rules: If the string begins with
    "0x", it is assumed to be hexadecimal; if it begins with "0", it
    is assumed to be octal; otherwise it is assumed to be decimal.

    If the conversion fails the function returns 0.

    If \a ok is not 0, failure is reported by setting *ok to false, and
    success by setting *ok to true.

    This function ignores leading and trailing whitespace.

    \sa toInt(), toULongLong(), toDouble(), toString()
*/


qlonglong QLocale::toLongLong(const QString &s, bool *ok, int base) const
{
    QLocalePrivate::GroupSeparatorMode mode
        = p.numberOptions & RejectGroupSeparator
            ? QLocalePrivate::FailOnGroupSeparators
            : QLocalePrivate::ParseGroupSeparators;

    return d()->stringToLongLong(s, base, ok, mode);
}

// ### Qt5: make the return type for toULongLong() qulonglong.

/*!
    Returns the unsigned long long int represented by the localized
    string \a s, using base \a base. If \a base is 0 the base is
    determined automatically using the following rules: If the string
    begins with "0x", it is assumed to be hexadecimal; if it begins
    with "0", it is assumed to be octal; otherwise it is assumed to be
    decimal.

    If the conversion fails the function returns 0.

    If \a ok is not 0, failure is reported by setting *ok to false, and
    success by setting *ok to true.

    This function ignores leading and trailing whitespace.

    \sa toLongLong(), toInt(), toDouble(), toString()
*/

qlonglong QLocale::toULongLong(const QString &s, bool *ok, int base) const
{
    QLocalePrivate::GroupSeparatorMode mode
        = p.numberOptions & RejectGroupSeparator
            ? QLocalePrivate::FailOnGroupSeparators
            : QLocalePrivate::ParseGroupSeparators;

    return d()->stringToUnsLongLong(s, base, ok, mode);
}

/*!
    Returns the float represented by the localized string \a s, or 0.0
    if the conversion failed.

    If \a ok is not 0, reports failure by setting
    *ok to false and success by setting *ok to true.

    This function ignores leading and trailing whitespace.

    \sa toDouble(), toInt(), toString()
*/

#define QT_MAX_FLOAT 3.4028234663852886e+38

float QLocale::toFloat(const QString &s, bool *ok) const
{
    bool myOk;
    double d = toDouble(s, &myOk);
    if (!myOk || d > QT_MAX_FLOAT || d < -QT_MAX_FLOAT) {
        if (ok != 0)
            *ok = false;
        return 0.0;
    }
    if (ok != 0)
        *ok = true;
    return float(d);
}

/*!
    Returns the double represented by the localized string \a s, or
    0.0 if the conversion failed.

    If \a ok is not 0, reports failure by setting
    *ok to false and success by setting *ok to true.

    Unlike QString::toDouble(), this function does not fall back to
    the "C" locale if the string cannot be interpreted in this
    locale.

    \snippet doc/src/snippets/code/src_corelib_tools_qlocale.cpp 3

    Notice that the last conversion returns 1234.0, because '.' is the
    thousands group separator in the German locale.

    This function ignores leading and trailing whitespace.

    \sa toFloat(), toInt(), toString()
*/

double QLocale::toDouble(const QString &s, bool *ok) const
{
    QLocalePrivate::GroupSeparatorMode mode
        = p.numberOptions & RejectGroupSeparator
            ? QLocalePrivate::FailOnGroupSeparators
            : QLocalePrivate::ParseGroupSeparators;

    return d()->stringToDouble(s, ok, mode);
}

/*!
    Returns a localized string representation of \a i.

    \sa toLongLong()
*/

QString QLocale::toString(qlonglong i) const
{
    int flags = p.numberOptions & OmitGroupSeparator
                    ? 0
                    : QLocalePrivate::ThousandsGroup;

    return d()->longLongToString(i, -1, 10, -1, flags);
}

/*!
    \overload

    \sa toULongLong()
*/

QString QLocale::toString(qulonglong i) const
{
    int flags = p.numberOptions & OmitGroupSeparator
                    ? 0
                    : QLocalePrivate::ThousandsGroup;

    return d()->unsLongLongToString(i, -1, 10, -1, flags);
}

/*!
    Returns a localized string representation of the given \a date in the
    specified \a format.
    If \a format is an empty string, an empty string is returned.
*/

QString QLocale::toString(const QDate &date, const QString &format) const
{
    return d()->dateTimeToString(format, &date, 0, this);
}

/*!
    Returns a localized string representation of the given \a date according
    to the specified \a format.
*/

QString QLocale::toString(const QDate &date, FormatType format) const
{
    if (!date.isValid())
        return QString();

#ifndef QT_NO_SYSTEMLOCALE
    if (d() == systemPrivate()) {
        QVariant res = systemLocale()->query(format == LongFormat
                                             ? QSystemLocale::DateToStringLong : QSystemLocale::DateToStringShort,
                                             date);
        if (!res.isNull())
            return res.toString();
    }
#endif

    QString format_str = dateFormat(format);
    return toString(date, format_str);
}

static bool timeFormatContainsAP(const QString &format)
{
    int i = 0;
    while (i < format.size()) {
        if (format.at(i).unicode() == '\'') {
            qt_readEscapedFormatString(format, &i);
            continue;
        }

        if (format.at(i).toLower().unicode() == 'a')
            return true;

        ++i;
    }
    return false;
}

static QString timeZone()
{
#if defined(Q_OS_WINCE)
    TIME_ZONE_INFORMATION info;
    DWORD res = GetTimeZoneInformation(&info);
    if (res == TIME_ZONE_ID_UNKNOWN)
        return QString();
    return QString::fromWCharArray(info.StandardName);
#elif defined(Q_OS_WIN)
    _tzset();
# if defined(_MSC_VER) && _MSC_VER >= 1400
    size_t returnSize = 0;
    char timeZoneName[512];
    if (_get_tzname(&returnSize, timeZoneName, 512, 1))
        return QString();
    return QString::fromLocal8Bit(timeZoneName);
# else
    return QString::fromLocal8Bit(_tzname[1]);
# endif
#elif defined(Q_OS_VXWORKS)
    return QString();
#else
    tzset();
    return QString::fromLocal8Bit(tzname[1]);
#endif
}

/*!
    Returns a localized string representation of the given \a time according
    to the specified \a format.
    If \a format is an empty string, an empty string is returned.
*/
QString QLocale::toString(const QTime &time, const QString &format) const
{
    return d()->dateTimeToString(format, 0, &time, this);
}

/*!
    \since 4.4

    Returns a localized string representation of the given \a dateTime according
    to the specified \a format.
    If \a format is an empty string, an empty string is returned.
*/

QString QLocale::toString(const QDateTime &dateTime, const QString &format) const
{
    const QDate dt = dateTime.date();
    const QTime tm = dateTime.time();
    return d()->dateTimeToString(format, &dt, &tm, this);
}

/*!
    \since 4.4

    Returns a localized string representation of the given \a dateTime according
    to the specified \a format.
*/

QString QLocale::toString(const QDateTime &dateTime, FormatType format) const
{
    if (!dateTime.isValid())
        return QString();

#ifndef QT_NO_SYSTEMLOCALE
    if (d() == systemPrivate()) {
        QVariant res = systemLocale()->query(format == LongFormat
                                             ? QSystemLocale::DateTimeToStringLong
                                             : QSystemLocale::DateTimeToStringShort,
                                             dateTime);
        if (!res.isNull())
            return res.toString();
    }
#endif

    const QString format_str = dateTimeFormat(format);
    return toString(dateTime, format_str);
}


/*!
    Returns a localized string representation of the given \a time in the
    specified \a format.
*/

QString QLocale::toString(const QTime &time, FormatType format) const
{
    if (!time.isValid())
        return QString();

#ifndef QT_NO_SYSTEMLOCALE
    if (d() == systemPrivate()) {
        QVariant res = systemLocale()->query(format == LongFormat
                                             ? QSystemLocale::TimeToStringLong : QSystemLocale::TimeToStringShort,
                                             time);
        if (!res.isNull())
            return res.toString();
    }
#endif

    QString format_str = timeFormat(format);
    return toString(time, format_str);
}

/*!
    \since 4.1

    Returns the date format used for the current locale.

    If \a format is LongFormat the format will be a long version.
    Otherwise it uses a shorter version.

    \sa QDate::toString(), QDate::fromString()
*/

QString QLocale::dateFormat(FormatType format) const
{
#ifndef QT_NO_SYSTEMLOCALE
    if (d() == systemPrivate()) {
        QVariant res = systemLocale()->query(format == LongFormat
                                             ? QSystemLocale::DateFormatLong : QSystemLocale::DateFormatShort,
                                             QVariant());
        if (!res.isNull())
            return res.toString();
    }
#endif

    quint32 idx, size;
    switch (format) {
    case LongFormat:
        idx = d()->m_long_date_format_idx;
        size = d()->m_long_date_format_size;
        break;
    default:
        idx = d()->m_short_date_format_idx;
        size = d()->m_short_date_format_size;
        break;
    }
    return getLocaleData(date_format_data + idx, size);
}

/*!
    \since 4.1

    Returns the time format used for the current locale.

    If \a format is LongFormat the format will be a long version.
    Otherwise it uses a shorter version.

    \sa QTime::toString(), QTime::fromString()
*/

QString QLocale::timeFormat(FormatType format) const
{
#ifndef QT_NO_SYSTEMLOCALE
    if (d() == systemPrivate()) {
        QVariant res = systemLocale()->query(format == LongFormat
                                             ? QSystemLocale::TimeFormatLong : QSystemLocale::TimeFormatShort,
                                             QVariant());
        if (!res.isNull())
            return res.toString();
    }
#endif

    quint32 idx, size;
    switch (format) {
    case LongFormat:
        idx = d()->m_long_time_format_idx;
        size = d()->m_long_time_format_size;
        break;
    default:
        idx = d()->m_short_time_format_idx;
        size = d()->m_short_time_format_size;
        break;
    }
    return getLocaleData(time_format_data + idx, size);
}

/*!
    \since 4.4

    Returns the date time format used for the current locale.

    If \a format is ShortFormat the format will be a short version.
    Otherwise it uses a longer version.

    \sa QDateTime::toString(), QDateTime::fromString()
*/

QString QLocale::dateTimeFormat(FormatType format) const
{
#ifndef QT_NO_SYSTEMLOCALE
    if (d() == systemPrivate()) {
        QVariant res = systemLocale()->query(format == LongFormat
                                             ? QSystemLocale::DateTimeFormatLong
                                             : QSystemLocale::DateTimeFormatShort,
                                             QVariant());
        if (!res.isNull()) {
            return res.toString();
        }
    }
#endif
    return dateFormat(format) + QLatin1Char(' ') + timeFormat(format);
}

/*!
    \since 4.4

    Parses the time string given in \a string and returns the
    time. The format of the time string is chosen according to the
    \a format parameter (see timeFormat()).

    If the time could not be parsed, returns an invalid time.

    \sa timeFormat(), toDate(), toDateTime(), QTime::fromString()
*/
#ifndef QT_NO_DATESTRING
QTime QLocale::toTime(const QString &string, FormatType format) const
{
    return toTime(string, timeFormat(format));
}
#endif

/*!
    \since 4.4

    Parses the date string given in \a string and returns the
    date. The format of the date string is chosen according to the
    \a format parameter (see dateFormat()).

    If the date could not be parsed, returns an invalid date.

    \sa dateFormat(), toTime(), toDateTime(), QDate::fromString()
*/
#ifndef QT_NO_DATESTRING
QDate QLocale::toDate(const QString &string, FormatType format) const
{
    return toDate(string, dateFormat(format));
}
#endif

/*!
    \since 4.4

    Parses the date/time string given in \a string and returns the
    time. The format of the date/time string is chosen according to the
    \a format parameter (see dateTimeFormat()).

    If the string could not be parsed, returns an invalid QDateTime.

    \sa dateTimeFormat(), toTime(), toDate(), QDateTime::fromString()
*/

#ifndef QT_NO_DATESTRING
QDateTime QLocale::toDateTime(const QString &string, FormatType format) const
{
    return toDateTime(string, dateTimeFormat(format));
}
#endif

/*!
    \since 4.4

    Parses the time string given in \a string and returns the
    time. See QTime::fromString() for information on what is a valid
    format string.

    If the time could not be parsed, returns an invalid time.

    \sa timeFormat(), toDate(), toDateTime(), QTime::fromString()
*/
#ifndef QT_NO_DATESTRING
QTime QLocale::toTime(const QString &string, const QString &format) const
{
    QTime time;
#ifndef QT_BOOTSTRAPPED
    QDateTimeParser dt(QVariant::Time, QDateTimeParser::FromString);
    dt.defaultLocale = *this;
    if (dt.parseFormat(format))
        dt.fromString(string, 0, &time);
#else
    Q_UNUSED(string);
    Q_UNUSED(format);
#endif
    return time;
}
#endif

/*!
    \since 4.4

    Parses the date string given in \a string and returns the
    date. See QDate::fromString() for information on the expressions
    that can be used with this function.

    This function searches month names and the names of the days of
    the week in the current locale.

    If the date could not be parsed, returns an invalid date.

    \sa dateFormat(), toTime(), toDateTime(), QDate::fromString()
*/
#ifndef QT_NO_DATESTRING
QDate QLocale::toDate(const QString &string, const QString &format) const
{
    QDate date;
#ifndef QT_BOOTSTRAPPED
    QDateTimeParser dt(QVariant::Date, QDateTimeParser::FromString);
    dt.defaultLocale = *this;
    if (dt.parseFormat(format))
        dt.fromString(string, &date, 0);
#else
    Q_UNUSED(string);
    Q_UNUSED(format);
#endif
    return date;
}
#endif

/*!
    \since 4.4

    Parses the date/time string given in \a string and returns the
    time.  See QDateTime::fromString() for information on the expressions
    that can be used with this function.

    \note The month and day names used must be given in the user's local
    language.

    If the string could not be parsed, returns an invalid QDateTime.

    \sa dateTimeFormat(), toTime(), toDate(), QDateTime::fromString()
*/
#ifndef QT_NO_DATESTRING
QDateTime QLocale::toDateTime(const QString &string, const QString &format) const
{
#ifndef QT_BOOTSTRAPPED
    QTime time;
    QDate date;

    QDateTimeParser dt(QVariant::DateTime, QDateTimeParser::FromString);
    dt.defaultLocale = *this;
    if (dt.parseFormat(format) && dt.fromString(string, &date, &time))
        return QDateTime(date, time);
#else
    Q_UNUSED(string);
    Q_UNUSED(format);
#endif
    return QDateTime(QDate(), QTime(-1, -1, -1));
}
#endif


/*!
    \since 4.1

    Returns the decimal point character of this locale.
*/
QChar QLocale::decimalPoint() const
{
    return d()->decimal();
}

/*!
    \since 4.1

    Returns the group separator character of this locale.
*/
QChar QLocale::groupSeparator() const
{
    return d()->group();
}

/*!
    \since 4.1

    Returns the percent character of this locale.
*/
QChar QLocale::percent() const
{
    return d()->percent();
}

/*!
    \since 4.1

    Returns the zero digit character of this locale.
*/
QChar QLocale::zeroDigit() const
{
    return d()->zero();
}

/*!
    \since 4.1

    Returns the negative sign character of this locale.
*/
QChar QLocale::negativeSign() const
{
    return d()->minus();
}

/*!
    \since 4.5

    Returns the positive sign character of this locale.
*/
QChar QLocale::positiveSign() const
{
    return d()->plus();
}

/*!
    \since 4.1

    Returns the exponential character of this locale.
*/
QChar QLocale::exponential() const
{
    return d()->exponential();
}

static bool qIsUpper(char c)
{
    return c >= 'A' && c <= 'Z';
}

static char qToLower(char c)
{
    if (c >= 'A' && c <= 'Z')
        return c - 'A' + 'a';
    else
        return c;
}

/*!
    \overload

    \a f and \a prec have the same meaning as in QString::number(double, char, int).

    \sa toDouble()
*/

QString QLocale::toString(double i, char f, int prec) const
{
    QLocalePrivate::DoubleForm form = QLocalePrivate::DFDecimal;
    uint flags = 0;

    if (qIsUpper(f))
        flags = QLocalePrivate::CapitalEorX;
    f = qToLower(f);

    switch (f) {
        case 'f':
            form = QLocalePrivate::DFDecimal;
            break;
        case 'e':
            form = QLocalePrivate::DFExponent;
            break;
        case 'g':
            form = QLocalePrivate::DFSignificantDigits;
            break;
        default:
            break;
    }

    if (!(p.numberOptions & OmitGroupSeparator))
        flags |= QLocalePrivate::ThousandsGroup;
    return d()->doubleToString(i, prec, form, -1, flags);
}

/*!
    \fn QLocale QLocale::c()

    Returns a QLocale object initialized to the "C" locale.

    \sa system()
*/

/*!
    Returns a QLocale object initialized to the system locale.

    On Windows and Mac, this locale will use the decimal/grouping characters and date/time
    formats specified in the system configuration panel.

    \sa c()
*/

QLocale QLocale::system()
{
    QLocale result(C);
    result.p.index = localePrivateIndex(systemPrivate());
    return result;
}


/*!
    \since 4.8

    Returns a list of valid locale objects that match the given \a language, \a
    script and \a country.

    Getting a list of all locales:
    QList<QLocale> allLocales = QLocale::matchingLocales(QLocale::AnyLanguage, QLocale::AnyScript, QLocale::AnyCountry);
*/
QList<QLocale> QLocale::matchingLocales(QLocale::Language language,
                                        QLocale::Script script,
                                        QLocale::Country country)
{
    if (uint(language) > QLocale::LastLanguage || uint(script) > QLocale::LastScript ||
            uint(country) > QLocale::LastCountry)
        return QList<QLocale>();

    QList<QLocale> result;
    const QLocalePrivate *d = locale_data;
    if (language == QLocale::AnyLanguage && script == QLocale::AnyScript && country == QLocale::AnyCountry)
        result.reserve(locale_data_size);
    if (language != QLocale::C)
        d += locale_index[language];
    while ( (d != locale_data + locale_data_size)
            && (language == QLocale::AnyLanguage || d->m_language_id == uint(language))) {
        QLocale locale(QLocale::C);
        locale.p.index = localePrivateIndex(d);
        result.append(locale);
        ++d;
    }
    return result;
}

/*!
    \obsolete
    \since 4.3

    Returns the list of countries that have entires for \a language in Qt's locale
    database. If the result is an empty list, then \a language is not represented in
    Qt's locale database.

    \sa matchingLocales()
*/
QList<QLocale::Country> QLocale::countriesForLanguage(Language language)
{
    QList<Country> result;

    unsigned language_id = language;
    uint idx = locale_index[language_id];

    if (language == C) {
        result << AnyCountry;
        return result;
    }

    const QLocalePrivate *d = locale_data + idx;

    while (d->languageId() == language_id) {
        result << static_cast<Country>(d->countryId());
        ++d;
    }

    return result;
}

/*!
    \since 4.2

    Returns the localized name of \a month, in the format specified
    by \a type.

    \sa dayName(), standaloneMonthName()
*/
QString QLocale::monthName(int month, FormatType type) const
{
    if (month < 1 || month > 12)
        return QString();

#ifndef QT_NO_SYSTEMLOCALE
    if (d() == systemPrivate()) {
        QVariant res = systemLocale()->query(type == LongFormat
                                             ? QSystemLocale::MonthNameLong : QSystemLocale::MonthNameShort,
                                             month);
        if (!res.isNull())
            return res.toString();
    }
#endif

    quint32 idx, size;
    switch (type) {
    case QLocale::LongFormat:
        idx = d()->m_long_month_names_idx;
        size = d()->m_long_month_names_size;
        break;
    case QLocale::ShortFormat:
        idx = d()->m_short_month_names_idx;
        size = d()->m_short_month_names_size;
        break;
    case QLocale::NarrowFormat:
        idx = d()->m_narrow_month_names_idx;
        size = d()->m_narrow_month_names_size;
        break;
    default:
        return QString();
    }
    return getLocaleListData(months_data + idx, size, month - 1);
}

/*!
    \since 4.5

    Returns the localized name of \a month that is used as a
    standalone text, in the format specified by \a type.

    If the locale information doesn't specify the standalone month
    name then return value is the same as in monthName().

    \sa monthName(), standaloneDayName()
*/
QString QLocale::standaloneMonthName(int month, FormatType type) const
{
    if (month < 1 || month > 12)
        return QString();

#ifndef QT_NO_SYSTEMLOCALE
    if (d() == systemPrivate()) {
        QVariant res = systemLocale()->query(type == LongFormat
                                             ? QSystemLocale::MonthNameLong : QSystemLocale::MonthNameShort,
                                             month);
        if (!res.isNull())
            return res.toString();
    }
#endif

    quint32 idx, size;
    switch (type) {
    case QLocale::LongFormat:
        idx = d()->m_standalone_long_month_names_idx;
        size = d()->m_standalone_long_month_names_size;
        break;
    case QLocale::ShortFormat:
        idx = d()->m_standalone_short_month_names_idx;
        size = d()->m_standalone_short_month_names_size;
        break;
    case QLocale::NarrowFormat:
        idx = d()->m_standalone_narrow_month_names_idx;
        size = d()->m_standalone_narrow_month_names_size;
        break;
    default:
        return QString();
    }
    QString name = getLocaleListData(standalone_months_data + idx, size, month - 1);
    if (name.isEmpty())
        return monthName(month, type);
    return name;
}

/*!
    \since 4.2

    Returns the localized name of the \a day (where 1 represents
    Monday, 2 represents Tuesday and so on), in the format specified
    by \a type.

    \sa monthName(), standaloneDayName()
*/
QString QLocale::dayName(int day, FormatType type) const
{
    if (day < 1 || day > 7)
        return QString();

#ifndef QT_NO_SYSTEMLOCALE
    if (d() == systemPrivate()) {
        QVariant res = systemLocale()->query(type == LongFormat
                                             ? QSystemLocale::DayNameLong : QSystemLocale::DayNameShort,
                                             day);
        if (!res.isNull())
            return res.toString();
    }
#endif
    if (day == 7)
        day = 0;

    quint32 idx, size;
    switch (type) {
    case QLocale::LongFormat:
        idx = d()->m_long_day_names_idx;
        size = d()->m_long_day_names_size;
        break;
    case QLocale::ShortFormat:
        idx = d()->m_short_day_names_idx;
        size = d()->m_short_day_names_size;
        break;
    case QLocale::NarrowFormat:
        idx = d()->m_narrow_day_names_idx;
        size = d()->m_narrow_day_names_size;
        break;
    default:
        return QString();
    }
    return getLocaleListData(days_data + idx, size, day);
}

/*!
    \since 4.5

    Returns the localized name of the \a day (where 1 represents
    Monday, 2 represents Tuesday and so on) that is used as a
    standalone text, in the format specified by \a type.

    If the locale information does not specify the standalone day
    name then return value is the same as in dayName().

    \sa dayName(), standaloneMonthName()
*/
QString QLocale::standaloneDayName(int day, FormatType type) const
{
    if (day < 1 || day > 7)
        return QString();

#ifndef QT_NO_SYSTEMLOCALE
    if (d() == systemPrivate()) {
        QVariant res = systemLocale()->query(type == LongFormat
                                             ? QSystemLocale::DayNameLong : QSystemLocale::DayNameShort,
                                             day);
        if (!res.isNull())
            return res.toString();
    }
#endif
    if (day == 7)
        day = 0;

    quint32 idx, size;
    switch (type) {
    case QLocale::LongFormat:
        idx = d()->m_standalone_long_day_names_idx;
        size = d()->m_standalone_long_day_names_size;
        break;
    case QLocale::ShortFormat:
        idx = d()->m_standalone_short_day_names_idx;
        size = d()->m_standalone_short_day_names_size;
        break;
    case QLocale::NarrowFormat:
        idx = d()->m_standalone_narrow_day_names_idx;
        size = d()->m_standalone_narrow_day_names_size;
        break;
    default:
        return QString();
    }
    QString name = getLocaleListData(days_data + idx, size, day);
    if (name.isEmpty())
        return dayName(day == 0 ? 7 : day, type);
    return name;
}

/*!
    \since 4.8

    Returns the first day of the week according to the current locale.
*/
Qt::DayOfWeek QLocale::firstDayOfWeek() const
{
#ifndef QT_NO_SYSTEMLOCALE
    if (d() == systemPrivate()) {
        QVariant res = systemLocale()->query(QSystemLocale::FirstDayOfWeek, QVariant());
        if (!res.isNull())
            return static_cast<Qt::DayOfWeek>(res.toUInt());
    }
#endif
    return static_cast<Qt::DayOfWeek>(d()->m_first_day_of_week);
}

QLocale::MeasurementSystem QLocalePrivate::measurementSystem() const
{
    for (int i = 0; i < ImperialMeasurementSystemsCount; ++i) {
        if (ImperialMeasurementSystems[i].languageId == m_language_id
            && ImperialMeasurementSystems[i].countryId == m_country_id) {
            return QLocale::ImperialSystem;
        }
    }
    return QLocale::MetricSystem;
}

/*!
    \since 4.8

    Returns a list of days that are considered weekdays according to the current locale.
*/
QList<Qt::DayOfWeek> QLocale::weekdays() const
{
#ifndef QT_NO_SYSTEMLOCALE
    if (d() == systemPrivate()) {
        QVariant res = systemLocale()->query(QSystemLocale::Weekdays, QVariant());
        if (!res.isNull())
            return static_cast<QList<Qt::DayOfWeek> >(res.value<QList<Qt::DayOfWeek> >());
    }
#endif
    QList<Qt::DayOfWeek> weekdays;
    quint16 weekendStart = d()->m_weekend_start;
    quint16 weekendEnd = d()->m_weekend_end;
    for (int day = Qt::Monday; day <= Qt::Sunday; day++) {
        if ((weekendEnd >= weekendStart && (day < weekendStart || day > weekendEnd)) ||
            (weekendEnd < weekendStart && (day > weekendEnd && day < weekendStart)))
                weekdays << static_cast<Qt::DayOfWeek>(day);
    }
    return weekdays;
}

/*!
    \since 4.4

    Returns the measurement system for the locale.
*/
QLocale::MeasurementSystem QLocale::measurementSystem() const
{
#ifndef QT_NO_SYSTEMLOCALE
    if (d() == systemPrivate()) {
        QVariant res = systemLocale()->query(QSystemLocale::MeasurementSystem, QVariant());
        if (!res.isNull())
            return MeasurementSystem(res.toInt());
    }
#endif

    return d()->measurementSystem();
}

/*!
  \since 4.7

  Returns the text direction of the language.
*/
Qt::LayoutDirection QLocale::textDirection() const
{
    Language lang = language();
    if (lang == QLocale::Arabic ||
        lang == QLocale::Hebrew ||
        lang == QLocale::Persian ||
        lang == QLocale::Urdu ||
        lang == QLocale::Syriac)
        return Qt::RightToLeft;

    return Qt::LeftToRight;
}

/*!
  \since 4.8

  Returns an uppercase copy of \a str.
*/
QString QLocale::toUpper(const QString &str) const
{
#ifdef QT_USE_ICU
    {
        QString result;
        if (qt_u_strToUpper(str, &result, *this))
            return result;
        // else fall through and use Qt's toUpper
    }
#endif
    return str.toUpper();
}

/*!
  \since 4.8

  Returns a lowercase copy of \a str.
*/
QString QLocale::toLower(const QString &str) const
{
#ifdef QT_USE_ICU
    {
        QString result;
        if (qt_u_strToLower(str, &result, *this))
            return result;
        // else fall through and use Qt's toUpper
    }
#endif
    return str.toLower();
}


/*!
    \since 4.5

    Returns the localized name of the "AM" suffix for times specified using
    the conventions of the 12-hour clock.

    \sa pmText()
*/
QString QLocale::amText() const
{
#ifndef QT_NO_SYSTEMLOCALE
    if (d() == systemPrivate()) {
        QVariant res = systemLocale()->query(QSystemLocale::AMText, QVariant());
        if (!res.isNull())
            return res.toString();
    }
#endif
    return getLocaleData(am_data + d()->m_am_idx, d()->m_am_size);
}

/*!
    \since 4.5

    Returns the localized name of the "PM" suffix for times specified using
    the conventions of the 12-hour clock.

    \sa amText()
*/
QString QLocale::pmText() const
{
#ifndef QT_NO_SYSTEMLOCALE
    if (d() == systemPrivate()) {
        QVariant res = systemLocale()->query(QSystemLocale::PMText, QVariant());
        if (!res.isNull())
            return res.toString();
    }
#endif
    return getLocaleData(pm_data + d()->m_pm_idx, d()->m_pm_size);
}


QString QLocalePrivate::dateTimeToString(const QString &format, const QDate *date, const QTime *time,
                                         const QLocale *q) const
{
    Q_ASSERT(date || time);
    if ((date && !date->isValid()) || (time && !time->isValid()))
        return QString();
    const bool format_am_pm = time && timeFormatContainsAP(format);

    enum { AM, PM } am_pm = AM;
    int hour12 = time ? time->hour() : -1;
    if (time) {
        if (hour12 == 0) {
            am_pm = AM;
            hour12 = 12;
        } else if (hour12 < 12) {
            am_pm = AM;
        } else if (hour12 == 12) {
            am_pm = PM;
        } else {
            am_pm = PM;
            hour12 -= 12;
        }
    }

    QString result;

    int i = 0;
    while (i < format.size()) {
        if (format.at(i).unicode() == '\'') {
            result.append(qt_readEscapedFormatString(format, &i));
            continue;
        }

        const QChar c = format.at(i);
        int repeat = qt_repeatCount(format, i);
        bool used = false;
        if (date) {
            switch (c.unicode()) {
            case 'y':
                used = true;
                if (repeat >= 4)
                    repeat = 4;
                else if (repeat >= 2)
                    repeat = 2;

                switch (repeat) {
                case 4:
                    result.append(longLongToString(date->year()));
                    break;
                case 2:
                    result.append(longLongToString(date->year() % 100, -1, 10, 2,
                                                   QLocalePrivate::ZeroPadded));
                    break;
                default:
                    repeat = 1;
                    result.append(c);
                    break;
                }
                break;

            case 'M':
                used = true;
                repeat = qMin(repeat, 4);
                switch (repeat) {
                case 1:
                    result.append(longLongToString(date->month()));
                    break;
                case 2:
                    result.append(longLongToString(date->month(), -1, 10, 2, QLocalePrivate::ZeroPadded));
                    break;
                case 3:
                    result.append(q->monthName(date->month(), QLocale::ShortFormat));
                    break;
                case 4:
                    result.append(q->monthName(date->month(), QLocale::LongFormat));
                    break;
                }
                break;

            case 'd':
                used = true;
                repeat = qMin(repeat, 4);
                switch (repeat) {
                case 1:
                    result.append(longLongToString(date->day()));
                    break;
                case 2:
                    result.append(longLongToString(date->day(), -1, 10, 2, QLocalePrivate::ZeroPadded));
                    break;
                case 3:
                    result.append(q->dayName(date->dayOfWeek(), QLocale::ShortFormat));
                    break;
                case 4:
                    result.append(q->dayName(date->dayOfWeek(), QLocale::LongFormat));
                    break;
                }
                break;

            default:
                break;
            }
        }
        if (!used && time) {
            switch (c.unicode()) {
            case 'h': {
                used = true;
                repeat = qMin(repeat, 2);
                const int hour = format_am_pm ? hour12 : time->hour();

                switch (repeat) {
                case 1:
                    result.append(longLongToString(hour));
                    break;
                case 2:
                    result.append(longLongToString(hour, -1, 10, 2, QLocalePrivate::ZeroPadded));
                    break;
                }
                break;
            }
            case 'H':
                used = true;
                repeat = qMin(repeat, 2);
                switch (repeat) {
                case 1:
                    result.append(longLongToString(time->hour()));
                    break;
                case 2:
                    result.append(longLongToString(time->hour(), -1, 10, 2, QLocalePrivate::ZeroPadded));
                    break;
                }
                break;

            case 'm':
                used = true;
                repeat = qMin(repeat, 2);
                switch (repeat) {
                case 1:
                    result.append(longLongToString(time->minute()));
                    break;
                case 2:
                    result.append(longLongToString(time->minute(), -1, 10, 2, QLocalePrivate::ZeroPadded));
                    break;
                }
                break;

            case 's':
                used = true;
                repeat = qMin(repeat, 2);
                switch (repeat) {
                case 1:
                    result.append(longLongToString(time->second()));
                    break;
                case 2:
                    result.append(longLongToString(time->second(), -1, 10, 2, QLocalePrivate::ZeroPadded));
                    break;
                }
                break;

            case 'a':
                used = true;
                if (i + 1 < format.length() && format.at(i + 1).unicode() == 'p') {
                    repeat = 2;
                } else {
                    repeat = 1;
                }
                result.append(am_pm == AM ? q->amText().toLower() : q->pmText().toLower());
                break;

            case 'A':
                used = true;
                if (i + 1 < format.length() && format.at(i + 1).unicode() == 'P') {
                    repeat = 2;
                } else {
                    repeat = 1;
                }
                result.append(am_pm == AM ? q->amText().toUpper() : q->pmText().toUpper());
                break;

            case 'z':
                used = true;
                if (repeat >= 3) {
                    repeat = 3;
                } else {
                    repeat = 1;
                }
                switch (repeat) {
                case 1:
                    result.append(longLongToString(time->msec()));
                    break;
                case 3:
                    result.append(longLongToString(time->msec(), -1, 10, 3, QLocalePrivate::ZeroPadded));
                    break;
                }
                break;

            case 't':
                used = true;
                repeat = 1;
                result.append(timeZone());
                break;
            default:
                break;
            }
        }
        if (!used) {
            result.append(QString(repeat, c));
        }
        i += repeat;
    }

    return result;
}

QString QLocalePrivate::doubleToString(double d,
                                       int precision,
                                       DoubleForm form,
                                       int width,
                                       unsigned flags) const
{
    return QLocalePrivate::doubleToString(zero(), plus(), minus(), exponential(),
                                          group(), decimal(),
                                          d, precision, form, width, flags);
}

QString QLocalePrivate::doubleToString(const QChar _zero, const QChar plus, const QChar minus,
                                       const QChar exponential, const QChar group, const QChar decimal,
                                       double d,
                                       int precision,
                                       DoubleForm form,
                                       int width,
                                       unsigned flags)
{
    if (precision == -1)
        precision = 6;
    if (width == -1)
        width = 0;

    bool negative = false;
    bool special_number = false; // nan, +/-inf
    QString num_str;

    // Detect special numbers (nan, +/-inf)
    if (qt_is_inf(d)) {
        num_str = QString::fromLatin1("inf");
        special_number = true;
        negative = d < 0;
    } else if (qt_is_nan(d)) {
        num_str = QString::fromLatin1("nan");
        special_number = true;
    }

    // Handle normal numbers
    if (!special_number) {
        int decpt, sign;
        QString digits;

#ifdef QT_QLOCALE_USES_FCVT
        // NOT thread safe!
        if (form == DFDecimal) {
            digits = QLatin1String(fcvt(d, precision, &decpt, &sign));
        } else {
            int pr = precision;
            if (form == DFExponent)
                ++pr;
            else if (form == DFSignificantDigits && pr == 0)
                pr = 1;
            digits = QLatin1String(ecvt(d, pr, &decpt, &sign));

            // Chop trailing zeros
            if (digits.length() > 0) {
                int last_nonzero_idx = digits.length() - 1;
                while (last_nonzero_idx > 0
                       && digits.unicode()[last_nonzero_idx] == QLatin1Char('0'))
                    --last_nonzero_idx;
                digits.truncate(last_nonzero_idx + 1);
            }

        }

#else
        int mode;
        if (form == DFDecimal)
            mode = 3;
        else
            mode = 2;

        /* This next bit is a bit quirky. In DFExponent form, the precision
           is the number of digits after decpt. So that would suggest using
           mode=3 for qdtoa. But qdtoa behaves strangely when mode=3 and
           precision=0. So we get around this by using mode=2 and reasoning
           that we want precision+1 significant digits, since the decimal
           point in this mode is always after the first digit. */
        int pr = precision;
        if (form == DFExponent)
            ++pr;

        char *rve = 0;
        char *buff = 0;
        QT_TRY {
            digits = QLatin1String(qdtoa(d, mode, pr, &decpt, &sign, &rve, &buff));
        } QT_CATCH(...) {
            if (buff != 0)
                free(buff);
            QT_RETHROW;
        }
        if (buff != 0)
            free(buff);
#endif // QT_QLOCALE_USES_FCVT

        if (_zero.unicode() != '0') {
            ushort z = _zero.unicode() - '0';
            for (int i = 0; i < digits.length(); ++i)
                reinterpret_cast<ushort *>(digits.data())[i] += z;
        }

        bool always_show_decpt = (flags & Alternate || flags & ForcePoint);
        switch (form) {
            case DFExponent: {
                num_str = exponentForm(_zero, decimal, exponential, group, plus, minus,
                                       digits, decpt, precision, PMDecimalDigits,
                                       always_show_decpt);
                break;
            }
            case DFDecimal: {
                num_str = decimalForm(_zero, decimal, group,
                                      digits, decpt, precision, PMDecimalDigits,
                                      always_show_decpt, flags & ThousandsGroup);
                break;
            }
            case DFSignificantDigits: {
                PrecisionMode mode = (flags & Alternate) ?
                            PMSignificantDigits : PMChopTrailingZeros;

                if (decpt != digits.length() && (decpt <= -4 || decpt > precision))
                    num_str = exponentForm(_zero, decimal, exponential, group, plus, minus,
                                           digits, decpt, precision, mode,
                                           always_show_decpt);
                else
                    num_str = decimalForm(_zero, decimal, group,
                                          digits, decpt, precision, mode,
                                          always_show_decpt, flags & ThousandsGroup);
                break;
            }
        }

        negative = sign != 0 && !isZero(d);
    }

    // pad with zeros. LeftAdjusted overrides this flag). Also, we don't
    // pad special numbers
    if (flags & QLocalePrivate::ZeroPadded
            && !(flags & QLocalePrivate::LeftAdjusted)
            && !special_number) {
        int num_pad_chars = width - num_str.length();
        // leave space for the sign
        if (negative
                || flags & QLocalePrivate::AlwaysShowSign
                || flags & QLocalePrivate::BlankBeforePositive)
            --num_pad_chars;

        for (int i = 0; i < num_pad_chars; ++i)
            num_str.prepend(_zero);
    }

    // add sign
    if (negative)
        num_str.prepend(minus);
    else if (flags & QLocalePrivate::AlwaysShowSign)
        num_str.prepend(plus);
    else if (flags & QLocalePrivate::BlankBeforePositive)
        num_str.prepend(QLatin1Char(' '));

    if (flags & QLocalePrivate::CapitalEorX)
        num_str = num_str.toUpper();

    return num_str;
}

QString QLocalePrivate::longLongToString(qlonglong l, int precision,
                                            int base, int width,
                                            unsigned flags) const
{
    return QLocalePrivate::longLongToString(zero(), group(), plus(), minus(),
                                            l, precision, base, width, flags);
}

QString QLocalePrivate::longLongToString(const QChar zero, const QChar group,
                                         const QChar plus, const QChar minus,
                                         qlonglong l, int precision,
                                         int base, int width,
                                         unsigned flags)
{
    bool precision_not_specified = false;
    if (precision == -1) {
        precision_not_specified = true;
        precision = 1;
    }

    bool negative = l < 0;
    if (base != 10) {
        // these are not supported by sprintf for octal and hex
        flags &= ~AlwaysShowSign;
        flags &= ~BlankBeforePositive;
        negative = false; // neither are negative numbers
    }

    QString num_str;
    if (base == 10)
        num_str = qlltoa(l, base, zero);
    else
        num_str = qulltoa(l, base, zero);

    uint cnt_thousand_sep = 0;
    if (flags & ThousandsGroup && base == 10) {
        for (int i = num_str.length() - 3; i > 0; i -= 3) {
            num_str.insert(i, group);
            ++cnt_thousand_sep;
        }
    }

    for (int i = num_str.length()/* - cnt_thousand_sep*/; i < precision; ++i)
        num_str.prepend(base == 10 ? zero : QChar::fromLatin1('0'));

    if ((flags & Alternate || flags & ShowBase)
            && base == 8
            && (num_str.isEmpty() || num_str[0].unicode() != QLatin1Char('0')))
        num_str.prepend(QLatin1Char('0'));

    // LeftAdjusted overrides this flag ZeroPadded. sprintf only padds
    // when precision is not specified in the format string
    bool zero_padded = flags & ZeroPadded
                        && !(flags & LeftAdjusted)
                        && precision_not_specified;

    if (zero_padded) {
        int num_pad_chars = width - num_str.length();

        // leave space for the sign
        if (negative
                || flags & AlwaysShowSign
                || flags & BlankBeforePositive)
            --num_pad_chars;

        // leave space for optional '0x' in hex form
        if (base == 16 && (flags & Alternate || flags & ShowBase))
            num_pad_chars -= 2;
        // leave space for optional '0b' in binary form
        else if (base == 2 && (flags & Alternate || flags & ShowBase))
            num_pad_chars -= 2;

        for (int i = 0; i < num_pad_chars; ++i)
            num_str.prepend(base == 10 ? zero : QChar::fromLatin1('0'));
    }

    if (flags & CapitalEorX)
        num_str = num_str.toUpper();

    if (base == 16 && (flags & Alternate || flags & ShowBase))
        num_str.prepend(QLatin1String(flags & UppercaseBase ? "0X" : "0x"));
    if (base == 2 && (flags & Alternate || flags & ShowBase))
        num_str.prepend(QLatin1String(flags & UppercaseBase ? "0B" : "0b"));

    // add sign
    if (negative)
        num_str.prepend(minus);
    else if (flags & AlwaysShowSign)
        num_str.prepend(plus);
    else if (flags & BlankBeforePositive)
        num_str.prepend(QLatin1Char(' '));

    return num_str;
}

QString QLocalePrivate::unsLongLongToString(qulonglong l, int precision,
                                            int base, int width,
                                            unsigned flags) const
{
    return QLocalePrivate::unsLongLongToString(zero(), group(), plus(),
                                               l, precision, base, width, flags);
}

QString QLocalePrivate::unsLongLongToString(const QChar zero, const QChar group,
                                            const QChar plus,
                                            qulonglong l, int precision,
                                            int base, int width,
                                            unsigned flags)
{
    bool precision_not_specified = false;
    if (precision == -1) {
        precision_not_specified = true;
        precision = 1;
    }

    QString num_str = qulltoa(l, base, zero);

    uint cnt_thousand_sep = 0;
    if (flags & ThousandsGroup && base == 10) {
        for (int i = num_str.length() - 3; i > 0; i -=3) {
            num_str.insert(i, group);
            ++cnt_thousand_sep;
        }
    }

    for (int i = num_str.length()/* - cnt_thousand_sep*/; i < precision; ++i)
        num_str.prepend(base == 10 ? zero : QChar::fromLatin1('0'));

    if ((flags & Alternate || flags & ShowBase)
            && base == 8
            && (num_str.isEmpty() || num_str[0].unicode() != QLatin1Char('0')))
        num_str.prepend(QLatin1Char('0'));

    // LeftAdjusted overrides this flag ZeroPadded. sprintf only padds
    // when precision is not specified in the format string
    bool zero_padded = flags & ZeroPadded
                        && !(flags & LeftAdjusted)
                        && precision_not_specified;

    if (zero_padded) {
        int num_pad_chars = width - num_str.length();

        // leave space for optional '0x' in hex form
        if (base == 16 && flags & Alternate)
            num_pad_chars -= 2;
        // leave space for optional '0b' in binary form
        else if (base == 2 && flags & Alternate)
            num_pad_chars -= 2;

        for (int i = 0; i < num_pad_chars; ++i)
            num_str.prepend(base == 10 ? zero : QChar::fromLatin1('0'));
    }

    if (flags & CapitalEorX)
        num_str = num_str.toUpper();

    if (base == 16 && (flags & Alternate || flags & ShowBase))
        num_str.prepend(QLatin1String(flags & UppercaseBase ? "0X" : "0x"));
    else if (base == 2 && (flags & Alternate || flags & ShowBase))
        num_str.prepend(QLatin1String(flags & UppercaseBase ? "0B" : "0b"));

    // add sign
    if (flags & AlwaysShowSign)
        num_str.prepend(plus);
    else if (flags & BlankBeforePositive)
        num_str.prepend(QLatin1Char(' '));

    return num_str;
}

/*
    Converts a number in locale to its representation in the C locale.
    Only has to guarantee that a string that is a correct representation of
    a number will be converted. If junk is passed in, junk will be passed
    out and the error will be detected during the actual conversion to a
    number. We can't detect junk here, since we don't even know the base
    of the number.
*/
bool QLocalePrivate::numberToCLocale(const QString &num,
                                            GroupSeparatorMode group_sep_mode,
                                            CharBuff *result) const
{
    const QChar *uc = num.unicode();
    int l = num.length();
    int idx = 0;

    // Skip whitespace
    while (idx < l && uc[idx].isSpace())
        ++idx;
    if (idx == l)
        return false;

    while (idx < l) {
        const QChar &in = uc[idx];

        char out = digitToCLocale(in);
        if (out == 0) {
            if (in == list())
                out = ';';
            else if (in == percent())
                out = '%';
            // for handling base-x numbers
            else if (in.unicode() >= 'A' && in.unicode() <= 'Z')
                out = in.toLower().toLatin1();
            else if (in.unicode() >= 'a' && in.unicode() <= 'z')
                out = in.toLatin1();
            else
                break;
        }

        result->append(out);

        ++idx;
    }

    // Check trailing whitespace
    for (; idx < l; ++idx) {
        if (!uc[idx].isSpace())
            return false;
    }

    result->append('\0');

    // Check separators
    if (group_sep_mode == ParseGroupSeparators
            && !removeGroupSeparators(result))
        return false;


    return true;
}

bool QLocalePrivate::validateChars(const QString &str, NumberMode numMode, QByteArray *buff,
                                    int decDigits) const
{
    buff->clear();
    buff->reserve(str.length());

    const bool scientific = numMode == DoubleScientificMode;
    bool lastWasE = false;
    bool lastWasDigit = false;
    int eCnt = 0;
    int decPointCnt = 0;
    bool dec = false;
    int decDigitCnt = 0;

    for (int i = 0; i < str.length(); ++i) {
        char c = digitToCLocale(str.at(i));

        if (c >= '0' && c <= '9') {
            if (numMode != IntegerMode) {
                // If a double has too many digits after decpt, it shall be Invalid.
                if (dec && decDigits != -1 && decDigits < ++decDigitCnt)
                    return false;
            }
            lastWasDigit = true;
        } else {
            switch (c) {
                case '.':
                    if (numMode == IntegerMode) {
                        // If an integer has a decimal point, it shall be Invalid.
                        return false;
                    } else {
                        // If a double has more than one decimal point, it shall be Invalid.
                        if (++decPointCnt > 1)
                            return false;
#if 0
                        // If a double with no decimal digits has a decimal point, it shall be
                        // Invalid.
                        if (decDigits == 0)
                            return false;
#endif                  // On second thoughts, it shall be Valid.

                        dec = true;
                    }
                    break;

                case '+':
                case '-':
                    if (scientific) {
                        // If a scientific has a sign that's not at the beginning or after
                        // an 'e', it shall be Invalid.
                        if (i != 0 && !lastWasE)
                            return false;
                    } else {
                        // If a non-scientific has a sign that's not at the beginning,
                        // it shall be Invalid.
                        if (i != 0)
                            return false;
                    }
                    break;

                case ',':
                    //it can only be placed after a digit which is before the decimal point
                    if (!lastWasDigit || decPointCnt > 0)
                        return false;
                    break;

                case 'e':
                    if (scientific) {
                        // If a scientific has more than one 'e', it shall be Invalid.
                        if (++eCnt > 1)
                            return false;
                        dec = false;
                    } else {
                        // If a non-scientific has an 'e', it shall be Invalid.
                        return false;
                    }
                    break;

                default:
                    // If it's not a valid digit, it shall be Invalid.
                    return false;
            }
            lastWasDigit = false;
        }

        lastWasE = c == 'e';
        if (c != ',')
            buff->append(c);
    }

    return true;
}

double QLocalePrivate::stringToDouble(const QString &number, bool *ok,
                                        GroupSeparatorMode group_sep_mode) const
{
    CharBuff buff;
    if (!numberToCLocale(group().unicode() == 0xa0 ? number.trimmed() : number,
                         group_sep_mode, &buff)) {
        if (ok != 0)
            *ok = false;
        return 0.0;
    }
    return bytearrayToDouble(buff.constData(), ok);
}

qlonglong QLocalePrivate::stringToLongLong(const QString &number, int base,
                                           bool *ok, GroupSeparatorMode group_sep_mode) const
{
    CharBuff buff;
    if (!numberToCLocale(group().unicode() == 0xa0 ? number.trimmed() : number,
                         group_sep_mode, &buff)) {
        if (ok != 0)
            *ok = false;
        return 0;
    }

    return bytearrayToLongLong(buff.constData(), base, ok);
}

qulonglong QLocalePrivate::stringToUnsLongLong(const QString &number, int base,
                                               bool *ok, GroupSeparatorMode group_sep_mode) const
{
    CharBuff buff;
    if (!numberToCLocale(group().unicode() == 0xa0 ? number.trimmed() : number,
                         group_sep_mode, &buff)) {
        if (ok != 0)
            *ok = false;
        return 0;
    }

    return bytearrayToUnsLongLong(buff.constData(), base, ok);
}


double QLocalePrivate::bytearrayToDouble(const char *num, bool *ok, bool *overflow)
{
    if (ok != 0)
        *ok = true;
    if (overflow != 0)
        *overflow = false;

    if (*num == '\0') {
        if (ok != 0)
            *ok = false;
        return 0.0;
    }

    if (qstrcmp(num, "nan") == 0)
        return qt_snan();

    if (qstrcmp(num, "+inf") == 0 || qstrcmp(num, "inf") == 0)
        return qt_inf();

    if (qstrcmp(num, "-inf") == 0)
        return -qt_inf();

    bool _ok;
    const char *endptr;
    double d = qstrtod(num, &endptr, &_ok);

    if (!_ok) {
        // the only way strtod can fail with *endptr != '\0' on a non-empty
        // input string is overflow
        if (ok != 0)
            *ok = false;
        if (overflow != 0)
            *overflow = *endptr != '\0';
        return 0.0;
    }

    if (*endptr != '\0') {
        // we stopped at a non-digit character after converting some digits
        if (ok != 0)
            *ok = false;
        if (overflow != 0)
            *overflow = false;
        return 0.0;
    }

    if (ok != 0)
        *ok = true;
    if (overflow != 0)
        *overflow = false;
    return d;
}

qlonglong QLocalePrivate::bytearrayToLongLong(const char *num, int base, bool *ok, bool *overflow)
{
    bool _ok;
    const char *endptr;

    if (*num == '\0') {
        if (ok != 0)
            *ok = false;
        if (overflow != 0)
            *overflow = false;
        return 0;
    }

    qlonglong l = qstrtoll(num, &endptr, base, &_ok);

    if (!_ok) {
        if (ok != 0)
            *ok = false;
        if (overflow != 0) {
            // the only way qstrtoll can fail with *endptr != '\0' on a non-empty
            // input string is overflow
            *overflow = *endptr != '\0';
        }
        return 0;
    }

    if (*endptr != '\0') {
        // we stopped at a non-digit character after converting some digits
        if (ok != 0)
            *ok = false;
        if (overflow != 0)
            *overflow = false;
        return 0;
    }

    if (ok != 0)
        *ok = true;
    if (overflow != 0)
        *overflow = false;
    return l;
}

qulonglong QLocalePrivate::bytearrayToUnsLongLong(const char *num, int base, bool *ok)
{
    bool _ok;
    const char *endptr;
    qulonglong l = qstrtoull(num, &endptr, base, &_ok);

    if (!_ok || *endptr != '\0') {
        if (ok != 0)
            *ok = false;
        return 0;
    }

    if (ok != 0)
        *ok = true;
    return l;
}

/*!
    \since 4.8

    \enum QLocale::CurrencySymbolFormat

    Specifies the format of the currency symbol.

    \value CurrencyIsoCode a ISO-4217 code of the currency.
    \value CurrencySymbol a currency symbol.
    \value CurrencyDisplayName a user readable name of the currency.
*/

/*!
    \since 4.8
    Returns a currency symbol according to the \a format.
*/
QString QLocale::currencySymbol(QLocale::CurrencySymbolFormat format) const
{
#ifndef QT_NO_SYSTEMLOCALE
    if (d() == systemPrivate()) {
        QVariant res = systemLocale()->query(QSystemLocale::CurrencySymbol, format);
        if (!res.isNull())
            return res.toString();
    }
#endif
    quint32 idx, size;
    switch (format) {
    case CurrencySymbol:
        idx = d()->m_currency_symbol_idx;
        size = d()->m_currency_symbol_size;
        return getLocaleData(currency_symbol_data + idx, size);
    case CurrencyDisplayName:
        idx = d()->m_currency_display_name_idx;
        size = d()->m_currency_display_name_size;
        return getLocaleListData(currency_display_name_data + idx, size, 0);
    case CurrencyIsoCode: {
        int len = 0;
        const QLocalePrivate *d = this->d();
        for (; len < 3; ++len)
            if (!d->m_currency_iso_code[len])
                break;
        return len ? QString::fromLatin1(d->m_currency_iso_code, len) : QString();
    }
    }
    return QString();
}

/*!
    \since 4.8

    Returns a localized string representation of \a value as a currency.
    If the \a symbol is provided it is used instead of the default currency symbol.

    \sa currencySymbol()
*/
QString QLocale::toCurrencyString(qlonglong value, const QString &symbol) const
{
#ifndef QT_NO_SYSTEMLOCALE
    if (d() == systemPrivate()) {
        QSystemLocale::CurrencyToStringArgument arg(value, symbol);
        QVariant res = systemLocale()->query(QSystemLocale::CurrencyToString, QVariant::fromValue(arg));
        if (!res.isNull())
            return res.toString();
    }
#endif
    const QLocalePrivate *d = this->d();
    quint8 idx = d->m_currency_format_idx;
    quint8 size = d->m_currency_format_size;
    if (d->m_currency_negative_format_size && value < 0) {
        idx = d->m_currency_negative_format_idx;
        size = d->m_currency_negative_format_size;
        value = -value;
    }
    QString str = d->longLongToString(value);
    QString sym = symbol.isNull() ? currencySymbol() : symbol;
    if (sym.isEmpty())
        sym = currencySymbol(QLocale::CurrencyIsoCode);
    QString format = getLocaleData(currency_format_data + idx, size);
    return format.arg(str, sym);
}

/*!
    \since 4.8
    \overload
*/
QString QLocale::toCurrencyString(qulonglong value, const QString &symbol) const
{
#ifndef QT_NO_SYSTEMLOCALE
    if (d() == systemPrivate()) {
        QSystemLocale::CurrencyToStringArgument arg(value, symbol);
        QVariant res = systemLocale()->query(QSystemLocale::CurrencyToString, QVariant::fromValue(arg));
        if (!res.isNull())
            return res.toString();
    }
#endif
    const QLocalePrivate *d = this->d();
    quint8 idx = d->m_currency_format_idx;
    quint8 size = d->m_currency_format_size;
    QString str = d->unsLongLongToString(value);
    QString sym = symbol.isNull() ? currencySymbol() : symbol;
    if (sym.isEmpty())
        sym = currencySymbol(QLocale::CurrencyIsoCode);
    QString format = getLocaleData(currency_format_data + idx, size);
    return format.arg(str, sym);
}

/*!
    \since 4.8
    \overload
*/
QString QLocale::toCurrencyString(double value, const QString &symbol) const
{
#ifndef QT_NO_SYSTEMLOCALE
    if (d() == systemPrivate()) {
        QSystemLocale::CurrencyToStringArgument arg(value, symbol);
        QVariant res = systemLocale()->query(QSystemLocale::CurrencyToString, QVariant::fromValue(arg));
        if (!res.isNull())
            return res.toString();
    }
#endif
    const QLocalePrivate *d = this->d();
    quint8 idx = d->m_currency_format_idx;
    quint8 size = d->m_currency_format_size;
    if (d->m_currency_negative_format_size && value < 0) {
        idx = d->m_currency_negative_format_idx;
        size = d->m_currency_negative_format_size;
        value = -value;
    }
    QString str = d->doubleToString(value, d->m_currency_digits,
                                    QLocalePrivate::DFDecimal);
    QString sym = symbol.isNull() ? currencySymbol() : symbol;
    if (sym.isEmpty())
        sym = currencySymbol(QLocale::CurrencyIsoCode);
    QString format = getLocaleData(currency_format_data + idx, size);
    return format.arg(str, sym);
}

/*!
    \since 4.8

    Returns an ordered list of locale names for translation purposes in
    preference order.

    The return value represents locale names that the user expects to see the
    UI translation in.

    Most like you do not need to use this function directly, but just pass the
    QLocale object to the QTranslator::load() function.

    The first item in the list is the most preferred one.

    \sa QTranslator, bcp47Name()
*/
QStringList QLocale::uiLanguages() const
{
#ifndef QT_NO_SYSTEMLOCALE
    if (d() == systemPrivate()) {
        QVariant res = systemLocale()->query(QSystemLocale::UILanguages, QVariant());
        if (!res.isNull()) {
            QStringList result = res.toStringList();
            if (!result.isEmpty())
                return result;
        }
    }
#endif
    return QStringList(bcp47Name());
}

/*!
    \since 4.8

    Returns a native name of the language for the locale. For example
    "Schwiizertüütsch" for Swiss-German locale.

    \sa nativeCountryName(), languageToString()
*/
QString QLocale::nativeLanguageName() const
{
#ifndef QT_NO_SYSTEMLOCALE
    if (d() == systemPrivate()) {
        QVariant res = systemLocale()->query(QSystemLocale::NativeLanguageName, QVariant());
        if (!res.isNull())
            return res.toString();
    }
#endif
    return getLocaleData(endonyms_data + d()->m_language_endonym_idx, d()->m_language_endonym_size);
}

/*!
    \since 4.8

    Returns a native name of the country for the locale. For example
    "España" for Spanish/Spain locale.

    \sa nativeLanguageName(), countryToString()
*/
QString QLocale::nativeCountryName() const
{
#ifndef QT_NO_SYSTEMLOCALE
    if (d() == systemPrivate()) {
        QVariant res = systemLocale()->query(QSystemLocale::NativeCountryName, QVariant());
        if (!res.isNull())
            return res.toString();
    }
#endif
    return getLocaleData(endonyms_data + d()->m_country_endonym_idx, d()->m_country_endonym_size);
}

QT_END_NAMESPACE
