/****************************************************************************
**
** Copyright (C) 2013 John Layt <jlayt@kde.org>
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

#include "qtimezone.h"
#include "qtimezoneprivate_p.h"

#include "qdatetime.h"

#include "qdebug.h"

QT_BEGIN_NAMESPACE

/*
    Private

    Windows system implementation
*/

#define MAX_KEY_LENGTH 255
#define FILETIME_UNIX_EPOCH Q_UINT64_C(116444736000000000)

// MSDN home page for Time support
// http://msdn.microsoft.com/en-us/library/windows/desktop/ms724962%28v=vs.85%29.aspx

// For Windows XP and later refer to MSDN docs on TIME_ZONE_INFORMATION structure
// http://msdn.microsoft.com/en-gb/library/windows/desktop/ms725481%28v=vs.85%29.aspx

// Vista introduced support for historic data, see MSDN docs on DYNAMIC_TIME_ZONE_INFORMATION
// http://msdn.microsoft.com/en-gb/library/windows/desktop/ms724253%28v=vs.85%29.aspx

static const char tzRegPath[] = "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Time Zones";
static const char currTzRegPath[] = "SYSTEM\\CurrentControlSet\\Control\\TimeZoneInformation";

enum {
    MIN_YEAR = -292275056,
    MAX_YEAR = 292278994,
    MSECS_PER_DAY = 86400000,
    TIME_T_MAX = 2145916799,  // int maximum 2037-12-31T23:59:59 UTC
    JULIAN_DAY_FOR_EPOCH = 2440588 // result of julianDayFromDate(1970, 1, 1)
};

// Copied from MSDN, see above for link
typedef struct _REG_TZI_FORMAT
{
    LONG Bias;
    LONG StandardBias;
    LONG DaylightBias;
    SYSTEMTIME StandardDate;
    SYSTEMTIME DaylightDate;
} REG_TZI_FORMAT;

// Fast and reliable conversion from msecs to date for all values
// Adapted from QDateTime msecsToDate
static QDate msecsToDate(qint64 msecs)
{
    qint64 jd = JULIAN_DAY_FOR_EPOCH;

    if (qAbs(msecs) >= MSECS_PER_DAY) {
        jd += (msecs / MSECS_PER_DAY);
        msecs %= MSECS_PER_DAY;
    }

    if (msecs < 0) {
        qint64 ds = MSECS_PER_DAY - msecs - 1;
        jd -= ds / MSECS_PER_DAY;
    }

    return QDate::fromJulianDay(jd);
}

static bool equalSystemtime(const SYSTEMTIME &t1, const SYSTEMTIME &t2)
{
    return (t1.wYear == t2.wYear
            && t1.wMonth == t2.wMonth
            && t1.wDay == t2.wDay
            && t1.wDayOfWeek == t2.wDayOfWeek
            && t1.wHour == t2.wHour
            && t1.wMinute == t2.wMinute
            && t1.wSecond == t2.wSecond
            && t1.wMilliseconds == t2.wMilliseconds);
}

static bool equalTzi(const TIME_ZONE_INFORMATION &tzi1, const TIME_ZONE_INFORMATION &tzi2)
{
    return(tzi1.Bias == tzi2.Bias
           && tzi1.StandardBias == tzi2.StandardBias
           && equalSystemtime(tzi1.StandardDate, tzi2.StandardDate)
           && wcscmp(tzi1.StandardName, tzi2.StandardName) == 0
           && tzi1.DaylightBias == tzi2.DaylightBias
           && equalSystemtime(tzi1.DaylightDate, tzi2.DaylightDate)
           && wcscmp(tzi1.DaylightName, tzi2.DaylightName) == 0);
}

static bool openRegistryKey(const QString &keyPath, HKEY *key)
{
    return (RegOpenKeyEx(HKEY_LOCAL_MACHINE, (const wchar_t*)keyPath.utf16(), 0, KEY_READ, key)
            == ERROR_SUCCESS);
}

static QString readRegistryString(const HKEY &key, const wchar_t *value)
{
    wchar_t buffer[MAX_PATH] = {0};
    DWORD size = sizeof(wchar_t) * MAX_PATH;
    RegQueryValueEx(key, (LPCWSTR)value, NULL, NULL, (LPBYTE)buffer, &size);
    return QString::fromWCharArray(buffer);
}

static int readRegistryValue(const HKEY &key, const wchar_t *value)
{
    DWORD buffer;
    DWORD size = sizeof(buffer);
    RegQueryValueEx(key, (LPCWSTR)value, NULL, NULL, (LPBYTE)&buffer, &size);
    return buffer;
}

static QWinTimeZonePrivate::QWinTransitionRule readRegistryRule(const HKEY &key,
                                                                const wchar_t *value, bool *ok)
{
    *ok = false;
    QWinTimeZonePrivate::QWinTransitionRule rule;
    REG_TZI_FORMAT tzi;
    DWORD tziSize = sizeof(tzi);
    if (RegQueryValueEx(key, (LPCWSTR)value, NULL, NULL, (BYTE *)&tzi, &tziSize)
        == ERROR_SUCCESS) {
        rule.startYear = 0;
        rule.standardTimeBias = tzi.Bias + tzi.StandardBias;
        rule.daylightTimeBias = tzi.Bias + tzi.DaylightBias - rule.standardTimeBias;
        rule.standardTimeRule = tzi.StandardDate;
        rule.daylightTimeRule = tzi.DaylightDate;
        *ok = true;
    }
    return rule;
}

static TIME_ZONE_INFORMATION getRegistryTzi(const QByteArray &windowsId, bool *ok)
{
    *ok = false;
    TIME_ZONE_INFORMATION tzi;
    REG_TZI_FORMAT regTzi;
    DWORD regTziSize = sizeof(regTzi);
    HKEY key = NULL;
    const QString tziKeyPath = QString::fromUtf8(tzRegPath) + QLatin1Char('\\')
                               + QString::fromUtf8(windowsId);

    if (openRegistryKey(tziKeyPath, &key)) {

        DWORD size = sizeof(tzi.DaylightName);
        RegQueryValueEx(key, L"Dlt", NULL, NULL, (LPBYTE)tzi.DaylightName, &size);

        size = sizeof(tzi.StandardName);
        RegQueryValueEx(key, L"Std", NULL, NULL, (LPBYTE)tzi.StandardName, &size);

        if (RegQueryValueEx(key, L"TZI", NULL, NULL, (BYTE *) &regTzi, &regTziSize)
            == ERROR_SUCCESS) {
            tzi.Bias = regTzi.Bias;
            tzi.StandardBias = regTzi.StandardBias;
            tzi.DaylightBias = regTzi.DaylightBias;
            tzi.StandardDate = regTzi.StandardDate;
            tzi.DaylightDate = regTzi.DaylightDate;
            *ok = true;
        }

        RegCloseKey(key);
    }

    return tzi;
}

static QList<QByteArray> availableWindowsIds()
{
    // TODO Consider caching results in a global static, very unlikely to change.
    QList<QByteArray> list;
    HKEY key = NULL;
    if (openRegistryKey(QString::fromUtf8(tzRegPath), &key)) {
        DWORD idCount = 0;
        if (RegQueryInfoKey(key, 0, 0, 0, &idCount, 0, 0, 0, 0, 0, 0, 0) == ERROR_SUCCESS
            && idCount > 0) {
            for (DWORD i = 0; i < idCount; ++i) {
                DWORD maxLen = MAX_KEY_LENGTH;
                TCHAR buffer[MAX_KEY_LENGTH];
                if (RegEnumKeyEx(key, i, buffer, &maxLen, 0, 0, 0, 0) == ERROR_SUCCESS)
                    list.append(QString::fromWCharArray(buffer).toUtf8());
            }
        }
        RegCloseKey(key);
    }
    return list;
}

static QByteArray windowsSystemZoneId()
{
    // On Vista and later is held in the value TimeZoneKeyName in key currTzRegPath
    QString id;
    HKEY key = NULL;
    QString tziKeyPath = QString::fromUtf8(currTzRegPath);
    if (openRegistryKey(tziKeyPath, &key)) {
        id = readRegistryString(key, L"TimeZoneKeyName");
        RegCloseKey(key);
        if (!id.isEmpty())
            return id.toUtf8();
    }

    // On XP we have to iterate over the zones until we find a match on
    // names/offsets with the current data
    TIME_ZONE_INFORMATION sysTzi;
    GetTimeZoneInformation(&sysTzi);
    bool ok = false;
    foreach (const QByteArray &winId, availableWindowsIds()) {
        if (equalTzi(getRegistryTzi(winId, &ok), sysTzi))
            return winId;
    }

    // If we can't determine the current ID use UTC
    return QByteArrayLiteral("UTC");
}

static QDate calculateTransitionLocalDate(const SYSTEMTIME &rule, int year)
{
    // If month is 0 then there is no date
    if (rule.wMonth == 0)
        return QDate();

    SYSTEMTIME time = rule;
    // If the year isn't set, then the rule date is relative
    if (time.wYear == 0) {
        if (time.wDayOfWeek == 0)
            time.wDayOfWeek = 7;
        QDate date(year, time.wMonth, 1);
        int startDow = date.dayOfWeek();
        if (startDow <= time.wDayOfWeek)
            date = date.addDays(time.wDayOfWeek - startDow - 7);
        else
            date = date.addDays(time.wDayOfWeek - startDow);
        date = date.addDays(time.wDay * 7);
        while (date.month() != time.wMonth)
            date = date.addDays(-7);
        return date;
    }

    // If the year is set then is an absolute date
    return QDate(time.wYear, time.wMonth, time.wDay);
}

// Converts a date/time value into msecs
static inline qint64 timeToMSecs(const QDate &date, const QTime &time)
{
    return ((date.toJulianDay() - JULIAN_DAY_FOR_EPOCH) * MSECS_PER_DAY)
           + time.msecsSinceStartOfDay();
}

static void calculateTransitionsForYear(const QWinTimeZonePrivate::QWinTransitionRule &rule, int year,
                                       qint64 *stdMSecs, qint64 *dstMSecs)
{
    // TODO Consider caching the calculated values
    // The local time in Daylight Time when switches to Standard Time
    QDate standardDate = calculateTransitionLocalDate(rule.standardTimeRule, year);
    QTime standardTime = QTime(rule.standardTimeRule.wHour, rule.standardTimeRule.wMinute,
                               rule.standardTimeRule.wSecond);
    if (standardDate.isValid() && standardTime.isValid()) {
        *stdMSecs = timeToMSecs(standardDate, standardTime)
                    + ((rule.standardTimeBias + rule.daylightTimeBias)  * 60000);
    } else {
        *stdMSecs = QTimeZonePrivate::invalidMSecs();
    }

    // The local time in Standard Time when switches to Daylight Time
    QDate daylightDate = calculateTransitionLocalDate(rule.daylightTimeRule, year);
    QTime daylightTime = QTime(rule.daylightTimeRule.wHour, rule.daylightTimeRule.wMinute,
                               rule.daylightTimeRule.wSecond);
    if (daylightDate.isValid() && daylightTime.isValid())
        *dstMSecs = timeToMSecs(daylightDate, daylightTime) + (rule.standardTimeBias * 60000);
    else
        *dstMSecs = QTimeZonePrivate::invalidMSecs();
}

static QLocale::Country userCountry()
{
#if defined(Q_OS_WINCE)
    // Guess that the syslem locale country is the right one to use
    // TODO Find if WinCE has equivalent api
    return QLocale::system().country();
#else
    const GEOID id = GetUserGeoID(GEOCLASS_NATION);
    wchar_t code[3];
    const int size = GetGeoInfo(id, GEO_ISO2, code, 3, 0);
    return (size == 3) ? QLocalePrivate::codeToCountry(QString::fromWCharArray(code))
                       : QLocale::AnyCountry;
#endif // Q_OS_WINCE
}

// Create the system default time zone
QWinTimeZonePrivate::QWinTimeZonePrivate()
                   : QTimeZonePrivate()
{
    init(QByteArray());
}

// Create a named time zone
QWinTimeZonePrivate::QWinTimeZonePrivate(const QByteArray &ianaId)
                   : QTimeZonePrivate()
{
    init(ianaId);
}

QWinTimeZonePrivate::QWinTimeZonePrivate(const QWinTimeZonePrivate &other)
                   : QTimeZonePrivate(other), m_windowsId(other.m_windowsId),
                     m_displayName(other.m_displayName), m_standardName(other.m_standardName),
                     m_daylightName(other.m_daylightName), m_tranRules(other.m_tranRules)
{
}

QWinTimeZonePrivate::~QWinTimeZonePrivate()
{
}

QTimeZonePrivate *QWinTimeZonePrivate::clone()
{
    return new QWinTimeZonePrivate(*this);
}

void QWinTimeZonePrivate::init(const QByteArray &ianaId)
{
    if (ianaId.isEmpty()) {
        m_windowsId = windowsSystemZoneId();
        m_id = systemTimeZoneId();
    } else {
        m_windowsId = ianaIdToWindowsId(ianaId);
        m_id = ianaId;
    }

    if (!m_windowsId.isEmpty()) {
        // Open the base TZI for the time zone
        HKEY baseKey = NULL;
        const QString baseKeyPath = QString::fromUtf8(tzRegPath) + QLatin1Char('\\')
                                   + QString::fromUtf8(m_windowsId);
        if (openRegistryKey(baseKeyPath, &baseKey)) {
            //  Load the localized names
            m_displayName = readRegistryString(baseKey, L"Display");
            m_standardName = readRegistryString(baseKey, L"Std");
            m_daylightName = readRegistryString(baseKey, L"Dlt");
            // On Vista and later the optional dynamic key holds historic data
            const QString dynamicKeyPath = baseKeyPath + QStringLiteral("\\Dynamic DST");
            HKEY dynamicKey = NULL;
            if (openRegistryKey(dynamicKeyPath, &dynamicKey)) {
                // Find out the start and end years stored, then iterate over them
                int startYear = readRegistryValue(dynamicKey, L"FirstEntry");
                int endYear = readRegistryValue(dynamicKey, L"LastEntry");
                for (int year = startYear; year <= endYear; ++year) {
                    bool ruleOk;
                    QWinTransitionRule rule = readRegistryRule(dynamicKey,
                                                               (LPCWSTR)QString::number(year).utf16(),
                                                               &ruleOk);
                    rule.startYear = year;
                    if (ruleOk)
                        m_tranRules.append(rule);
                }
                RegCloseKey(dynamicKey);
            } else {
                // No dynamic data so use the base data
                bool ruleOk;
                QWinTransitionRule rule = readRegistryRule(baseKey, L"TZI", &ruleOk);
                rule.startYear = 1970;
                if (ruleOk)
                    m_tranRules.append(rule);
            }
            RegCloseKey(baseKey);
        }
    }

    // If there are no rules then we failed to find a windowsId or any tzi info
    if (m_tranRules.size() == 0) {
        m_id.clear();
        m_windowsId.clear();
        m_displayName.clear();
    }
}

QString QWinTimeZonePrivate::comment() const
{
    return m_displayName;
}

QString QWinTimeZonePrivate::displayName(QTimeZone::TimeType timeType,
                                         QTimeZone::NameType nameType,
                                         const QLocale &locale) const
{
    // TODO Registry holds MUI keys, should be able to look up translations?
    Q_UNUSED(locale);

    if (nameType == QTimeZone::OffsetName) {
        QWinTransitionRule rule = ruleForYear(QDate::currentDate().year());
        if (timeType == QTimeZone::DaylightTime)
            return isoOffsetFormat((rule.standardTimeBias + rule.daylightTimeBias) * -60);
        else
            return isoOffsetFormat((rule.standardTimeBias) * -60);
    }

    switch (timeType) {
    case  QTimeZone::DaylightTime :
        return m_daylightName;
    case  QTimeZone::GenericTime :
        return m_displayName;
    case  QTimeZone::StandardTime :
        return m_standardName;
    }
    return m_standardName;
}

QString QWinTimeZonePrivate::abbreviation(qint64 atMSecsSinceEpoch) const
{
    return data(atMSecsSinceEpoch).abbreviation;
}

int QWinTimeZonePrivate::offsetFromUtc(qint64 atMSecsSinceEpoch) const
{
    return data(atMSecsSinceEpoch).offsetFromUtc;
}

int QWinTimeZonePrivate::standardTimeOffset(qint64 atMSecsSinceEpoch) const
{
    return data(atMSecsSinceEpoch).standardTimeOffset;
}

int QWinTimeZonePrivate::daylightTimeOffset(qint64 atMSecsSinceEpoch) const
{
    return data(atMSecsSinceEpoch).daylightTimeOffset;
}

bool QWinTimeZonePrivate::hasDaylightTime() const
{
    return hasTransitions();
}

bool QWinTimeZonePrivate::isDaylightTime(qint64 atMSecsSinceEpoch) const
{
    return (data(atMSecsSinceEpoch).daylightTimeOffset != 0);
}

QTimeZonePrivate::Data QWinTimeZonePrivate::data(qint64 forMSecsSinceEpoch) const
{
    // Convert MSecs to year to get transitions for, assumes no transitions around 31 Dec/1 Jan
    int year = msecsToDate(forMSecsSinceEpoch).year();

    qint64 first;
    qint64 second;
    qint64 next = maxMSecs();
    qint64 stdMSecs;
    qint64 dstMSecs;
    QWinTransitionRule rule;
    do {
        // Convert the transition rules into msecs for the year we want to try
        rule = ruleForYear(year);
        // If no transition rules to calculate then no DST, so just use rule for std
        if (rule.standardTimeRule.wMonth == 0 && rule.daylightTimeRule.wMonth == 0)
            break;
        calculateTransitionsForYear(rule, year, &stdMSecs, &dstMSecs);
        if (stdMSecs < dstMSecs) {
            first = stdMSecs;
            second = dstMSecs;
        } else {
            first = dstMSecs;
            second = stdMSecs;
        }
        if (forMSecsSinceEpoch >= second && second != invalidMSecs())
            next = second;
        else if (forMSecsSinceEpoch >= first && first != invalidMSecs())
            next = first;
        // If didn't fall in this year, try the previous
        --year;
    } while (next == maxMSecs() && year >= MIN_YEAR);

    return ruleToData(rule, forMSecsSinceEpoch, (next == dstMSecs) ? QTimeZone::DaylightTime : QTimeZone::StandardTime);
}

bool QWinTimeZonePrivate::hasTransitions() const
{
    foreach (const QWinTransitionRule &rule, m_tranRules) {
        if (rule.standardTimeRule.wMonth > 0 && rule.daylightTimeRule.wMonth > 0)
            return true;
    }
    return false;
}

QTimeZonePrivate::Data QWinTimeZonePrivate::nextTransition(qint64 afterMSecsSinceEpoch) const
{
    // Convert MSecs to year to get transitions for, assumes no transitions around 31 Dec/1 Jan
    int year = msecsToDate(afterMSecsSinceEpoch).year();

    QWinTransitionRule rule;
    // If the required year falls after the last rule start year and the last rule has no
    // valid future transition calculations then there is no next transition
    if (year > m_tranRules.last().startYear) {
        rule = ruleForYear(year);
        // If the rules have either a fixed year, or no month, then no future trans
        if (rule.standardTimeRule.wYear != 0 || rule.daylightTimeRule.wYear != 0
            || rule.standardTimeRule.wMonth == 0 || rule.daylightTimeRule.wMonth == 0) {
            return invalidData();
        }
    }

    // Otherwise we have a valid rule for the required year that can be used
    // to calculate this year or next
    qint64 first;
    qint64 second;
    qint64 next = minMSecs();
    qint64 stdMSecs;
    qint64 dstMSecs;
    do {
        // Convert the transition rules into msecs for the year we want to try
        rule = ruleForYear(year);
        // If no transition rules to calculate then no next transition
        if (rule.standardTimeRule.wMonth == 0 && rule.daylightTimeRule.wMonth == 0)
            return invalidData();
        calculateTransitionsForYear(rule, year, &stdMSecs, &dstMSecs);
        // Find the first and second transition for the year
        if (stdMSecs < dstMSecs) {
            first = stdMSecs;
            second = dstMSecs;
        } else {
            first = dstMSecs;
            second = stdMSecs;
        }
        if (afterMSecsSinceEpoch < first)
            next = first;
        else if (afterMSecsSinceEpoch < second)
            next = second;
        // If didn't fall in this year, try the next
        ++year;
    } while (next == minMSecs() && year <= MAX_YEAR);

    if (next == minMSecs() || next == invalidMSecs())
        return invalidData();

    return ruleToData(rule, next, (next == dstMSecs) ? QTimeZone::DaylightTime : QTimeZone::StandardTime);
}

QTimeZonePrivate::Data QWinTimeZonePrivate::previousTransition(qint64 beforeMSecsSinceEpoch) const
{
    // Convert MSecs to year to get transitions for, assumes no transitions around 31 Dec/1 Jan
    int year = msecsToDate(beforeMSecsSinceEpoch).year();

    QWinTransitionRule rule;
    // If the required year falls before the first rule start year and the first rule has no
    // valid transition calculations then there is no previous transition
    if (year < m_tranRules.first().startYear) {
        rule = ruleForYear(year);
        // If the rules have either a fixed year, or no month, then no previous trans
        if (rule.standardTimeRule.wYear != 0 || rule.daylightTimeRule.wYear != 0
            || rule.standardTimeRule.wMonth == 0 || rule.daylightTimeRule.wMonth == 0) {
            return invalidData();
        }
    }

    qint64 first;
    qint64 second;
    qint64 next = maxMSecs();
    qint64 stdMSecs;
    qint64 dstMSecs;
    do {
        // Convert the transition rules into msecs for the year we want to try
        rule = ruleForYear(year);
        // If no transition rules to calculate then no previous transition
        if (rule.standardTimeRule.wMonth == 0 && rule.daylightTimeRule.wMonth == 0)
            return invalidData();
        calculateTransitionsForYear(rule, year, &stdMSecs, &dstMSecs);
        if (stdMSecs < dstMSecs) {
            first = stdMSecs;
            second = dstMSecs;
        } else {
            first = dstMSecs;
            second = stdMSecs;
        }
        if (beforeMSecsSinceEpoch > second && second != invalidMSecs())
            next = second;
        else if (beforeMSecsSinceEpoch > first && first != invalidMSecs())
            next = first;
        // If didn't fall in this year, try the previous
        --year;
    } while (next == maxMSecs() && year >= MIN_YEAR);

    if (next == maxMSecs())
        return invalidData();

    return ruleToData(rule, next, (next == dstMSecs) ? QTimeZone::DaylightTime : QTimeZone::StandardTime);
}

QByteArray QWinTimeZonePrivate::systemTimeZoneId() const
{
    const QLocale::Country country = userCountry();
    const QByteArray windowsId = windowsSystemZoneId();
    QByteArray ianaId;
    // If we have a real country, then try get a specific match for that country
    if (country != QLocale::AnyCountry)
        ianaId = windowsIdToDefaultIanaId(windowsId, country);
    // If we don't have a real country, or there wasn't a specific match, try the global default
    if (ianaId.isEmpty()) {
        ianaId = windowsIdToDefaultIanaId(windowsId);
        // If no global default then probably an unknown Windows ID so return UTC
        if (ianaId.isEmpty())
            return QByteArrayLiteral("UTC");
    }
    return ianaId;
}

QSet<QByteArray> QWinTimeZonePrivate::availableTimeZoneIds() const
{
    QSet<QByteArray> set;
    foreach (const QByteArray &winId, availableWindowsIds()) {
        foreach (const QByteArray &ianaId, windowsIdToIanaIds(winId))
            set << ianaId;
    }
    return set;
}

QWinTimeZonePrivate::QWinTransitionRule QWinTimeZonePrivate::ruleForYear(int year) const
{
    for (int i = m_tranRules.size() - 1; i >= 0; --i) {
        if (m_tranRules.at(i).startYear <= year)
            return m_tranRules.at(i);
    }
    return m_tranRules.at(0);
}

QTimeZonePrivate::Data QWinTimeZonePrivate::ruleToData(const QWinTransitionRule &rule,
                                                       qint64 atMSecsSinceEpoch,
                                                       QTimeZone::TimeType type) const
{
    QTimeZonePrivate::Data tran = QTimeZonePrivate::invalidData();
    tran.atMSecsSinceEpoch = atMSecsSinceEpoch;
    tran.standardTimeOffset = rule.standardTimeBias * -60;
    if (type == QTimeZone::DaylightTime) {
        tran.daylightTimeOffset = rule.daylightTimeBias * -60;
        tran.abbreviation = m_daylightName;
    } else {
        tran.daylightTimeOffset = 0;
        tran.abbreviation = m_standardName;
    }
    tran.offsetFromUtc = tran.standardTimeOffset + tran.daylightTimeOffset;
    return tran;
}

QT_END_NAMESPACE
