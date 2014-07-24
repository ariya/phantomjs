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

#include "private/qcore_mac_p.h"
#include "qstringlist.h"

#include <Foundation/NSTimeZone.h>

#include <qdebug.h>

QT_BEGIN_NAMESPACE

/*
    Private

    OS X system implementation
*/

// Create the system default time zone
QMacTimeZonePrivate::QMacTimeZonePrivate()
{
    init(systemTimeZoneId());
}

// Create a named time zone
QMacTimeZonePrivate::QMacTimeZonePrivate(const QByteArray &ianaId)
    : m_nstz(0)
{
    init(ianaId);
}

QMacTimeZonePrivate::QMacTimeZonePrivate(const QMacTimeZonePrivate &other)
    : QTimeZonePrivate(other), m_nstz(0)
{
    m_nstz = [other.m_nstz copy];
}

QMacTimeZonePrivate::~QMacTimeZonePrivate()
{
    [m_nstz release];
}

QTimeZonePrivate *QMacTimeZonePrivate::clone()
{
    return new QMacTimeZonePrivate(*this);
}

void QMacTimeZonePrivate::init(const QByteArray &ianaId)
{
    if (availableTimeZoneIds().contains(ianaId)) {
        m_nstz = [[NSTimeZone timeZoneWithName:QCFString::toNSString(QString::fromUtf8(ianaId))] retain];
        if (m_nstz)
            m_id = ianaId;
    }
}

QString QMacTimeZonePrivate::comment() const
{
    return QCFString::toQString([m_nstz description]);
}

QString QMacTimeZonePrivate::displayName(QTimeZone::TimeType timeType,
                                         QTimeZone::NameType nameType,
                                         const QLocale &locale) const
{
    // TODO Mac doesn't support OffsetName yet so use standard offset name
    if (nameType == QTimeZone::OffsetName) {
        const Data nowData = data(QDateTime::currentDateTimeUtc().toMSecsSinceEpoch());
        // TODO Cheat for now, assume if has dst the offset if 1 hour
        if (timeType == QTimeZone::DaylightTime && hasDaylightTime())
            return isoOffsetFormat(nowData.standardTimeOffset + 3600);
        else
            return isoOffsetFormat(nowData.standardTimeOffset);
    }

    NSTimeZoneNameStyle style = NSTimeZoneNameStyleStandard;

    switch (nameType) {
    case QTimeZone::ShortName :
        if (timeType == QTimeZone::DaylightTime)
            style = NSTimeZoneNameStyleShortDaylightSaving;
        else if (timeType == QTimeZone::GenericTime)
            style = NSTimeZoneNameStyleShortGeneric;
        else
            style = NSTimeZoneNameStyleShortStandard;
        break;
    case QTimeZone::DefaultName :
    case QTimeZone::LongName :
        if (timeType == QTimeZone::DaylightTime)
            style = NSTimeZoneNameStyleDaylightSaving;
        else if (timeType == QTimeZone::GenericTime)
            style = NSTimeZoneNameStyleGeneric;
        else
            style = NSTimeZoneNameStyleStandard;
        break;
    case QTimeZone::OffsetName :
        // Unreachable
        break;
    }

    NSString *macLocaleCode = QCFString::toNSString(locale.name());
    NSLocale *macLocale = [[NSLocale alloc] initWithLocaleIdentifier:macLocaleCode];
    const QString result = QCFString::toQString([m_nstz localizedName:style locale:macLocale]);
    [macLocale release];
    return result;
}

QString QMacTimeZonePrivate::abbreviation(qint64 atMSecsSinceEpoch) const
{
    const NSTimeInterval seconds = atMSecsSinceEpoch / 1000.0;
    return QCFString::toQString([m_nstz abbreviationForDate:[NSDate dateWithTimeIntervalSince1970:seconds]]);
}

int QMacTimeZonePrivate::offsetFromUtc(qint64 atMSecsSinceEpoch) const
{
    const NSTimeInterval seconds = atMSecsSinceEpoch / 1000.0;
    return [m_nstz secondsFromGMTForDate:[NSDate dateWithTimeIntervalSince1970:seconds]];
}

int QMacTimeZonePrivate::standardTimeOffset(qint64 atMSecsSinceEpoch) const
{
    return offsetFromUtc(atMSecsSinceEpoch) - daylightTimeOffset(atMSecsSinceEpoch);
}

int QMacTimeZonePrivate::daylightTimeOffset(qint64 atMSecsSinceEpoch) const
{
    const NSTimeInterval seconds = atMSecsSinceEpoch / 1000.0;
    return [m_nstz daylightSavingTimeOffsetForDate:[NSDate dateWithTimeIntervalSince1970:seconds]];
}

bool QMacTimeZonePrivate::hasDaylightTime() const
{
    // TODO No Mac API, assume if has transitions
    return hasTransitions();
}

bool QMacTimeZonePrivate::isDaylightTime(qint64 atMSecsSinceEpoch) const
{
    const NSTimeInterval seconds = atMSecsSinceEpoch / 1000.0;
    return [m_nstz isDaylightSavingTimeForDate:[NSDate dateWithTimeIntervalSince1970:seconds]];
}

QTimeZonePrivate::Data QMacTimeZonePrivate::data(qint64 forMSecsSinceEpoch) const
{
    const NSTimeInterval seconds = forMSecsSinceEpoch / 1000.0;
    NSDate *date = [NSDate dateWithTimeIntervalSince1970:seconds];
    Data data;
    data.atMSecsSinceEpoch = forMSecsSinceEpoch;
    data.offsetFromUtc = [m_nstz secondsFromGMTForDate:date];
    data.daylightTimeOffset = [m_nstz daylightSavingTimeOffsetForDate:date];
    data.standardTimeOffset = data.offsetFromUtc - data.daylightTimeOffset;
    data.abbreviation = QCFString::toQString([m_nstz abbreviationForDate:date]);
    return data;
}

bool QMacTimeZonePrivate::hasTransitions() const
{
    // TODO No direct Mac API, so return if has next after 1970, i.e. since start of tz
    // TODO Not sure what is returned in event of no transitions, assume will be before requested date
    NSDate *epoch = [NSDate dateWithTimeIntervalSince1970:0];
    const NSDate *date = [m_nstz nextDaylightSavingTimeTransitionAfterDate:epoch];
    const bool result = ([date timeIntervalSince1970] > [epoch timeIntervalSince1970]);
    return result;
}

QTimeZonePrivate::Data QMacTimeZonePrivate::nextTransition(qint64 afterMSecsSinceEpoch) const
{
    QTimeZonePrivate::Data tran;
    const NSTimeInterval seconds = afterMSecsSinceEpoch / 1000.0;
    NSDate *nextDate = [NSDate dateWithTimeIntervalSince1970:seconds];
    nextDate = [m_nstz nextDaylightSavingTimeTransitionAfterDate:nextDate];
    const NSTimeInterval nextSecs = [nextDate timeIntervalSince1970];
    if (nextDate == nil || nextSecs <= seconds) {
        [nextDate release];
        return invalidData();
    }
    tran.atMSecsSinceEpoch = nextSecs * 1000;
    tran.offsetFromUtc = [m_nstz secondsFromGMTForDate:nextDate];
    tran.daylightTimeOffset = [m_nstz daylightSavingTimeOffsetForDate:nextDate];
    tran.standardTimeOffset = tran.offsetFromUtc - tran.daylightTimeOffset;
    tran.abbreviation = QCFString::toQString([m_nstz abbreviationForDate:nextDate]);
    return tran;
}

QTimeZonePrivate::Data QMacTimeZonePrivate::previousTransition(qint64 beforeMSecsSinceEpoch) const
{
    // No direct Mac API, so get all transitions since epoch and return the last one
    QList<int> secsList;
    if (beforeMSecsSinceEpoch > 0) {
        const int endSecs = beforeMSecsSinceEpoch / 1000.0;
        NSTimeInterval prevSecs = 0;
        NSTimeInterval nextSecs = 0;
        NSDate *nextDate = [NSDate dateWithTimeIntervalSince1970:nextSecs];
        // If invalid may return a nil date or an Epoch date
        nextDate = [m_nstz nextDaylightSavingTimeTransitionAfterDate:nextDate];
        nextSecs = [nextDate timeIntervalSince1970];
        while (nextDate != nil && nextSecs > prevSecs && nextSecs < endSecs) {
            secsList.append(nextSecs);
            prevSecs = nextSecs;
            nextDate = [m_nstz nextDaylightSavingTimeTransitionAfterDate:nextDate];
            nextSecs = [nextDate timeIntervalSince1970];
        }
    }
    if (secsList.size() >= 1)
        return data(qint64(secsList.last()) * 1000);
    else
        return invalidData();
}

QByteArray QMacTimeZonePrivate::systemTimeZoneId() const
{
    // Reset the cached system tz then return the name
    [NSTimeZone resetSystemTimeZone];
    return QCFString::toQString([[NSTimeZone systemTimeZone] name]).toUtf8();
}

QSet<QByteArray> QMacTimeZonePrivate::availableTimeZoneIds() const
{
    NSEnumerator *enumerator = [[NSTimeZone knownTimeZoneNames] objectEnumerator];
    QByteArray tzid = QCFString::toQString([enumerator nextObject]).toUtf8();

    QSet<QByteArray> set;
    while (!tzid.isEmpty()) {
        set << tzid;
        tzid = QCFString::toQString([enumerator nextObject]).toUtf8();
    }

    return set;
}

QT_END_NAMESPACE
