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
#include "qtimezoneprivate_data_p.h"

#include <qdebug.h>

QT_BEGIN_NAMESPACE

enum {
    MSECS_TRAN_WINDOW = 21600000 // 6 hour window for possible recent transitions
};

/*
    Static utilities for looking up Windows ID tables
*/

static const int windowsDataTableSize = sizeof(windowsDataTable) / sizeof(QWindowsData) - 1;
static const int zoneDataTableSize = sizeof(zoneDataTable) / sizeof(QZoneData) - 1;
static const int utcDataTableSize = sizeof(utcDataTable) / sizeof(QUtcData) - 1;


static const QZoneData *zoneData(quint16 index)
{
    Q_ASSERT(index < zoneDataTableSize);
    return &zoneDataTable[index];
}

static const QWindowsData *windowsData(quint16 index)
{
    Q_ASSERT(index < windowsDataTableSize);
    return &windowsDataTable[index];
}

static const QUtcData *utcData(quint16 index)
{
    Q_ASSERT(index < utcDataTableSize);
    return &utcDataTable[index];
}

// Return the Windows ID literal for a given QWindowsData
static QByteArray windowsId(const QWindowsData *windowsData)
{
    return (windowsIdData + windowsData->windowsIdIndex);
}

// Return the IANA ID literal for a given QWindowsData
static QByteArray ianaId(const QWindowsData *windowsData)
{
    return (ianaIdData + windowsData->ianaIdIndex);
}

// Return the IANA ID literal for a given QZoneData
static QByteArray ianaId(const QZoneData *zoneData)
{
    return (ianaIdData + zoneData->ianaIdIndex);
}

static QByteArray utcId(const QUtcData *utcData)
{
    return (ianaIdData + utcData->ianaIdIndex);
}

static quint16 toWindowsIdKey(const QByteArray &winId)
{
    for (quint16 i = 0; i < windowsDataTableSize; ++i) {
        const QWindowsData *data = windowsData(i);
        if (windowsId(data) == winId)
            return data->windowsIdKey;
    }
    return 0;
}

static QByteArray toWindowsIdLiteral(quint16 windowsIdKey)
{
    for (quint16 i = 0; i < windowsDataTableSize; ++i) {
        const QWindowsData *data = windowsData(i);
        if (data->windowsIdKey == windowsIdKey)
            return windowsId(data);
    }
    return QByteArray();
}

/*
    Base class implementing common utility routines, only intantiate for a null tz.
*/

QTimeZonePrivate::QTimeZonePrivate()
{
}

QTimeZonePrivate::QTimeZonePrivate(const QTimeZonePrivate &other)
    : QSharedData(other), m_id(other.m_id)
{
}

QTimeZonePrivate::~QTimeZonePrivate()
{
}

QTimeZonePrivate *QTimeZonePrivate::clone()
{
    return new QTimeZonePrivate(*this);
}

bool QTimeZonePrivate::operator==(const QTimeZonePrivate &other) const
{
    // TODO Too simple, but need to solve problem of comparing different derived classes
    // Should work for all System and ICU classes as names guaranteed unique, but not for Simple.
    // Perhaps once all classes have working transitions can compare full list?
    return (m_id == other.m_id);
}

bool QTimeZonePrivate::operator!=(const QTimeZonePrivate &other) const
{
    return !(*this == other);
}

bool QTimeZonePrivate::isValid() const
{
    return !m_id.isEmpty();
}

QByteArray QTimeZonePrivate::id() const
{
    return m_id;
}

QLocale::Country QTimeZonePrivate::country() const
{
    // Default fall-back mode, use the zoneTable to find Region of known Zones
    for (int i = 0; i < zoneDataTableSize; ++i) {
        const QZoneData *data = zoneData(i);
        if (ianaId(data).split(' ').contains(m_id))
            return (QLocale::Country)data->country;
    }
    return QLocale::AnyCountry;
}

QString QTimeZonePrivate::comment() const
{
    return QString();
}

QString QTimeZonePrivate::displayName(qint64 atMSecsSinceEpoch,
                                      QTimeZone::NameType nameType,
                                      const QLocale &locale) const
{
    if (nameType == QTimeZone::OffsetName)
        return isoOffsetFormat(offsetFromUtc(atMSecsSinceEpoch));

    if (isDaylightTime(atMSecsSinceEpoch))
        return displayName(QTimeZone::DaylightTime, nameType, locale);
    else
        return displayName(QTimeZone::StandardTime, nameType, locale);
}

QString QTimeZonePrivate::displayName(QTimeZone::TimeType timeType,
                                      QTimeZone::NameType nameType,
                                      const QLocale &locale) const
{
    Q_UNUSED(timeType)
    Q_UNUSED(nameType)
    Q_UNUSED(locale)
    return QString();
}

QString QTimeZonePrivate::abbreviation(qint64 atMSecsSinceEpoch) const
{
    Q_UNUSED(atMSecsSinceEpoch)
    return QString();
}

int QTimeZonePrivate::offsetFromUtc(qint64 atMSecsSinceEpoch) const
{
    return standardTimeOffset(atMSecsSinceEpoch) + daylightTimeOffset(atMSecsSinceEpoch);
}

int QTimeZonePrivate::standardTimeOffset(qint64 atMSecsSinceEpoch) const
{
    Q_UNUSED(atMSecsSinceEpoch)
    return invalidSeconds();
}

int QTimeZonePrivate::daylightTimeOffset(qint64 atMSecsSinceEpoch) const
{
    Q_UNUSED(atMSecsSinceEpoch)
    return invalidSeconds();
}

bool QTimeZonePrivate::hasDaylightTime() const
{
    return false;
}

bool QTimeZonePrivate::isDaylightTime(qint64 atMSecsSinceEpoch) const
{
    Q_UNUSED(atMSecsSinceEpoch)
    return false;
}

QTimeZonePrivate::Data QTimeZonePrivate::data(qint64 forMSecsSinceEpoch) const
{
    Q_UNUSED(forMSecsSinceEpoch)
    return invalidData();
}

// Private only method for use by QDateTime to convert local msecs to epoch msecs
// TODO Could be platform optimised if needed
QTimeZonePrivate::Data QTimeZonePrivate::dataForLocalTime(qint64 forLocalMSecs) const
{
    if (!hasDaylightTime() ||!hasTransitions()) {
        // No daylight time means same offset for all local msecs
        // Having daylight time but no transitions means we can't calculate, so use nearest
        return data(forLocalMSecs - (standardTimeOffset(forLocalMSecs) * 1000));
    }

    // Get the transition for the local msecs which most of the time should be the right one
    // Only around the transition times might it not be the right one
    Data tran = previousTransition(forLocalMSecs);
    Data nextTran;

    // If the local msecs is less than the real local time of the transition
    // then get the previous transition to use instead
    if (forLocalMSecs < tran.atMSecsSinceEpoch + (tran.offsetFromUtc * 1000)) {
        while (tran.atMSecsSinceEpoch != invalidMSecs()
               && forLocalMSecs < tran.atMSecsSinceEpoch + (tran.offsetFromUtc * 1000)) {
            nextTran = tran;
            tran = previousTransition(tran.atMSecsSinceEpoch);
        }
    } else {
        // The zone msecs is after the transition, so check it is before the next tran
        // If not try use the next transition instead
        nextTran = nextTransition(tran.atMSecsSinceEpoch);
        while (nextTran.atMSecsSinceEpoch != invalidMSecs()
               && forLocalMSecs >= nextTran.atMSecsSinceEpoch + (nextTran.offsetFromUtc * 1000)) {
            tran = nextTran;
            nextTran = nextTransition(tran.atMSecsSinceEpoch);
        }
    }

    if (tran.daylightTimeOffset == 0) {
        // If tran is in StandardTime, then need to check if falls close either daylight transition
        // If it does, then it may need adjusting for missing hour or for second occurrence
        qint64 diffPrevTran = forLocalMSecs
                              - (tran.atMSecsSinceEpoch + (tran.offsetFromUtc * 1000));
        qint64 diffNextTran = nextTran.atMSecsSinceEpoch + (nextTran.offsetFromUtc * 1000)
                              - forLocalMSecs;
        if (diffPrevTran >= 0 && diffPrevTran < MSECS_TRAN_WINDOW) {
            // If tran picked is for standard time check if changed from daylight in last 6 hours,
            // as the local msecs may be ambiguous and represent two valid utc msecs.
            // If in last 6 hours then get prev tran and if diff falls within the daylight offset
            // then use the prev tran as we default to the FirstOccurrence
            // TODO Check if faster to just always get prev tran, or if faster using 6 hour check.
            Data dstTran = previousTransition(tran.atMSecsSinceEpoch);
            if (dstTran.atMSecsSinceEpoch != invalidMSecs()
                && dstTran.daylightTimeOffset > 0 && diffPrevTran < (dstTran.daylightTimeOffset * 1000))
                tran = dstTran;
        } else if (diffNextTran >= 0 && diffNextTran <= (nextTran.daylightTimeOffset * 1000)) {
            // If time falls within last hour of standard time then is actually the missing hour
            // So return the next tran instead and adjust the local time to be valid
            tran = nextTran;
            forLocalMSecs = forLocalMSecs + (nextTran.daylightTimeOffset * 1000);
        }
    }

    // tran should now hold the right transition offset to use
    tran.atMSecsSinceEpoch = forLocalMSecs - (tran.offsetFromUtc * 1000);
    return tran;
}

bool QTimeZonePrivate::hasTransitions() const
{
    return false;
}

QTimeZonePrivate::Data QTimeZonePrivate::nextTransition(qint64 afterMSecsSinceEpoch) const
{
    Q_UNUSED(afterMSecsSinceEpoch)
    return invalidData();
}

QTimeZonePrivate::Data QTimeZonePrivate::previousTransition(qint64 beforeMSecsSinceEpoch) const
{
    Q_UNUSED(beforeMSecsSinceEpoch)
    return invalidData();
}

QTimeZonePrivate::DataList QTimeZonePrivate::transitions(qint64 fromMSecsSinceEpoch,
                                                         qint64 toMSecsSinceEpoch) const
{
    DataList list;
    if (toMSecsSinceEpoch >= fromMSecsSinceEpoch) {
        // fromMSecsSinceEpoch is inclusive but nextTransitionTime() is exclusive so go back 1 msec
        Data next = nextTransition(fromMSecsSinceEpoch - 1);
        while (next.atMSecsSinceEpoch != invalidMSecs()
               && next.atMSecsSinceEpoch <= toMSecsSinceEpoch) {
            list.append(next);
            next = nextTransition(next.atMSecsSinceEpoch);
        }
    }
    return list;
}

QByteArray QTimeZonePrivate::systemTimeZoneId() const
{
    return QByteArray();
}

QSet<QByteArray> QTimeZonePrivate::availableTimeZoneIds() const
{
    return QSet<QByteArray>();
}

QSet<QByteArray> QTimeZonePrivate::availableTimeZoneIds(QLocale::Country country) const
{
    // Default fall-back mode, use the zoneTable to find Region of know Zones
    QSet<QByteArray> regionSet;

    // First get all Zones in the Zones table belonging to the Region
    for (int i = 0; i < zoneDataTableSize; ++i) {
        if (zoneData(i)->country == country)
            regionSet += ianaId(zoneData(i)).split(' ').toSet();
    }

    // Then select just those that are available
    QSet<QByteArray> set;
    foreach (const QByteArray &ianaId, availableTimeZoneIds()) {
        if (regionSet.contains(ianaId))
            set << ianaId;
    }

    return set;
}

QSet<QByteArray> QTimeZonePrivate::availableTimeZoneIds(int offsetFromUtc) const
{
    // Default fall-back mode, use the zoneTable to find Offset of know Zones
    QSet<QByteArray> offsetSet;
    // First get all Zones in the table using the Offset
    for (int i = 0; i < windowsDataTableSize; ++i) {
        const QWindowsData *winData = windowsData(i);
        if (winData->offsetFromUtc == offsetFromUtc) {
            for (int j = 0; j < zoneDataTableSize; ++j) {
                const QZoneData *data = zoneData(j);
                if (data->windowsIdKey == winData->windowsIdKey)
                    offsetSet += ianaId(data).split(' ').toSet();
            }
        }
    }

    // Then select just those that are available
    QSet<QByteArray> set;
    foreach (const QByteArray &ianaId, availableTimeZoneIds()) {
        if (offsetSet.contains(ianaId))
            set << ianaId;
    }

    return set;
}

#ifndef QT_NO_DATASTREAM
void QTimeZonePrivate::serialize(QDataStream &ds) const
{
    ds << QString::fromUtf8(m_id);
}
#endif // QT_NO_DATASTREAM

// Static Utility Methods

QTimeZonePrivate::Data QTimeZonePrivate::invalidData()
{
    Data data;
    data.atMSecsSinceEpoch = invalidMSecs();
    data.offsetFromUtc = invalidSeconds();
    data.standardTimeOffset = invalidSeconds();
    data.daylightTimeOffset = invalidSeconds();
    return data;
}

QTimeZone::OffsetData QTimeZonePrivate::invalidOffsetData()
{
    QTimeZone::OffsetData offsetData;
    offsetData.atUtc = QDateTime();
    offsetData.offsetFromUtc = invalidSeconds();
    offsetData.standardTimeOffset = invalidSeconds();
    offsetData.daylightTimeOffset = invalidSeconds();
    return offsetData;
}

QTimeZone::OffsetData QTimeZonePrivate::toOffsetData(const QTimeZonePrivate::Data &data)
{
    QTimeZone::OffsetData offsetData = invalidOffsetData();
    if (data.atMSecsSinceEpoch != invalidMSecs()) {
        offsetData.atUtc = QDateTime::fromMSecsSinceEpoch(data.atMSecsSinceEpoch, Qt::UTC);
        offsetData.offsetFromUtc = data.offsetFromUtc;
        offsetData.standardTimeOffset = data.standardTimeOffset;
        offsetData.daylightTimeOffset = data.daylightTimeOffset;
        offsetData.abbreviation = data.abbreviation;
    }
    return offsetData;
}

// If the format of the ID is valid
bool QTimeZonePrivate::isValidId(const QByteArray &ianaId)
{
    // Rules for defining TZ/IANA names as per ftp://ftp.iana.org/tz/code/Theory
    // * Use only valid POSIX file name components
    // * Within a file name component, use only ASCII letters, `.', `-' and `_'.
    // * Do not use digits
    // * A file name component must not exceed 14 characters or start with `-'
    // Aliases such as "Etc/GMT+7" and "SystemV/EST5EDT" are valid so we need to accept digits
    if (ianaId.contains(' '))
        return false;
    QList<QByteArray> parts = ianaId.split('/');
    foreach (const QByteArray &part, parts) {
        if (part.size() > 14 || part.size() < 1)
            return false;
        if (part.at(0) == '-')
            return false;
        for (int i = 0; i < part.size(); ++i) {
            QChar ch = part.at(i);
            if (!(ch >= 'a' && ch <= 'z')
                && !(ch >= 'A' && ch <= 'Z')
                && !(ch == '_')
                && !(ch >= '0' && ch <= '9')
                && !(ch == '-')
                && !(ch == '+')
                && !(ch == ':')
                && !(ch == '.'))
                return false;
        }
    }
    return true;
}

QString QTimeZonePrivate::isoOffsetFormat(int offsetFromUtc)
{
    const int mins = offsetFromUtc / 60;
    return QString::fromUtf8("UTC%1%2:%3").arg(mins >= 0 ? QLatin1Char('+') : QLatin1Char('-'))
                                          .arg(qAbs(mins) / 60, 2, 10, QLatin1Char('0'))
                                          .arg(qAbs(mins) % 60, 2, 10, QLatin1Char('0'));
}

QByteArray QTimeZonePrivate::ianaIdToWindowsId(const QByteArray &id)
{
    for (int i = 0; i < zoneDataTableSize; ++i) {
        const QZoneData *data = zoneData(i);
        if (ianaId(data).split(' ').contains(id))
            return toWindowsIdLiteral(data->windowsIdKey);
    }
    return QByteArray();
}

QByteArray QTimeZonePrivate::windowsIdToDefaultIanaId(const QByteArray &windowsId)
{
    const quint16 windowsIdKey = toWindowsIdKey(windowsId);
    for (int i = 0; i < windowsDataTableSize; ++i) {
        const QWindowsData *data = windowsData(i);
        if (data->windowsIdKey == windowsIdKey)
            return ianaId(data);
    }
    return QByteArray();
}

QByteArray QTimeZonePrivate::windowsIdToDefaultIanaId(const QByteArray &windowsId,
                                                       QLocale::Country country)
{
    const QList<QByteArray> list = windowsIdToIanaIds(windowsId, country);
    if (list.count() > 0)
        return list.first();
    else
        return QByteArray();
}

QList<QByteArray> QTimeZonePrivate::windowsIdToIanaIds(const QByteArray &windowsId)
{
    const quint16 windowsIdKey = toWindowsIdKey(windowsId);
    QList<QByteArray> list;

    for (int i = 0; i < zoneDataTableSize; ++i) {
        const QZoneData *data = zoneData(i);
        if (data->windowsIdKey == windowsIdKey)
            list << ianaId(data).split(' ');
    }

    // Return the full list in alpha order
    std::sort(list.begin(), list.end());
    return list;
}

QList<QByteArray> QTimeZonePrivate::windowsIdToIanaIds(const QByteArray &windowsId,
                                                        QLocale::Country country)
{
    const quint16 windowsIdKey = toWindowsIdKey(windowsId);
    for (int i = 0; i < zoneDataTableSize; ++i) {
        const QZoneData *data = zoneData(i);
        // Return the region matches in preference order
        if (data->windowsIdKey == windowsIdKey && data->country == (quint16) country)
            return ianaId(data).split(' ');
    }

    return QList<QByteArray>();
}

// Define template for derived classes to reimplement so QSharedDataPointer clone() works correctly
template<> QTimeZonePrivate *QSharedDataPointer<QTimeZonePrivate>::clone()
{
    return d->clone();
}

/*
    UTC Offset implementation, used when QT_NO_SYSTEMLOCALE set and QT_USE_ICU not set,
    or for QDateTimes with a Qt:Spec of Qt::OffsetFromUtc.
*/

// Create default UTC time zone
QUtcTimeZonePrivate::QUtcTimeZonePrivate()
{
    const QString name = QStringLiteral("UTC");
    init(QByteArrayLiteral("UTC"), 0, name, name, QLocale::AnyCountry, name);
}

// Create a named UTC time zone
QUtcTimeZonePrivate::QUtcTimeZonePrivate(const QByteArray &id)
{
    // Look for the name in the UTC list, if found set the values
    for (int i = 0; i < utcDataTableSize; ++i) {
        const QUtcData *data = utcData(i);
        const QByteArray uid = utcId(data);
        if (uid == id) {
            QString name = QString::fromUtf8(id);
            init(id, data->offsetFromUtc, name, name, QLocale::AnyCountry, name);
            break;
        }
    }
}

// Create offset from UTC
QUtcTimeZonePrivate::QUtcTimeZonePrivate(qint32 offsetSeconds)
{
    QString utcId;

    if (offsetSeconds == 0)
        utcId = QStringLiteral("UTC");
    else
        utcId = isoOffsetFormat(offsetSeconds);

    init(utcId.toUtf8(), offsetSeconds, utcId, utcId, QLocale::AnyCountry, utcId);
}

QUtcTimeZonePrivate::QUtcTimeZonePrivate(const QByteArray &zoneId, int offsetSeconds,
                                         const QString &name, const QString &abbreviation,
                                         QLocale::Country country, const QString &comment)
{
    init(zoneId, offsetSeconds, name, abbreviation, country, comment);
}

QUtcTimeZonePrivate::QUtcTimeZonePrivate(const QUtcTimeZonePrivate &other)
    : QTimeZonePrivate(other), m_name(other.m_name),
      m_abbreviation(other.m_abbreviation),
      m_comment(other.m_comment),
      m_country(other.m_country),
      m_offsetFromUtc(other.m_offsetFromUtc)
{
}

QUtcTimeZonePrivate::~QUtcTimeZonePrivate()
{
}

QTimeZonePrivate *QUtcTimeZonePrivate::clone()
{
    return new QUtcTimeZonePrivate(*this);
}

void QUtcTimeZonePrivate::init(const QByteArray &zoneId)
{
    m_id = zoneId;
}

void QUtcTimeZonePrivate::init(const QByteArray &zoneId, int offsetSeconds, const QString &name,
                               const QString &abbreviation, QLocale::Country country,
                               const QString &comment)
{
    m_id = zoneId;
    m_offsetFromUtc = offsetSeconds;
    m_name = name;
    m_abbreviation = abbreviation;
    m_country = country;
    m_comment = comment;
}

QLocale::Country QUtcTimeZonePrivate::country() const
{
    return m_country;
}

QString QUtcTimeZonePrivate::comment() const
{
    return m_comment;
}

QString QUtcTimeZonePrivate::displayName(QTimeZone::TimeType timeType,
                                         QTimeZone::NameType nameType,
                                         const QLocale &locale) const
{
    Q_UNUSED(timeType)
    Q_UNUSED(locale)
    if (nameType == QTimeZone::ShortName)
        return m_abbreviation;
    else if (nameType == QTimeZone::OffsetName)
        return isoOffsetFormat(m_offsetFromUtc);
    return m_name;
}

QString QUtcTimeZonePrivate::abbreviation(qint64 atMSecsSinceEpoch) const
{
    Q_UNUSED(atMSecsSinceEpoch)
    return m_abbreviation;
}

qint32 QUtcTimeZonePrivate::standardTimeOffset(qint64 atMSecsSinceEpoch) const
{
    Q_UNUSED(atMSecsSinceEpoch)
    return m_offsetFromUtc;
}

qint32 QUtcTimeZonePrivate::daylightTimeOffset(qint64 atMSecsSinceEpoch) const
{
    Q_UNUSED(atMSecsSinceEpoch)
    return 0;
}

QByteArray QUtcTimeZonePrivate::systemTimeZoneId() const
{
    return QByteArrayLiteral("UTC");
}

QSet<QByteArray> QUtcTimeZonePrivate::availableTimeZoneIds() const
{
    QSet<QByteArray> set;
    for (int i = 0; i < utcDataTableSize; ++i)
        set << utcId(utcData(i));
    return set;
}

QSet<QByteArray> QUtcTimeZonePrivate::availableTimeZoneIds(QLocale::Country country) const
{
    // If AnyCountry then is request for all non-region offset codes
    if (country == QLocale::AnyCountry)
        return availableTimeZoneIds();
    return QSet<QByteArray>();
}

QSet<QByteArray> QUtcTimeZonePrivate::availableTimeZoneIds(qint32 offsetSeconds) const
{
    QSet<QByteArray> set;
    for (int i = 0; i < utcDataTableSize; ++i) {
        const QUtcData *data = utcData(i);
        if (data->offsetFromUtc == offsetSeconds)
            set << utcId(data);
    }
    return set;
}

#ifndef QT_NO_DATASTREAM
void QUtcTimeZonePrivate::serialize(QDataStream &ds) const
{
    ds << QStringLiteral("OffsetFromUtc") << QString::fromUtf8(m_id) << m_offsetFromUtc << m_name
       << m_abbreviation << (qint32) m_country << m_comment;
}
#endif // QT_NO_DATASTREAM

QT_END_NAMESPACE
