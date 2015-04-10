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

#include <QtCore/QFile>
#include <QtCore/QHash>
#include <QtCore/QDateTime>

#include <qdebug.h>


QT_BEGIN_NAMESPACE

/*
    Private

    tz file implementation
*/

struct QTzTimeZone {
    QLocale::Country country;
    QByteArray comment;
};

// Define as a type as Q_GLOBAL_STATIC doesn't like it
typedef QHash<QByteArray, QTzTimeZone> QTzTimeZoneHash;

// Parse zone.tab table, assume lists all installed zones, if not will need to read directories
static QTzTimeZoneHash loadTzTimeZones()
{
    QString path = QStringLiteral("/usr/share/zoneinfo/zone.tab");
    if (!QFile::exists(path))
        path = QStringLiteral("/usr/lib/zoneinfo/zone.tab");

    QFile tzif(path);
    if (!tzif.open(QIODevice::ReadOnly))
        return QTzTimeZoneHash();

    QTzTimeZoneHash zonesHash;
    // TODO QTextStream inefficient, replace later
    QTextStream ts(&tzif);
    while (!ts.atEnd()) {
        const QString line = ts.readLine();
        // Comment lines are prefixed with a #
        if (!line.isEmpty() && line.at(0) != '#') {
            // Data rows are tab-separated columns Region, Coordinates, ID, Optional Comments
            const QStringList parts = line.split('\t');
            QTzTimeZone zone;
            zone.country = QLocalePrivate::codeToCountry(parts.at(0));
            if (parts.size() > 3)
                zone.comment = parts.at(3).toUtf8();
            zonesHash.insert(parts.at(2).toUtf8(), zone);
        }
    }
    return zonesHash;
}

// Hash of available system tz files as loaded by loadTzTimeZones()
Q_GLOBAL_STATIC_WITH_ARGS(const QTzTimeZoneHash, tzZones, (loadTzTimeZones()));

/*
    The following is copied and modified from tzfile.h which is in the public domain.
    Copied as no compatibility guarantee and is never system installed.
    See https://github.com/eggert/tz/blob/master/tzfile.h
*/

#define TZ_MAGIC      "TZif"
#define TZ_MAX_TIMES  1200
#define TZ_MAX_TYPES   256  // Limited by what (unsigned char)'s can hold
#define TZ_MAX_CHARS    50  // Maximum number of abbreviation characters
#define TZ_MAX_LEAPS    50  // Maximum number of leap second corrections

struct QTzHeader {
    char       tzh_magic[4];        // TZ_MAGIC
    char       tzh_version;         // '\0' or '2' as of 2005
    char       tzh_reserved[15];    // reserved--must be zero
    quint32    tzh_ttisgmtcnt;      // number of trans. time flags
    quint32    tzh_ttisstdcnt;      // number of trans. time flags
    quint32    tzh_leapcnt;         // number of leap seconds
    quint32    tzh_timecnt;         // number of transition times
    quint32    tzh_typecnt;         // number of local time types
    quint32    tzh_charcnt;         // number of abbr. chars
};

struct QTzTransition {
    qint64 tz_time;     // Transition time
    quint8 tz_typeind;  // Type Index
};

struct QTzType {
    int tz_gmtoff;  // UTC offset in seconds
    bool   tz_isdst;   // Is DST
    quint8 tz_abbrind; // abbreviation list index
    bool   tz_ttisgmt; // Is in UTC time
    bool   tz_ttisstd; // Is in Standard time
};
Q_DECLARE_TYPEINFO(QTzType, Q_PRIMITIVE_TYPE);


// TZ File parsing

static QTzHeader parseTzHeader(QDataStream &ds, bool *ok)
{
    QTzHeader hdr;
    quint8 ch;
    *ok = false;

    // Parse Magic, 4 bytes
    ds.readRawData(hdr.tzh_magic, 4);

    if (memcmp(hdr.tzh_magic, TZ_MAGIC, 4) != 0 || ds.status() != QDataStream::Ok)
        return hdr;

    // Parse Version, 1 byte, before 2005 was '\0', since 2005 a '2', since 2013 a '3'
    ds >> ch;
    hdr.tzh_version = ch;
    if (ds.status() != QDataStream::Ok
        || (hdr.tzh_version != '2' && hdr.tzh_version != '\0' && hdr.tzh_version != '3')) {
        return hdr;
    }

    // Parse reserved space, 15 bytes
    ds.readRawData(hdr.tzh_reserved, 15);
    if (ds.status() != QDataStream::Ok)
        return hdr;

    // Parse rest of header, 6 x 4-byte transition counts
    ds >> hdr.tzh_ttisgmtcnt >> hdr.tzh_ttisstdcnt >> hdr.tzh_leapcnt >> hdr.tzh_timecnt
       >> hdr.tzh_typecnt >> hdr.tzh_charcnt;

    // Check defined maximums
    if (ds.status() != QDataStream::Ok
        || hdr.tzh_timecnt > TZ_MAX_TIMES
        || hdr.tzh_typecnt > TZ_MAX_TYPES
        || hdr.tzh_charcnt > TZ_MAX_CHARS
        || hdr.tzh_leapcnt > TZ_MAX_LEAPS
        || hdr.tzh_ttisgmtcnt > hdr.tzh_typecnt
        || hdr.tzh_ttisstdcnt > hdr.tzh_typecnt) {
        return hdr;
    }

    *ok = true;
    return hdr;
}

static QList<QTzTransition> parseTzTransitions(QDataStream &ds, int tzh_timecnt, bool longTran)
{
    QList<QTzTransition> tranList;

    if (longTran) {
        // Parse tzh_timecnt x 8-byte transition times
        for (int i = 0; i < tzh_timecnt && ds.status() == QDataStream::Ok; ++i) {
            QTzTransition tran;
            ds >> tran.tz_time;
            if (ds.status() == QDataStream::Ok)
                tranList.append(tran);
        }
    } else {
        // Parse tzh_timecnt x 4-byte transition times
        int val;
        for (int i = 0; i < tzh_timecnt && ds.status() == QDataStream::Ok; ++i) {
            QTzTransition tran;
            ds >> val;
            tran.tz_time = val;
            if (ds.status() == QDataStream::Ok)
                tranList.append(tran);
        }
    }

    // Parse tzh_timecnt x 1-byte transition type index
    for (int i = 0; i < tzh_timecnt && ds.status() == QDataStream::Ok; ++i) {
        quint8 typeind;
        ds >> typeind;
        if (ds.status() == QDataStream::Ok)
            tranList[i].tz_typeind = typeind;
    }

    return tranList;
}

static QList<QTzType> parseTzTypes(QDataStream &ds, int tzh_typecnt)
{
    QList<QTzType> typeList;
    // Parse tzh_typecnt x transition types
    for (int i = 0; i < tzh_typecnt && ds.status() == QDataStream::Ok; ++i) {
        QTzType type;
        // Parse UTC Offset, 4 bytes
        ds >> type.tz_gmtoff;
        // Parse Is DST flag, 1 byte
        if (ds.status() == QDataStream::Ok)
            ds >> type.tz_isdst;
        // Parse Abbreviation Array Index, 1 byte
        if (ds.status() == QDataStream::Ok)
            ds >> type.tz_abbrind;
        // Set defaults in case not populated later
        type.tz_ttisgmt = false;
        type.tz_ttisstd = false;
        if (ds.status() == QDataStream::Ok)
            typeList.append(type);
    }

    return typeList;
}

static QMap<int, QByteArray> parseTzAbbreviations(QDataStream &ds, int tzh_charcnt, QList<QTzType> typeList)
{
    // Parse the abbreviation list which is tzh_charcnt long with '\0' separated strings. The
    // QTzType.tz_abbrind index points to the first char of the abbreviation in the array, not the
    // occurrence in the list. It can also point to a partial string so we need to use the actual typeList
    // index values when parsing.  By using a map with tz_abbrind as ordered key we get both index
    // methods in one data structure and can convert the types afterwards.
    QMap<int, QByteArray> map;
    quint8 ch;
    QByteArray input;
    // First parse the full abbrev string
    for (int i = 0; i < tzh_charcnt && ds.status() == QDataStream::Ok; ++i) {
        ds >> ch;
        if (ds.status() == QDataStream::Ok)
            input.append(char(ch));
        else
            return map;
    }
    // Then extract all the substrings pointed to by typeList
    foreach (const QTzType type, typeList) {
        QByteArray abbrev;
        for (int i = type.tz_abbrind; input.at(i) != '\0'; ++i)
            abbrev.append(input.at(i));
        // Have reached end of an abbreviation, so add to map
        map[type.tz_abbrind] = abbrev;
    }
    return map;
}

static void parseTzLeapSeconds(QDataStream &ds, int tzh_leapcnt, bool longTran)
{
    // Parse tzh_leapcnt x pairs of leap seconds
    // We don't use leap seconds, so only read and don't store
    qint64 val;
    if (longTran) {
        qint64 time;
        for (int i = 0; i < tzh_leapcnt && ds.status() == QDataStream::Ok; ++i) {
            // Parse Leap Occurrence Time, 8 bytes
            ds >> time;
            // Parse Leap Seconds To Apply, 4 bytes
            if (ds.status() == QDataStream::Ok)
                ds >> val;
        }
    } else {
        for (int i = 0; i < tzh_leapcnt && ds.status() == QDataStream::Ok; ++i) {
            // Parse Leap Occurrence Time, 4 bytes
            ds >> val;
            // Parse Leap Seconds To Apply, 4 bytes
            if (ds.status() == QDataStream::Ok)
                ds >> val;
        }
    }
}

static QList<QTzType> parseTzIndicators(QDataStream &ds, const QList<QTzType> &typeList, int tzh_ttisstdcnt, int tzh_ttisgmtcnt)
{
    QList<QTzType> list = typeList;
    bool temp;

    // Parse tzh_ttisstdcnt x 1-byte standard/wall indicators
    for (int i = 0; i < tzh_ttisstdcnt && ds.status() == QDataStream::Ok; ++i) {
        ds >> temp;
        if (ds.status() == QDataStream::Ok)
            list[i].tz_ttisstd = temp;
    }

    // Parse tzh_ttisgmtcnt x 1-byte UTC/local indicators
    for (int i = 0; i < tzh_ttisgmtcnt && ds.status() == QDataStream::Ok; ++i) {
        ds >> temp;
        if (ds.status() == QDataStream::Ok)
            list[i].tz_ttisgmt = temp;
    }

    return list;
}

static QByteArray parseTzPosixRule(QDataStream &ds)
{
    // Parse POSIX rule, variable length '\n' enclosed
    QByteArray rule;

    quint8 ch;
    ds >> ch;
    if (ch != '\n' || ds.status() != QDataStream::Ok)
        return rule;
    ds >> ch;
    while (ch != '\n' && ds.status() == QDataStream::Ok) {
        rule.append((char)ch);
        ds >> ch;
    }

    return rule;
}

static QDate calculateDowDate(int year, int month, int dayOfWeek, int week)
{
    QDate date(year, month, 1);
    int startDow = date.dayOfWeek();
    if (startDow <= dayOfWeek)
        date = date.addDays(dayOfWeek - startDow - 7);
    else
        date = date.addDays(dayOfWeek - startDow);
    date = date.addDays(week * 7);
    while (date.month() != month)
        date = date.addDays(-7);
    return date;
}

static QDate calculatePosixDate(const QByteArray dateRule, int year)
{
    // Can start with M, J, or a digit
    if (dateRule.at(0) == 'M') {
        // nth week in month format "Mmonth.week.dow"
        QList<QByteArray> dateParts = dateRule.split('.');
        int month = dateParts.at(0).mid(1).toInt();
        int week = dateParts.at(1).toInt();
        int dow = dateParts.at(2).toInt();
        if (dow == 0)
            ++dow;
        return calculateDowDate(year, month, dow, week);
    } else if (dateRule.at(0) == 'J') {
        // Day of Year ignores Feb 29
        int doy = dateRule.mid(1).toInt();
        QDate date = QDate(year, 1, 1).addDays(doy - 1);
        if (QDate::isLeapYear(date.year()))
            date = date.addDays(-1);
        return date;
    } else {
        // Day of Year includes Feb 29
        int doy = dateRule.toInt();
        return QDate(year, 1, 1).addDays(doy - 1);
    }
}

static QTime parsePosixTime(const QByteArray &timeRule)
{
    // Format "HH:mm:ss", put check parts count just in case
    QList<QByteArray> parts = timeRule.split(':');
    int count = parts.count();
    if (count == 3)
        return QTime(parts.at(0).toInt(), parts.at(1).toInt(), parts.at(2).toInt());
    else if (count == 2)
        return QTime(parts.at(0).toInt(), parts.at(1).toInt(), 0);
    else if (count == 1)
        return QTime(parts.at(0).toInt(), 0, 0);
    return QTime(2, 0, 0);
}

static int parsePosixOffset(const QByteArray &timeRule)
{
    // Format "[+|-]hh[:mm[:ss]]"
    QList<QByteArray> parts = timeRule.split(':');
    int count = parts.count();
    if (count == 3)
        return (parts.at(0).toInt() * -60 * 60) + (parts.at(1).toInt() * 60) + parts.at(2).toInt();
    else if (count == 2)
        return (parts.at(0).toInt() * -60 * 60) + (parts.at(1).toInt() * 60);
    else if (count == 1)
        return (parts.at(0).toInt() * -60 * 60);
    return 0;
}

static QList<QTimeZonePrivate::Data> calculatePosixTransitions(const QByteArray &posixRule,
                                                               int startYear, int endYear,
                                                               int lastTranMSecs)
{
    QList<QTimeZonePrivate::Data> list;

    // Limit year by qint64 max size for msecs
    if (startYear > 292278994)
        startYear = 292278994;
    if (endYear > 292278994)
        endYear = 292278994;

    // POSIX Format is like "TZ=CST6CDT,M3.2.0/2:00:00,M11.1.0/2:00:00"
    // i.e. "std offset dst [offset],start[/time],end[/time]"
    // See http://www.gnu.org/software/libc/manual/html_node/TZ-Variable.html
    QList<QByteArray> parts = posixRule.split(',');

    QString name = QString::fromUtf8(parts.at(0));
    QString stdName;
    QString stdOffsetString;
    QString dstName;
    QString dstOffsetString;
    bool parsedStdName = false;
    bool parsedStdOffset = false;
    for (int i = 0; i < name.size(); ++i) {
        if (name.at(i).isLetter()) {
            if (parsedStdName) {
                parsedStdOffset = true;
                dstName.append(name.at(i));
            } else {
                stdName.append(name.at(i));
            }
        } else {
            parsedStdName = true;
            if (parsedStdOffset)
                dstOffsetString.append(name.at(i));
            else
                stdOffsetString.append(name.at(i));
        }
    }

    int utcOffset = parsePosixOffset(stdOffsetString.toUtf8());

    // If only the name part then no transitions
    if (parts.count() == 1) {
        QTimeZonePrivate::Data data;
        data.atMSecsSinceEpoch = lastTranMSecs;
        data.offsetFromUtc = utcOffset;
        data.standardTimeOffset = utcOffset;
        data.daylightTimeOffset = 0;
        data.abbreviation = stdName;
        list << data;
        return list;
    }

    // If not populated the total dst offset is 1 hour
    int dstOffset = utcOffset + (60 * 60);
    if (!dstOffsetString.isEmpty())
        dstOffset = parsePosixOffset(dstOffsetString.toUtf8());

    // Get the std to dst transtion details
    QList<QByteArray> dstParts = parts.at(1).split('/');
    QByteArray dstDateRule = dstParts.at(0);
    QTime dstTime;
    if (dstParts.count() > 1)
        dstTime = parsePosixTime(dstParts.at(1));
    else
        dstTime = QTime(2, 0, 0);

    // Get the dst to std transtion details
    QList<QByteArray> stdParts = parts.at(2).split('/');
    QByteArray stdDateRule = stdParts.at(0);
    QTime stdTime;
    if (stdParts.count() > 1)
        stdTime = parsePosixTime(stdParts.at(1));
    else
        stdTime = QTime(2, 0, 0);

    for (int year = startYear; year <= endYear; ++year) {
        QTimeZonePrivate::Data dstData;
        QDateTime dst(calculatePosixDate(dstDateRule, year), dstTime, Qt::UTC);
        dstData.atMSecsSinceEpoch = dst.toMSecsSinceEpoch() - (utcOffset * 1000);
        dstData.offsetFromUtc = dstOffset;
        dstData.standardTimeOffset = utcOffset;
        dstData.daylightTimeOffset = dstOffset - utcOffset;
        dstData.abbreviation = dstName;
        QTimeZonePrivate::Data stdData;
        QDateTime std(calculatePosixDate(stdDateRule, year), stdTime, Qt::UTC);
        stdData.atMSecsSinceEpoch = std.toMSecsSinceEpoch() - (dstOffset * 1000);
        stdData.offsetFromUtc = utcOffset;
        stdData.standardTimeOffset = utcOffset;
        stdData.daylightTimeOffset = 0;
        stdData.abbreviation = stdName;
        // Part of the high year will overflow
        if (year == 292278994 && (dstData.atMSecsSinceEpoch < 0 || stdData.atMSecsSinceEpoch < 0)) {
            if (dstData.atMSecsSinceEpoch > 0) {
                list << dstData;
            } else if (stdData.atMSecsSinceEpoch > 0) {
                list << stdData;
            }
        } else if (dst < std) {
            list << dstData << stdData;
        } else {
            list << stdData << dstData;
        }
    }
    return list;
}

// Create the system default time zone
QTzTimeZonePrivate::QTzTimeZonePrivate()
#ifdef QT_USE_ICU
    : m_icu(0)
#endif // QT_USE_ICU
{
    init(systemTimeZoneId());
}

// Create a named time zone
QTzTimeZonePrivate::QTzTimeZonePrivate(const QByteArray &ianaId)
#ifdef QT_USE_ICU
    : m_icu(0)
#endif // QT_USE_ICU
{
    init(ianaId);
}

QTzTimeZonePrivate::QTzTimeZonePrivate(const QTzTimeZonePrivate &other)
                  : QTimeZonePrivate(other), m_tranTimes(other.m_tranTimes),
                    m_tranRules(other.m_tranRules), m_abbreviations(other.m_abbreviations),
#ifdef QT_USE_ICU
                    m_icu(other.m_icu),
#endif // QT_USE_ICU
                    m_posixRule(other.m_posixRule)
{
}

QTzTimeZonePrivate::~QTzTimeZonePrivate()
{
}

QTimeZonePrivate *QTzTimeZonePrivate::clone()
{
    return new QTzTimeZonePrivate(*this);
}

void QTzTimeZonePrivate::init(const QByteArray &ianaId)
{
    QFile tzif;
    if (ianaId.isEmpty()) {
        // Open system tz
        tzif.setFileName(QStringLiteral("/etc/localtime"));
        if (!tzif.open(QIODevice::ReadOnly))
            return;
    } else {
        // Open named tz, try modern path first, if fails try legacy path
        tzif.setFileName(QLatin1String("/usr/share/zoneinfo/") + QString::fromLocal8Bit(ianaId));
        if (!tzif.open(QIODevice::ReadOnly)) {
            tzif.setFileName(QLatin1String("/usr/lib/zoneinfo/") + QString::fromLocal8Bit(ianaId));
            if (!tzif.open(QIODevice::ReadOnly))
                return;
        }
    }

    QDataStream ds(&tzif);

    // Parse the old version block of data
    bool ok = false;
    QTzHeader hdr = parseTzHeader(ds, &ok);
    if (!ok || ds.status() != QDataStream::Ok)
        return;
    QList<QTzTransition> tranList = parseTzTransitions(ds, hdr.tzh_timecnt, false);
    if (ds.status() != QDataStream::Ok)
        return;
    QList<QTzType> typeList = parseTzTypes(ds, hdr.tzh_typecnt);
    if (ds.status() != QDataStream::Ok)
        return;
    QMap<int, QByteArray> abbrevMap = parseTzAbbreviations(ds, hdr.tzh_charcnt, typeList);
    if (ds.status() != QDataStream::Ok)
        return;
    parseTzLeapSeconds(ds, hdr.tzh_leapcnt, false);
    if (ds.status() != QDataStream::Ok)
        return;
    typeList = parseTzIndicators(ds, typeList, hdr.tzh_ttisstdcnt, hdr.tzh_ttisgmtcnt);
    if (ds.status() != QDataStream::Ok)
        return;

    // If version 2 then parse the second block of data
    if (hdr.tzh_version == '2' || hdr.tzh_version == '3') {
        ok = false;
        QTzHeader hdr2 = parseTzHeader(ds, &ok);
        if (!ok || ds.status() != QDataStream::Ok)
            return;
        tranList = parseTzTransitions(ds, hdr2.tzh_timecnt, true);
        if (ds.status() != QDataStream::Ok)
            return;
        typeList = parseTzTypes(ds, hdr2.tzh_typecnt);
        if (ds.status() != QDataStream::Ok)
            return;
        abbrevMap = parseTzAbbreviations(ds, hdr2.tzh_charcnt, typeList);
        if (ds.status() != QDataStream::Ok)
            return;
        parseTzLeapSeconds(ds, hdr2.tzh_leapcnt, true);
        if (ds.status() != QDataStream::Ok)
            return;
        typeList = parseTzIndicators(ds, typeList, hdr2.tzh_ttisstdcnt, hdr2.tzh_ttisgmtcnt);
        if (ds.status() != QDataStream::Ok)
            return;
        m_posixRule = parseTzPosixRule(ds);
        if (ds.status() != QDataStream::Ok)
            return;
    }

    // Translate the TZ file into internal format

    // Translate the array index based tz_abbrind into list index
    m_abbreviations = abbrevMap.values();
    QList<int> abbrindList = abbrevMap.keys();
    for (int i = 0; i < typeList.size(); ++i)
        typeList[i].tz_abbrind = abbrindList.indexOf(typeList.at(i).tz_abbrind);

    // Offsets are stored as total offset, want to know separate UTC and DST offsets
    // so find the first non-dst transition to use as base UTC Offset
    int utcOffset = 0;
    foreach (const QTzTransition &tran, tranList) {
        if (!typeList.at(tran.tz_typeind).tz_isdst) {
            utcOffset = typeList.at(tran.tz_typeind).tz_gmtoff;
            break;
        }
    }

    // Now for each transition time calculate our rule and save them
    foreach (const QTzTransition &tz_tran, tranList) {
        QTzTransitionTime tran;
        QTzTransitionRule rule;
        const QTzType tz_type = typeList.at(tz_tran.tz_typeind);

        // Calculate the associated Rule
        if (!tz_type.tz_isdst)
            utcOffset = tz_type.tz_gmtoff;
        rule.stdOffset = utcOffset;
        rule.dstOffset = tz_type.tz_gmtoff - utcOffset;
        rule.abbreviationIndex = tz_type.tz_abbrind;
        // If the rule already exist then use that, otherwise add it
        int ruleIndex = m_tranRules.indexOf(rule);
        if (ruleIndex == -1) {
            m_tranRules.append(rule);
            tran.ruleIndex = m_tranRules.size() - 1;
        } else {
            tran.ruleIndex = ruleIndex;
        }

        // TODO convert to UTC if not in UTC
        if (tz_type.tz_ttisgmt)
            tran.atMSecsSinceEpoch = tz_tran.tz_time * 1000;
        else if (tz_type.tz_ttisstd)
            tran.atMSecsSinceEpoch = tz_tran.tz_time * 1000;
        else
            tran.atMSecsSinceEpoch = tz_tran.tz_time * 1000;

        m_tranTimes.append(tran);
    }

    if (ianaId.isEmpty())
        m_id = systemTimeZoneId();
    else
        m_id = ianaId;
}

QLocale::Country QTzTimeZonePrivate::country() const
{
    return tzZones->value(m_id).country;
}

QString QTzTimeZonePrivate::comment() const
{
    return QString::fromUtf8(tzZones->value(m_id).comment);
}

QString QTzTimeZonePrivate::displayName(qint64 atMSecsSinceEpoch,
                                        QTimeZone::NameType nameType,
                                        const QLocale &locale) const
{
#ifdef QT_USE_ICU
    if (!m_icu)
        m_icu = new QIcuTimeZonePrivate(m_id);
    // TODO small risk may not match if tran times differ due to outdated files
    // TODO Some valid TZ names are not valid ICU names, use translation table?
    if (m_icu->isValid())
        return m_icu->displayName(atMSecsSinceEpoch, nameType, locale);
#else
    Q_UNUSED(nameType)
    Q_UNUSED(locale)
#endif // QT_USE_ICU
    return abbreviation(atMSecsSinceEpoch);
}

QString QTzTimeZonePrivate::displayName(QTimeZone::TimeType timeType,
                                        QTimeZone::NameType nameType,
                                        const QLocale &locale) const
{
#ifdef QT_USE_ICU
    if (!m_icu)
        m_icu = new QIcuTimeZonePrivate(m_id);
    // TODO small risk may not match if tran times differ due to outdated files
    // TODO Some valid TZ names are not valid ICU names, use translation table?
    if (m_icu->isValid())
        return m_icu->displayName(timeType, nameType, locale);
#else
    Q_UNUSED(timeType)
    Q_UNUSED(nameType)
    Q_UNUSED(locale)
#endif // QT_USE_ICU
    // If no ICU available then have to use abbreviations instead
    // Abbreviations don't have GenericTime
    if (timeType == QTimeZone::GenericTime)
        timeType = QTimeZone::StandardTime;

    // Get current tran, if valid and is what we want, then use it
    const qint64 currentMSecs = QDateTime::currentMSecsSinceEpoch();
    QTimeZonePrivate::Data tran = data(currentMSecs);
    if (tran.atMSecsSinceEpoch != invalidMSecs()
        && ((timeType == QTimeZone::DaylightTime && tran.daylightTimeOffset != 0)
        || (timeType == QTimeZone::StandardTime && tran.daylightTimeOffset == 0))) {
        return tran.abbreviation;
    }

    // Otherwise get next tran and if valid and is what we want, then use it
    tran = nextTransition(currentMSecs);
    if (tran.atMSecsSinceEpoch != invalidMSecs()
        && ((timeType == QTimeZone::DaylightTime && tran.daylightTimeOffset != 0)
        || (timeType == QTimeZone::StandardTime && tran.daylightTimeOffset == 0))) {
        return tran.abbreviation;
    }

    // Otherwise get prev tran and if valid and is what we want, then use it
    tran = previousTransition(currentMSecs);
    if (tran.atMSecsSinceEpoch != invalidMSecs())
        tran = previousTransition(tran.atMSecsSinceEpoch);
    if (tran.atMSecsSinceEpoch != invalidMSecs()
        && ((timeType == QTimeZone::DaylightTime && tran.daylightTimeOffset != 0)
        || (timeType == QTimeZone::StandardTime && tran.daylightTimeOffset == 0))) {
        return tran.abbreviation;
    }

    // Otherwise is strange sequence, so work backwards through trans looking for first match, if any
    for (int i = m_tranTimes.size() - 1; i >= 0; --i) {
        if (m_tranTimes.at(i).atMSecsSinceEpoch <= currentMSecs) {
            tran = dataForTzTransition(m_tranTimes.at(i));
            if ((timeType == QTimeZone::DaylightTime && tran.daylightTimeOffset != 0)
                || (timeType == QTimeZone::StandardTime && tran.daylightTimeOffset == 0)) {
                return tran.abbreviation;
            }
        }
    }

    // Otherwise if no match use current data
    return data(currentMSecs).abbreviation;
}

QString QTzTimeZonePrivate::abbreviation(qint64 atMSecsSinceEpoch) const
{
    return data(atMSecsSinceEpoch).abbreviation;
}

int QTzTimeZonePrivate::offsetFromUtc(qint64 atMSecsSinceEpoch) const
{
    const QTimeZonePrivate::Data tran = data(atMSecsSinceEpoch);
    return tran.standardTimeOffset + tran.daylightTimeOffset;
}

int QTzTimeZonePrivate::standardTimeOffset(qint64 atMSecsSinceEpoch) const
{
    return data(atMSecsSinceEpoch).standardTimeOffset;
}

int QTzTimeZonePrivate::daylightTimeOffset(qint64 atMSecsSinceEpoch) const
{
    return data(atMSecsSinceEpoch).daylightTimeOffset;
}

bool QTzTimeZonePrivate::hasDaylightTime() const
{
    // TODO Perhaps cache as frequently accessed?
    foreach (const QTzTransitionRule &rule, m_tranRules) {
        if (rule.dstOffset != 0)
            return true;
    }
    return false;
}

bool QTzTimeZonePrivate::isDaylightTime(qint64 atMSecsSinceEpoch) const
{
    return (daylightTimeOffset(atMSecsSinceEpoch) != 0);
}

QTimeZonePrivate::Data QTzTimeZonePrivate::dataForTzTransition(QTzTransitionTime tran) const
{
    QTimeZonePrivate::Data data;
    data.atMSecsSinceEpoch = tran.atMSecsSinceEpoch;
    QTzTransitionRule rule = m_tranRules.at(tran.ruleIndex);
    data.standardTimeOffset = rule.stdOffset;
    data.daylightTimeOffset = rule.dstOffset;
    data.offsetFromUtc = rule.stdOffset + rule.dstOffset;
    data.abbreviation = QString::fromUtf8(m_abbreviations.at(rule.abbreviationIndex));
    return data;
}

QTimeZonePrivate::Data QTzTimeZonePrivate::data(qint64 forMSecsSinceEpoch) const
{
    // If the required time is after the last transition and we have a POSIX rule then use it
    if (m_tranTimes.size() > 0 && m_tranTimes.last().atMSecsSinceEpoch < forMSecsSinceEpoch
        &&!m_posixRule.isEmpty() && forMSecsSinceEpoch >= 0) {
        const int year = QDateTime::fromMSecsSinceEpoch(forMSecsSinceEpoch, Qt::UTC).date().year();
        const int lastMSecs = (m_tranTimes.size() > 0) ? m_tranTimes.last().atMSecsSinceEpoch : 0;
        QList<QTimeZonePrivate::Data> posixTrans = calculatePosixTransitions(m_posixRule, year - 1,
                                                                             year + 1, lastMSecs);
        for (int i = posixTrans.size() - 1; i >= 0; --i) {
            if (posixTrans.at(i).atMSecsSinceEpoch <= forMSecsSinceEpoch) {
                QTimeZonePrivate::Data data;
                data = posixTrans.at(i);
                data.atMSecsSinceEpoch = forMSecsSinceEpoch;
                return data;
            }
        }
    }

    // Otherwise if we can find a valid tran then use its rule
    for (int i = m_tranTimes.size() - 1; i >= 0; --i) {
        if (m_tranTimes.at(i).atMSecsSinceEpoch <= forMSecsSinceEpoch) {
            Data data = dataForTzTransition(m_tranTimes.at(i));
            data.atMSecsSinceEpoch = forMSecsSinceEpoch;
            return data;
        }
    }

    // Otherwise use the earliest transition we have
    if (m_tranTimes.size() > 0) {
        Data data = dataForTzTransition(m_tranTimes.at(0));
        data.atMSecsSinceEpoch = forMSecsSinceEpoch;
        return data;
    }

    // Otherwise we have no rules, so probably an invalid tz, so return invalid data
    return invalidData();
}

bool QTzTimeZonePrivate::hasTransitions() const
{
    return true;
}

QTimeZonePrivate::Data QTzTimeZonePrivate::nextTransition(qint64 afterMSecsSinceEpoch) const
{
    // If the required time is after the last transition and we have a POSIX rule then use it
    if (m_tranTimes.size() > 0 && m_tranTimes.last().atMSecsSinceEpoch < afterMSecsSinceEpoch
        &&!m_posixRule.isEmpty() && afterMSecsSinceEpoch >= 0) {
        const int year = QDateTime::fromMSecsSinceEpoch(afterMSecsSinceEpoch, Qt::UTC).date().year();
        const int lastMSecs = (m_tranTimes.size() > 0) ? m_tranTimes.last().atMSecsSinceEpoch : 0;
        QList<QTimeZonePrivate::Data> posixTrans = calculatePosixTransitions(m_posixRule, year - 1,
                                                                             year + 1, lastMSecs);
        for (int i = 0; i < posixTrans.size(); ++i) {
            if (posixTrans.at(i).atMSecsSinceEpoch > afterMSecsSinceEpoch)
                return posixTrans.at(i);
        }
    }

    // Otherwise if we can find a valid tran then use its rule
    for (int i = 0; i < m_tranTimes.size(); ++i) {
        if (m_tranTimes.at(i).atMSecsSinceEpoch > afterMSecsSinceEpoch) {
            return dataForTzTransition(m_tranTimes.at(i));
        }
    }

    // Otherwise we have no rule, or there is no next transition, so return invalid data
    return invalidData();
}

QTimeZonePrivate::Data QTzTimeZonePrivate::previousTransition(qint64 beforeMSecsSinceEpoch) const
{
    // If the required time is after the last transition and we have a POSIX rule then use it
    if (m_tranTimes.size() > 0 && m_tranTimes.last().atMSecsSinceEpoch < beforeMSecsSinceEpoch
        &&!m_posixRule.isEmpty() && beforeMSecsSinceEpoch > 0) {
        const int year = QDateTime::fromMSecsSinceEpoch(beforeMSecsSinceEpoch, Qt::UTC).date().year();
        const int lastMSecs = (m_tranTimes.size() > 0) ? m_tranTimes.last().atMSecsSinceEpoch : 0;
        QList<QTimeZonePrivate::Data> posixTrans = calculatePosixTransitions(m_posixRule, year - 1,
                                                                             year + 1, lastMSecs);
        for (int i = posixTrans.size() - 1; i >= 0; --i) {
            if (posixTrans.at(i).atMSecsSinceEpoch < beforeMSecsSinceEpoch)
                return posixTrans.at(i);
        }
    }

    // Otherwise if we can find a valid tran then use its rule
    for (int i = m_tranTimes.size() - 1; i >= 0; --i) {
        if (m_tranTimes.at(i).atMSecsSinceEpoch < beforeMSecsSinceEpoch) {
            return dataForTzTransition(m_tranTimes.at(i));
        }
    }

    // Otherwise we have no rule, so return invalid data
    return invalidData();
}

// TODO Could cache the value and monitor the required files for any changes
QByteArray QTzTimeZonePrivate::systemTimeZoneId() const
{
    // Check TZ env var first, if not populated try find it
    QByteArray ianaId = qgetenv("TZ");
    if (!ianaId.isEmpty() && ianaId.at(0) == ':')
        ianaId = ianaId.mid(1);

    // On Debian Etch and later /etc/localtime is real file with name held in /etc/timezone
    if (ianaId.isEmpty()) {
        QFile tzif(QStringLiteral("/etc/timezone"));
        if (tzif.open(QIODevice::ReadOnly)) {
            // TODO QTextStream inefficient, replace later
            QTextStream ts(&tzif);
            if (!ts.atEnd())
                ianaId = ts.readLine().toUtf8();
        }
    }

    // On other distros /etc/localtime is symlink to real file so can extract name from the path
    if (ianaId.isEmpty()) {
        const QString path = QFile::symLinkTarget(QStringLiteral("/etc/localtime"));
        if (!path.isEmpty()) {
            // /etc/localtime is a symlink to the current TZ file, so extract from path
            int index = path.indexOf(QLatin1String("/zoneinfo/")) + 10;
            ianaId = path.mid(index).toUtf8();
        }
    }

    // On some Red Hat distros /etc/localtime is real file with name held in /etc/sysconfig/clock
    // in a line like ZONE="Europe/Oslo" or TIMEZONE="Europe/Oslo"
    if (ianaId.isEmpty()) {
        QFile tzif(QStringLiteral("/etc/sysconfig/clock"));
        if (tzif.open(QIODevice::ReadOnly)) {
            // TODO QTextStream inefficient, replace later
            QTextStream ts(&tzif);
            QString line;
            while (ianaId.isEmpty() && !ts.atEnd() && ts.status() == QTextStream::Ok) {
                line = ts.readLine();
                if (line.left(5) == QStringLiteral("ZONE=")) {
                    ianaId = line.mid(6, line.size() - 7).toUtf8();
                } else if (line.left(9) == QStringLiteral("TIMEZONE=")) {
                    ianaId = line.mid(10, line.size() - 11).toUtf8();
                }
            }
        }
    }

    // Give up for now and return UTC
    if (ianaId.isEmpty())
        ianaId = QByteArrayLiteral("UTC");

    return ianaId;
}

QSet<QByteArray> QTzTimeZonePrivate::availableTimeZoneIds() const
{
    return tzZones->keys().toSet();
}

QSet<QByteArray> QTzTimeZonePrivate::availableTimeZoneIds(QLocale::Country country) const
{
    // TODO AnyCountry
    QSet<QByteArray> set;
    foreach (const QByteArray &key, tzZones->keys()) {
        if (tzZones->value(key).country == country)
            set << key;
    }
    return set;
}

QT_END_NAMESPACE
