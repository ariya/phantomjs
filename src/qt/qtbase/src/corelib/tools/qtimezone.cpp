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

#include <QtCore/qdatetime.h>

#include <qdebug.h>

#include <algorithm>

QT_BEGIN_NAMESPACE

// Create default time zone using appropriate backend
static QTimeZonePrivate *newBackendTimeZone()
{
#ifdef QT_NO_SYSTEMLOCALE
#ifdef QT_USE_ICU
    return new QIcuTimeZonePrivate();
#else
    return new QUtcTimeZonePrivate();
#endif // QT_USE_ICU
#else
#if defined Q_OS_MAC
    return new QMacTimeZonePrivate();
#elif defined Q_OS_UNIX
    return new QTzTimeZonePrivate();
    // Registry based timezone backend not available on WinRT
#elif defined Q_OS_WIN && !defined Q_OS_WINRT
    return new QWinTimeZonePrivate();
#elif defined QT_USE_ICU
    return new QIcuTimeZonePrivate();
#else
    return new QUtcTimeZonePrivate();
#endif // System Locales
#endif // QT_NO_SYSTEMLOCALE
}

// Create named time zone using appropriate backend
static QTimeZonePrivate *newBackendTimeZone(const QByteArray &ianaId)
{
#ifdef QT_NO_SYSTEMLOCALE
#ifdef QT_USE_ICU
    return new QIcuTimeZonePrivate(ianaId);
#else
    return new QUtcTimeZonePrivate(ianaId);
#endif // QT_USE_ICU
#else
#if defined Q_OS_MAC
    return new QMacTimeZonePrivate(ianaId);
#elif defined Q_OS_UNIX
    return new QTzTimeZonePrivate(ianaId);
    // Registry based timezone backend not available on WinRT
#elif defined Q_OS_WIN && !defined Q_OS_WINRT
    return new QWinTimeZonePrivate(ianaId);
#elif defined QT_USE_ICU
    return new QIcuTimeZonePrivate(ianaId);
#else
    return new QUtcTimeZonePrivate(ianaId);
#endif // System Locales
#endif // QT_NO_SYSTEMLOCALE
}

class QTimeZoneSingleton
{
public:
    QTimeZoneSingleton() : backend(newBackendTimeZone()) {}

    // The backend_tz is the tz to use in static methods such as availableTimeZoneIds() and
    // isTimeZoneIdAvailable() and to create named IANA time zones.  This is usually the host
    // system, but may be different if the host resources are insufficient or if
    // QT_NO_SYSTEMLOCALE is set.  A simple UTC backend is used if no alternative is available.
    QSharedDataPointer<QTimeZonePrivate> backend;
};

Q_GLOBAL_STATIC(QTimeZoneSingleton, global_tz);

/*!
    \class QTimeZone
    \inmodule QtCore
    \since 5.2
    \brief The QTimeZone class converts between between UTC and local time in a
           specific time zone.

    \threadsafe

    This class provides a stateless calculator for time zone conversions
    between UTC and the local time in a specific time zone.  By default it uses
    the host system time zone data to perform these conversions.

    This class is primarily designed for use in QDateTime; most applications
    will not need to access this class directly and should instead use
    QDateTime with a Qt::TimeSpec of Qt::TimeZone.

    \note For consistency with QDateTime, QTimeZone does not account for leap
    seconds.

    \section1

    \section2 IANA Time Zone IDs

    QTimeZone uses the IANA time zone IDs as defined in the IANA Time Zone
    Database (http://www.iana.org/time-zones). This is to ensure a standard ID
    across all supported platforms.  Most platforms support the IANA IDs
    and the IANA Database natively, but for Windows a mapping is required to
    the native IDs.  See below for more details.

    The IANA IDs can and do change on a regular basis, and can vary depending
    on how recently the host system data was updated.  As such you cannot rely
    on any given ID existing on any host system.  You must use
    availableTimeZoneIds() to determine what IANA IDs are available.

    The IANA IDs and database are also know as the Olson IDs and database,
    named after their creator.

    \section2 UTC Offset Time Zones

    A default UTC time zone backend is provided which is always guaranteed to
    be available.  This provides a set of generic Offset From UTC time zones
    in the range UTC-14:00 to UTC+14:00.  These time zones can be created
    using either the standard ISO format names "UTC+00:00" as listed by
    availableTimeZoneIds(), or using the number of offset seconds.

    \section2 Windows Time Zones

    Windows native time zone support is severely limited compared to the
    standard IANA TZ Database.  Windows time zones cover larger geographic
    areas and are thus less accurate in their conversions.  They also do not
    support as much historic conversion data and so may only be accurate for
    the current year.

    QTimeZone uses a conversion table derived form the Unicode CLDR data to map
    between IANA IDs and Windows IDs.  Depending on your version of Windows
    and Qt, this table may not be able to provide a valid conversion, in which
    "UTC" will be returned.

    QTimeZone provides a public API to use this conversion table.  The Windows ID
    used is the Windows Registry Key for the time zone which is also the MS
    Exchange EWS ID as well, but is different to the Time Zone Name (TZID) and
    COD code used by MS Exchange in versions before 2007.

    \section2 System Time Zone

    QTimeZone does not support any concept of a system or default time zone.
    If you require a QDateTime that uses the current system time zone at any
    given moment then you should use a Qt::TimeSpec of Qt::LocalTime.

    The method systemTimeZoneId() returns the current system IANA time zone
    ID which on OSX and Linux will always be correct.  On Windows this ID is
    translated from the Windows system ID using an internal translation
    table and the user's selected country.  As a consequence there is a small
    chance any Windows install may have IDs not known by Qt, in which case
    "UTC" will be returned.

    Creating a new QTimeZone instance using the system time zone ID will only
    produce a fixed named copy of the time zone, it will not change if the
    system time zone changes.

    \section2 Time Zone Offsets

    The difference between UTC and the local time in a time zone is expressed
    as an offset in seconds from UTC, i.e. the number of seconds to add to UTC
    to obtain the local time.  The total offset is comprised of two component
    parts, the standard time offset and the daylight time offset.  The standard
    time offset is the number of seconds to add to UTC to obtain standard time
    in the time zone.  The daylight time offset is the number of seconds to add
    to the standard time offset to obtain daylight time in the time zone.

    Note that the standard and daylight offsets for a time zone may change over
    time as countries have changed daylight time laws or even their standard
    time offset.

    \section2 License

    This class includes data obtained from the CLDR data files under the terms
    of the Unicode license.

    \legalese
    COPYRIGHT AND PERMISSION NOTICE

    Copyright © 1991-2012 Unicode, Inc. All rights reserved. Distributed under
    the Terms of Use in http://www.unicode.org/copyright.html.

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of the Unicode data files and any associated documentation (the "Data
    Files") or Unicode software and any associated documentation (the "Software")
    to deal in the Data Files or Software without restriction, including without
    limitation the rights to use, copy, modify, merge, publish, distribute, and/or
    sell copies of the Data Files or Software, and to permit persons to whom the
    Data Files or Software are furnished to do so, provided that (a) the above
    copyright notice(s) and this permission notice appear with all copies of the
    Data Files or Software, (b) both the above copyright notice(s) and this
    permission notice appear in associated documentation, and (c) there is clear
    notice in each modified Data File or in the Software as well as in the
    documentation associated with the Data File(s) or Software that the data or
    software has been modified.
    \endlegalese

    \sa QDateTime
*/

/*!
    \enum QTimeZone::TimeType

    The type of time zone time, for example when requesting the name.  In time
    zones that do not apply daylight time, all three values may return the
    same result.

    \value StandardTime
           The standard time in a time zone, i.e. when Daylight Savings is not
           in effect.
           For example when formatting a display name this will show something
           like "Pacific Standard Time".
    \value DaylightTime
           A time when Daylight Savings is in effect.
           For example when formatting a display name this will show something
           like "Pacific daylight time".
    \value GenericTime
           A time which is not specifically Standard or Daylight time, either
           an unknown time or a neutral form.
           For example when formatting a display name this will show something
           like "Pacific Time".
*/

/*!
    \enum QTimeZone::NameType

    The type of time zone name.

    \value DefaultName
           The default form of the time zone name, e.g. LongName, ShortName or OffsetName
    \value LongName
           The long form of the time zone name, e.g. "Central European Time"
    \value ShortName
           The short form of the time zone name, usually an abbreviation, e.g. "CET"
    \value OffsetName
           The standard ISO offset form of the time zone name, e.g. "UTC+01:00"
*/

/*!
    \class QTimeZone::OffsetData
    \inmodule QtCore

    The time zone offset data for a given moment in time, i.e. the time zone
    offsets and abbreviation to use at that moment in time.

    \list
    \li OffsetData::atUtc  The datetime of the offset data in UTC time.
    \li OffsetData::offsetFromUtc  The total offset from UTC in effect at the datetime.
    \li OffsetData::standardTimeOffset  The standard time offset component of the total offset.
    \li OffsetData::daylightTimeOffset  The daylight time offset component of the total offset.
    \li OffsetData::abbreviation  The abbreviation in effect at the datetime.
    \endlist

    For example, for time zone "Europe/Berlin" the OffsetDate in standard and daylight time might be:

    \list
    \li atUtc = QDateTime(QDate(2013, 1, 1), QTime(0, 0, 0), Qt::UTC)
    \li offsetFromUtc = 3600
    \li standardTimeOffset = 3600
    \li daylightTimeOffset = 0
    \li abbreviation = "CET"
    \endlist

    \list
    \li atUtc = QDateTime(QDate(2013, 6, 1), QTime(0, 0, 0), Qt::UTC)
    \li offsetFromUtc = 7200
    \li standardTimeOffset = 3600
    \li daylightTimeOffset = 3600
    \li abbreviation = "CEST"
    \endlist
*/

/*!
    \typedef QTimeZone::OffsetDataList
    \relates QTimeZone

    Synonym for QList<OffsetData>.
*/

/*!
    Create a null/invalid time zone instance.
*/

QTimeZone::QTimeZone()
    : d(0)
{
}

/*!
    Creates an instance of the requested time zone \a ianaId.

    The ID must be one of the available system IDs otherwise an invalid
    time zone will be returned.

    \sa availableTimeZoneIds()
*/

QTimeZone::QTimeZone(const QByteArray &ianaId)
{
    // Try and see if it's a valid UTC offset ID, just as quick to try create as look-up
    d = new QUtcTimeZonePrivate(ianaId);
    // If not a valid UTC offset ID then try create it with the system backend
    // Relies on backend not creating valid tz with invalid name
    if (!d->isValid())
        d = newBackendTimeZone(ianaId);
}

/*!
    Creates an instance of a time zone with the requested Offset from UTC of
    \a offsetSeconds.

    The \a offsetSeconds from UTC must be in the range -14 hours to +14 hours
    otherwise an invalid time zone will be returned.
*/

QTimeZone::QTimeZone(int offsetSeconds)
{
    // offsetSeconds must fall between -14:00 and +14:00 hours
    if (offsetSeconds >= -50400 && offsetSeconds <= 50400)
        d = new QUtcTimeZonePrivate(offsetSeconds);
    else
        d = 0;
}

/*!
    Creates a custom time zone with an ID of \a ianaId and an offset from UTC
    of \a offsetSeconds.  The \a name will be the name used by displayName()
    for the LongName, the \a abbreviation will be used by displayName() for the
    ShortName and by abbreviation(), and the optional \a country will be used
    by country().  The \a comment is an optional note that may be displayed in
    a GUI to assist users in selecting a time zone.

    The \a ianaId must not be one of the available system IDs returned by
    availableTimeZoneIds().  The \a offsetSeconds from UTC must be in the range
    -14 hours to +14 hours.

    If the custom time zone does not have a specific country then set it to the
    default value of QLocale::AnyCountry.
*/

QTimeZone::QTimeZone(const QByteArray &ianaId, int offsetSeconds, const QString &name,
                     const QString &abbreviation, QLocale::Country country, const QString &comment)
{
    // ianaId must be a valid ID and must not clash with the standard system names
    if (QTimeZonePrivate::isValidId(ianaId) && !availableTimeZoneIds().contains(ianaId))
        d = new QUtcTimeZonePrivate(ianaId, offsetSeconds, name, abbreviation, country, comment);
    else
        d = 0;
}

/*!
    \internal

    Private. Create time zone with given private backend
*/

QTimeZone::QTimeZone(QTimeZonePrivate &dd)
    : d(&dd)
{
}

/*!
    Copy constructor, copy \a other to this.
*/

QTimeZone::QTimeZone(const QTimeZone &other)
    : d(other.d)
{
}

/*!
    Destroys the time zone.
*/

QTimeZone::~QTimeZone()
{
}

/*!
    \fn QTimeZone::swap(QTimeZone &other)

    Swaps this time zone instance with \a other. This function is very
    fast and never fails.
*/

/*!
    Assignment operator, assign \a other to this.
*/

QTimeZone &QTimeZone::operator=(const QTimeZone &other)
{
    d = other.d;
    return *this;
}

/*
    \fn void QTimeZone::swap(QTimeZone &other)

    Swaps this timezone with \a other. This function is very fast and
    never fails.
*/

/*!
    \fn QTimeZone &QTimeZone::operator=(QTimeZone &&other)

    Move-assigns \a other to this QTimeZone instance, transferring the
    ownership of the managed pointer to this instance.
*/

/*!
    Returns \c true if this time zone is equal to the \a other time zone.
*/

bool QTimeZone::operator==(const QTimeZone &other) const
{
    if (d && other.d)
        return (*d == *other.d);
    else
        return (d == other.d);
}

/*!
    Returns \c true if this time zone is not equal to the \a other time zone.
*/

bool QTimeZone::operator!=(const QTimeZone &other) const
{
    if (d && other.d)
        return (*d != *other.d);
    else
        return (d != other.d);
}

/*!
    Returns \c true if this time zone is valid.
*/

bool QTimeZone::isValid() const
{
    if (d)
        return d->isValid();
    else
        return false;
}

/*!
    Returns the IANA ID for the time zone.

    IANA IDs are used on all platforms.  On Windows these are translated
    from the Windows ID into the closest IANA ID for the time zone and country.
*/

QByteArray QTimeZone::id() const
{
    if (d)
        return d->id();
    else
        return QByteArray();
}

/*!
    Returns the country for the time zone.
*/

QLocale::Country QTimeZone::country() const
{
    if (isValid())
        return d->country();
    else
        return QLocale::AnyCountry;
}

/*!
    Returns any comment for the time zone.

    A comment may be provided by the host platform to assist users in
    choosing the correct time zone.  Depending on the platform this may not
    be localized.
*/

QString QTimeZone::comment() const
{
    if (isValid())
        return d->comment();
    else
        return QString();
}

/*!
    Returns the localized time zone display name at the given \a atDateTime
    for the given \a nameType in the given \a locale.  The \a nameType and
    \a locale requested may not be supported on all platforms, in which case
    the best available option will be returned.

    If the \a locale is not provided then the application default locale will
    be used.

    The display name may change depending on daylight time or historical
    events.

    \sa abbreviation()
*/

QString QTimeZone::displayName(const QDateTime &atDateTime, NameType nameType,
                               const QLocale &locale) const
{
    if (isValid())
        return d->displayName(atDateTime.toMSecsSinceEpoch(), nameType, locale);
    else
        return QString();
}

/*!
    Returns the localized time zone display name for the given \a timeType
    and \a nameType in the given \a locale. The \a nameType and \a locale
    requested may not be supported on all platforms, in which case the best
    available option will be returned.

    If the \a locale is not provided then the application default locale will
    be used.

    Where the time zone display names have changed over time then the most
    recent names will be used.

    \sa abbreviation()
*/

QString QTimeZone::displayName(TimeType timeType, NameType nameType,
                               const QLocale &locale) const
{
    if (isValid())
        return d->displayName(timeType, nameType, locale);
    else
        return QString();
}

/*!
    Returns the time zone abbreviation at the given \a atDateTime.  The
    abbreviation may change depending on daylight time or even
    historical events.

    Note that the abbreviation is not guaranteed to be unique to this time zone
    and should not be used in place of the ID or display name.

    \sa displayName()
*/

QString QTimeZone::abbreviation(const QDateTime &atDateTime) const
{
    if (isValid())
        return d->abbreviation(atDateTime.toMSecsSinceEpoch());
    else
        return QString();
}

/*!
    Returns the total effective offset at the given \a atDateTime, i.e. the
    number of seconds to add to UTC to obtain the local time.  This includes
    any daylight time offset that may be in effect, i.e. it is the sum of
    standardTimeOffset() and daylightTimeOffset() for the given datetime.

    For example, for the time zone "Europe/Berlin" the standard time offset is
    +3600 seconds and the daylight time offset is +3600 seconds.  During standard
    time offsetFromUtc() will return +3600 (UTC+01:00), and during daylight time
    it will return +7200 (UTC+02:00).

    \sa standardTimeOffset(), daylightTimeOffset()
*/

int QTimeZone::offsetFromUtc(const QDateTime &atDateTime) const
{
    if (isValid())
        return d->offsetFromUtc(atDateTime.toMSecsSinceEpoch());
    else
        return 0;
}

/*!
    Returns the standard time offset at the given \a atDateTime, i.e. the
    number of seconds to add to UTC to obtain the local Standard Time.  This
    excludes any daylight time offset that may be in effect.

    For example, for the time zone "Europe/Berlin" the standard time offset is
    +3600 seconds.  During both standard and daylight time offsetFromUtc() will
    return +3600 (UTC+01:00).

    \sa offsetFromUtc(), daylightTimeOffset()
*/

int QTimeZone::standardTimeOffset(const QDateTime &atDateTime) const
{
    if (isValid())
        return d->standardTimeOffset(atDateTime.toMSecsSinceEpoch());
    else
        return 0;
}

/*!
    Returns the daylight time offset at the given \a atDateTime, i.e. the
    number of seconds to add to the standard time offset to obtain the local
    daylight time.

    For example, for the time zone "Europe/Berlin" the daylight time offset
    is +3600 seconds.  During standard time daylightTimeOffset() will return
    0, and during daylight time it will return +3600.

    \sa offsetFromUtc(), standardTimeOffset()
*/

int QTimeZone::daylightTimeOffset(const QDateTime &atDateTime) const
{
    if (hasDaylightTime())
        return d->daylightTimeOffset(atDateTime.toMSecsSinceEpoch());
    else
        return 0;
}

/*!
    Returns \c true if the time zone has observed daylight time at any time.

    \sa isDaylightTime(), daylightTimeOffset()
*/

bool QTimeZone::hasDaylightTime() const
{
    if (isValid())
        return d->hasDaylightTime();
    else
        return false;
}

/*!
    Returns \c true if the given \a atDateTime is in daylight time.

    \sa hasDaylightTime(), daylightTimeOffset()
*/

bool QTimeZone::isDaylightTime(const QDateTime &atDateTime) const
{
    if (hasDaylightTime())
        return d->isDaylightTime(atDateTime.toMSecsSinceEpoch());
    else
        return false;
}

/*!
    Returns the effective offset details at the given \a forDateTime. This is
    the equivalent of calling offsetFromUtc(), abbreviation(), etc individually but is
    more efficient.

    \sa offsetFromUtc(), standardTimeOffset(), daylightTimeOffset(), abbreviation()
*/

QTimeZone::OffsetData QTimeZone::offsetData(const QDateTime &forDateTime) const
{
    if (hasTransitions())
        return d->toOffsetData(d->data(forDateTime.toMSecsSinceEpoch()));
    else
        return d->invalidOffsetData();
}

/*!
    Returns \c true if the system backend supports obtaining transitions.
*/

bool QTimeZone::hasTransitions() const
{
    if (isValid())
        return d->hasTransitions();
    else
        return false;
}

/*!
    Returns the first time zone Transition after the given \a afterDateTime.
    This is most useful when you have a Transition time and wish to find the
    Transition after it.

    If there is no transition after the given \a afterDateTime then an invalid
    OffsetData will be returned with an invalid QDateTime.

    The given \a afterDateTime is exclusive.

    \sa hasTransitions(), previousTransition(), transitions()
*/

QTimeZone::OffsetData QTimeZone::nextTransition(const QDateTime &afterDateTime) const
{
    if (hasTransitions())
        return d->toOffsetData(d->nextTransition(afterDateTime.toMSecsSinceEpoch()));
    else
        return d->invalidOffsetData();
}

/*!
    Returns the first time zone Transition before the given \a beforeDateTime.
    This is most useful when you have a Transition time and wish to find the
    Transition before it.

    If there is no transition before the given \a beforeDateTime then an invalid
    OffsetData will be returned with an invalid QDateTime.

    The given \a beforeDateTime is exclusive.

    \sa hasTransitions(), nextTransition(), transitions()
*/

QTimeZone::OffsetData QTimeZone::previousTransition(const QDateTime &beforeDateTime) const
{
    if (hasTransitions())
        return d->toOffsetData(d->previousTransition(beforeDateTime.toMSecsSinceEpoch()));
    else
        return d->invalidOffsetData();
}

/*!
    Returns a list of all time zone transitions between the given datetimes.

    The given \a fromDateTime and \a toDateTime are inclusive.

    \sa hasTransitions(), nextTransition(), previousTransition()
*/

QTimeZone::OffsetDataList QTimeZone::transitions(const QDateTime &fromDateTime,
                                                 const QDateTime &toDateTime) const
{
    OffsetDataList list;
    if (hasTransitions()) {
        QTimeZonePrivate::DataList plist = d->transitions(fromDateTime.toMSecsSinceEpoch(),
                                                          toDateTime.toMSecsSinceEpoch());
        foreach (const QTimeZonePrivate::Data &pdata, plist)
            list.append(d->toOffsetData(pdata));
    }
    return list;
}

// Static methods

/*!
    Returns the current system time zone IANA ID.

    On Windows this ID is translated from the Windows ID using an internal
    translation table and the user's selected country.  As a consequence there
    is a small chance any Windows install may have IDs not known by Qt, in
    which case "UTC" will be returned.
*/

QByteArray QTimeZone::systemTimeZoneId()
{
    return global_tz->backend->systemTimeZoneId();
}

/*!
    Returns \c true if a given time zone \a ianaId is available on this system.

    \sa availableTimeZoneIds()
*/

bool QTimeZone::isTimeZoneIdAvailable(const QByteArray &ianaId)
{
    // isValidId is not strictly required, but faster to weed out invalid
    // IDs as availableTimeZoneIds() may be slow
    return (QTimeZonePrivate::isValidId(ianaId) && (availableTimeZoneIds().contains(ianaId)));
}

/*!
    Returns a list of all available IANA time zone IDs on this system.

    \sa isTimeZoneIdAvailable()
*/

QList<QByteArray> QTimeZone::availableTimeZoneIds()
{
    QSet<QByteArray> set = QUtcTimeZonePrivate().availableTimeZoneIds()
                           + global_tz->backend->availableTimeZoneIds();
    QList<QByteArray> list = set.toList();
    std::sort(list.begin(), list.end());
    return list;
}

/*!
    Returns a list of all available IANA time zone IDs for a given \a country.

    As a special case, a \a country of Qt::AnyCountry returns those time zones
    that do not have any country related to them, such as UTC.  If you require
    a list of all time zone IDs for all countries then use the standard
    availableTimeZoneIds() method.

    \sa isTimeZoneIdAvailable()
*/

QList<QByteArray> QTimeZone::availableTimeZoneIds(QLocale::Country country)
{
    QSet<QByteArray> set = QUtcTimeZonePrivate().availableTimeZoneIds(country)
                           + global_tz->backend->availableTimeZoneIds(country);
    QList<QByteArray> list = set.toList();
    std::sort(list.begin(), list.end());
    return list;
}

/*!
    Returns a list of all available IANA time zone IDs with a given standard
    time offset of \a offsetSeconds.

    \sa isTimeZoneIdAvailable()
*/

QList<QByteArray> QTimeZone::availableTimeZoneIds(int offsetSeconds)
{
    QSet<QByteArray> set = QUtcTimeZonePrivate().availableTimeZoneIds(offsetSeconds)
                           + global_tz->backend->availableTimeZoneIds(offsetSeconds);
    QList<QByteArray> list = set.toList();
    std::sort(list.begin(), list.end());
    return list;
}

/*!
    Returns the Windows ID equivalent to the given \a ianaId.

    \sa windowsIdToDefaultIanaId(), windowsIdToIanaIds()
*/

QByteArray QTimeZone::ianaIdToWindowsId(const QByteArray &ianaId)
{
    return QTimeZonePrivate::ianaIdToWindowsId(ianaId);
}

/*!
    Returns the default IANA ID for a given \a windowsId.

    Because a Windows ID can cover several IANA IDs in several different
    countries, this function returns the most frequently used IANA ID with no
    regard for the country and should thus be used with care.  It is usually
    best to request the default for a specific country.

    \sa ianaIdToWindowsId(), windowsIdToIanaIds()
*/

QByteArray QTimeZone::windowsIdToDefaultIanaId(const QByteArray &windowsId)
{
    return QTimeZonePrivate::windowsIdToDefaultIanaId(windowsId);
}

/*!
    Returns the default IANA ID for a given \a windowsId and \a country.

    Because a Windows ID can cover several IANA IDs within a given country,
    the most frequently used IANA ID in that country is returned.

    As a special case, QLocale::AnyCountry returns the default of those IANA IDs
    that do not have any specific country.

    \sa ianaIdToWindowsId(), windowsIdToIanaIds()
*/

QByteArray QTimeZone::windowsIdToDefaultIanaId(const QByteArray &windowsId,
                                                QLocale::Country country)
{
    return QTimeZonePrivate::windowsIdToDefaultIanaId(windowsId, country);
}

/*!
    Returns all the IANA IDs for a given \a windowsId.

    The returned list is sorted alphabetically.

    \sa ianaIdToWindowsId(), windowsIdToDefaultIanaId()
*/

QList<QByteArray> QTimeZone::windowsIdToIanaIds(const QByteArray &windowsId)
{
    return QTimeZonePrivate::windowsIdToIanaIds(windowsId);
}

/*!
    Returns all the IANA IDs for a given \a windowsId and \a country.

    As a special case QLocale::AnyCountry returns those IANA IDs that do
    not have any specific country.

    The returned list is in order of frequency of usage, i.e. larger zones
    within a country are listed first.

    \sa ianaIdToWindowsId(), windowsIdToDefaultIanaId()
*/

QList<QByteArray> QTimeZone::windowsIdToIanaIds(const QByteArray &windowsId,
                                                    QLocale::Country country)
{
    return QTimeZonePrivate::windowsIdToIanaIds(windowsId, country);
}

#ifndef QT_NO_DATASTREAM
QDataStream &operator<<(QDataStream &ds, const QTimeZone &tz)
{
    tz.d->serialize(ds);
    return ds;
}

QDataStream &operator>>(QDataStream &ds, QTimeZone &tz)
{
    QString ianaId;
    ds >> ianaId;
    if (ianaId == QStringLiteral("OffsetFromUtc")) {
        int utcOffset;
        QString name;
        QString abbreviation;
        int country;
        QString comment;
        ds >> ianaId >> utcOffset >> name >> abbreviation >> country >> comment;
        tz = QTimeZone(ianaId.toUtf8(), utcOffset, name, abbreviation, (QLocale::Country) country, comment);
    } else {
        tz = QTimeZone(ianaId.toUtf8());
    }
    return ds;
}
#endif // QT_NO_DATASTREAM

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QTimeZone &tz)
{
    //TODO Include backend and data version details?
    dbg.nospace() << QStringLiteral("QTimeZone(") << qPrintable(QString::fromUtf8(tz.id())) << ')';
    return dbg.space();
}
#endif

QT_END_NAMESPACE
