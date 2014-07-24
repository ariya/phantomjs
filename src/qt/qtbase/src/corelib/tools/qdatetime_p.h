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

#ifndef QDATETIME_P_H
#define QDATETIME_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qplatformdefs.h"
#include "QtCore/qatomic.h"
#include "QtCore/qdatetime.h"

#include "qtimezone.h"

QT_BEGIN_NAMESPACE

class QDateTimePrivate : public QSharedData
{
public:
    // Never change or delete this enum, it is required for backwards compatible
    // serialization of QDateTime before 5.2, so is essentially public API
    enum Spec {
        LocalUnknown = -1,
        LocalStandard = 0,
        LocalDST = 1,
        UTC = 2,
        OffsetFromUTC = 3,
        TimeZone = 4
    };

    // Daylight Time Status
    enum DaylightStatus {
        NoDaylightTime = -2,
        UnknownDaylightTime = -1,
        StandardTime = 0,
        DaylightTime = 1
    };

    // Status of date/time
    enum StatusFlag {
        NullDate            = 0x01,
        NullTime            = 0x02,
        ValidDate           = 0x04,
        ValidTime           = 0x08,
        ValidDateTime       = 0x10,
        TimeZoneCached      = 0x20,
        SetToStandardTime   = 0x40,
        SetToDaylightTime   = 0x80
    };
    Q_DECLARE_FLAGS(StatusFlags, StatusFlag)

    QDateTimePrivate() : m_msecs(0),
                         m_spec(Qt::LocalTime),
                         m_offsetFromUtc(0),
                         m_status(NullDate | NullTime)
    {}

    QDateTimePrivate(const QDate &toDate, const QTime &toTime, Qt::TimeSpec toSpec,
                     int offsetSeconds);

#ifndef QT_BOOTSTRAPPED
    QDateTimePrivate(const QDate &toDate, const QTime &toTime, const QTimeZone & timeZone);
#endif // QT_BOOTSTRAPPED

    QDateTimePrivate(const QDateTimePrivate &other) : QSharedData(other),
                                                      m_msecs(other.m_msecs),
                                                      m_spec(other.m_spec),
                                                      m_offsetFromUtc(other.m_offsetFromUtc),
#ifndef QT_BOOTSTRAPPED
                                                      m_timeZone(other.m_timeZone),
#endif // QT_BOOTSTRAPPED
                                                      m_status(other.m_status)
    {}

    qint64 m_msecs;
    Qt::TimeSpec m_spec;
    int m_offsetFromUtc;
#ifndef QT_BOOTSTRAPPED
    QTimeZone m_timeZone;
#endif // QT_BOOTSTRAPPED
    StatusFlags m_status;

    void setTimeSpec(Qt::TimeSpec spec, int offsetSeconds);
    void setDateTime(const QDate &date, const QTime &time);
    void getDateTime(QDate *date, QTime *time) const;

    void setDaylightStatus(DaylightStatus status);
    DaylightStatus daylightStatus() const;

    // Returns msecs since epoch, assumes offset value is current
    inline qint64 toMSecsSinceEpoch() const { return (m_msecs - (m_offsetFromUtc * 1000)); }

    void checkValidDateTime();
    void refreshDateTime();

    // Get/set date and time status
    inline bool isNullDate() const { return (m_status & NullDate) == NullDate; }
    inline bool isNullTime() const { return (m_status & NullTime) == NullTime; }
    inline bool isValidDate() const { return (m_status & ValidDate) == ValidDate; }
    inline bool isValidTime() const { return (m_status & ValidTime) == ValidTime; }
    inline bool isValidDateTime() const { return (m_status & ValidDateTime) == ValidDateTime; }
    inline void setValidDateTime() { m_status = m_status | ValidDateTime; }
    inline void clearValidDateTime() { m_status = m_status & ~ValidDateTime; }
    inline bool isTimeZoneCached() const { return (m_status & TimeZoneCached) == TimeZoneCached; }
    inline void setTimeZoneCached() { m_status = m_status | TimeZoneCached; }
    inline void clearTimeZoneCached() { m_status = m_status & ~TimeZoneCached; }
    inline void clearSetToDaylightStatus() { m_status = m_status & ~SetToStandardTime & ~SetToDaylightTime; }

#ifndef QT_BOOTSTRAPPED
    static qint64 zoneMSecsToEpochMSecs(qint64 msecs, const QTimeZone &zone,
                                        QDate *localDate, QTime *localTime);
#endif // QT_BOOTSTRAPPED

    static inline qint64 minJd() { return QDate::minJd(); }
    static inline qint64 maxJd() { return QDate::maxJd(); }
};

QT_END_NAMESPACE

#endif // QDATETIME_P_H
