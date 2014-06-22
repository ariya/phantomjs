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


#ifndef QTIMEZONEPRIVATE_P_H
#define QTIMEZONEPRIVATE_P_H

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

#include "qtimezone.h"
#include "qlocale_p.h"

#ifdef QT_USE_ICU
#include <unicode/ucal.h>
#endif // QT_USE_ICU

#ifdef Q_OS_MAC
#ifdef __OBJC__
@class NSTimeZone;
#else
class NSTimeZone;
#endif // __OBJC__
#endif // Q_OS_MAC

#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif // Q_OS_WIN

QT_BEGIN_NAMESPACE

class Q_CORE_EXPORT QTimeZonePrivate : public QSharedData
{
public:
    //Version of QTimeZone::OffsetData struct using msecs for efficiency
    struct Data {
        QString abbreviation;
        qint64 atMSecsSinceEpoch;
        int offsetFromUtc;
        int standardTimeOffset;
        int daylightTimeOffset;
    };
    typedef QVector<Data> DataList;

    // Create null time zone
    QTimeZonePrivate();
    QTimeZonePrivate(const QTimeZonePrivate &other);
    virtual ~QTimeZonePrivate();

    virtual QTimeZonePrivate *clone();

    bool operator==(const QTimeZonePrivate &other) const;
    bool operator!=(const QTimeZonePrivate &other) const;

    bool isValid() const;

    QByteArray id() const;
    virtual QLocale::Country country() const;
    virtual QString comment() const;

    virtual QString displayName(qint64 atMSecsSinceEpoch,
                                QTimeZone::NameType nameType,
                                const QLocale &locale) const;
    virtual QString displayName(QTimeZone::TimeType timeType,
                                QTimeZone::NameType nameType,
                                const QLocale &locale) const;
    virtual QString abbreviation(qint64 atMSecsSinceEpoch) const;

    virtual int offsetFromUtc(qint64 atMSecsSinceEpoch) const;
    virtual int standardTimeOffset(qint64 atMSecsSinceEpoch) const;
    virtual int daylightTimeOffset(qint64 atMSecsSinceEpoch) const;

    virtual bool hasDaylightTime() const;
    virtual bool isDaylightTime(qint64 atMSecsSinceEpoch) const;

    virtual Data data(qint64 forMSecsSinceEpoch) const;
    virtual Data dataForLocalTime(qint64 forLocalMSecs) const;

    virtual bool hasTransitions() const;
    virtual Data nextTransition(qint64 afterMSecsSinceEpoch) const;
    virtual Data previousTransition(qint64 beforeMSecsSinceEpoch) const;
    DataList transitions(qint64 fromMSecsSinceEpoch, qint64 toMSecsSinceEpoch) const;

    virtual QByteArray systemTimeZoneId() const;

    virtual QSet<QByteArray> availableTimeZoneIds() const;
    virtual QSet<QByteArray> availableTimeZoneIds(QLocale::Country country) const;
    virtual QSet<QByteArray> availableTimeZoneIds(int utcOffset) const;

    virtual void serialize(QDataStream &ds) const;

    // Static Utility Methods
    static inline qint64 maxMSecs() { return std::numeric_limits<qint64>::max(); }
    static inline qint64 minMSecs() { return std::numeric_limits<qint64>::min() + 1; }
    static inline qint64 invalidMSecs() { return std::numeric_limits<qint64>::min(); }
    static inline qint64 invalidSeconds() { return std::numeric_limits<int>::min(); }
    static Data invalidData();
    static QTimeZone::OffsetData invalidOffsetData();
    static QTimeZone::OffsetData toOffsetData(const Data &data);
    static bool isValidId(const QByteArray &ianaId);
    static QString isoOffsetFormat(int offsetFromUtc);

    static QByteArray ianaIdToWindowsId(const QByteArray &ianaId);
    static QByteArray windowsIdToDefaultIanaId(const QByteArray &windowsId);
    static QByteArray windowsIdToDefaultIanaId(const QByteArray &windowsId,
                                                QLocale::Country country);
    static QList<QByteArray> windowsIdToIanaIds(const QByteArray &windowsId);
    static QList<QByteArray> windowsIdToIanaIds(const QByteArray &windowsId,
                                                 QLocale::Country country);

protected:
    QByteArray m_id;
};

template<> QTimeZonePrivate *QSharedDataPointer<QTimeZonePrivate>::clone();

class Q_AUTOTEST_EXPORT QUtcTimeZonePrivate Q_DECL_FINAL : public QTimeZonePrivate
{
public:
    // Create default UTC time zone
    QUtcTimeZonePrivate();
    // Create named time zone
    QUtcTimeZonePrivate(const QByteArray &utcId);
    // Create offset from UTC
    QUtcTimeZonePrivate(int offsetSeconds);
    // Create custom offset from UTC
    QUtcTimeZonePrivate(const QByteArray &zoneId, int offsetSeconds, const QString &name,
                        const QString &abbreviation, QLocale::Country country,
                        const QString &comment);
    QUtcTimeZonePrivate(const QUtcTimeZonePrivate &other);
    virtual ~QUtcTimeZonePrivate();

    QTimeZonePrivate *clone();

    QLocale::Country country() const Q_DECL_OVERRIDE;
    QString comment() const Q_DECL_OVERRIDE;

    QString displayName(QTimeZone::TimeType timeType,
                        QTimeZone::NameType nameType,
                        const QLocale &locale) const Q_DECL_OVERRIDE;
    QString abbreviation(qint64 atMSecsSinceEpoch) const Q_DECL_OVERRIDE;

    int standardTimeOffset(qint64 atMSecsSinceEpoch) const Q_DECL_OVERRIDE;
    int daylightTimeOffset(qint64 atMSecsSinceEpoch) const Q_DECL_OVERRIDE;

    QByteArray systemTimeZoneId() const Q_DECL_OVERRIDE;

    QSet<QByteArray> availableTimeZoneIds() const Q_DECL_OVERRIDE;
    QSet<QByteArray> availableTimeZoneIds(QLocale::Country country) const Q_DECL_OVERRIDE;
    QSet<QByteArray> availableTimeZoneIds(int utcOffset) const Q_DECL_OVERRIDE;

    void serialize(QDataStream &ds) const Q_DECL_OVERRIDE;

private:
    void init(const QByteArray &zoneId);
    void init(const QByteArray &zoneId, int offsetSeconds, const QString &name,
              const QString &abbreviation, QLocale::Country country,
              const QString &comment);

    QString m_name;
    QString m_abbreviation;
    QString m_comment;
    QLocale::Country m_country;
    int m_offsetFromUtc;
};

#ifdef QT_USE_ICU
class Q_AUTOTEST_EXPORT QIcuTimeZonePrivate Q_DECL_FINAL : public QTimeZonePrivate
{
public:
    // Create default time zone
    QIcuTimeZonePrivate();
    // Create named time zone
    QIcuTimeZonePrivate(const QByteArray &ianaId);
    QIcuTimeZonePrivate(const QIcuTimeZonePrivate &other);
    ~QIcuTimeZonePrivate();

    QTimeZonePrivate *clone();

    QString displayName(QTimeZone::TimeType timeType, QTimeZone::NameType nameType,
                        const QLocale &locale) const Q_DECL_OVERRIDE;
    QString abbreviation(qint64 atMSecsSinceEpoch) const Q_DECL_OVERRIDE;

    int offsetFromUtc(qint64 atMSecsSinceEpoch) const Q_DECL_OVERRIDE;
    int standardTimeOffset(qint64 atMSecsSinceEpoch) const Q_DECL_OVERRIDE;
    int daylightTimeOffset(qint64 atMSecsSinceEpoch) const Q_DECL_OVERRIDE;

    bool hasDaylightTime() const Q_DECL_OVERRIDE;
    bool isDaylightTime(qint64 atMSecsSinceEpoch) const Q_DECL_OVERRIDE;

    Data data(qint64 forMSecsSinceEpoch) const Q_DECL_OVERRIDE;

    bool hasTransitions() const Q_DECL_OVERRIDE;
    Data nextTransition(qint64 afterMSecsSinceEpoch) const Q_DECL_OVERRIDE;
    Data previousTransition(qint64 beforeMSecsSinceEpoch) const Q_DECL_OVERRIDE;

    QByteArray systemTimeZoneId() const Q_DECL_OVERRIDE;

    QSet<QByteArray> availableTimeZoneIds() const Q_DECL_OVERRIDE;
    QSet<QByteArray> availableTimeZoneIds(QLocale::Country country) const Q_DECL_OVERRIDE;
    QSet<QByteArray> availableTimeZoneIds(int offsetFromUtc) const Q_DECL_OVERRIDE;

private:
    void init(const QByteArray &ianaId);

    UCalendar *m_ucal;
};
#endif // QT_USE_ICU

#if defined Q_OS_UNIX && !defined Q_OS_MAC
class Q_AUTOTEST_EXPORT QTzTimeZonePrivate Q_DECL_FINAL : public QTimeZonePrivate
{
public:
    // Create default time zone
    QTzTimeZonePrivate();
    // Create named time zone
    QTzTimeZonePrivate(const QByteArray &ianaId);
    QTzTimeZonePrivate(const QTzTimeZonePrivate &other);
    ~QTzTimeZonePrivate();

    QTimeZonePrivate *clone();

    QLocale::Country country() const Q_DECL_OVERRIDE;
    QString comment() const Q_DECL_OVERRIDE;

    QString displayName(qint64 atMSecsSinceEpoch,
                        QTimeZone::NameType nameType,
                        const QLocale &locale) const Q_DECL_OVERRIDE;
    QString displayName(QTimeZone::TimeType timeType,
                        QTimeZone::NameType nameType,
                        const QLocale &locale) const Q_DECL_OVERRIDE;
    QString abbreviation(qint64 atMSecsSinceEpoch) const Q_DECL_OVERRIDE;

    int offsetFromUtc(qint64 atMSecsSinceEpoch) const Q_DECL_OVERRIDE;
    int standardTimeOffset(qint64 atMSecsSinceEpoch) const Q_DECL_OVERRIDE;
    int daylightTimeOffset(qint64 atMSecsSinceEpoch) const Q_DECL_OVERRIDE;

    bool hasDaylightTime() const Q_DECL_OVERRIDE;
    bool isDaylightTime(qint64 atMSecsSinceEpoch) const Q_DECL_OVERRIDE;

    Data data(qint64 forMSecsSinceEpoch) const Q_DECL_OVERRIDE;

    bool hasTransitions() const Q_DECL_OVERRIDE;
    Data nextTransition(qint64 afterMSecsSinceEpoch) const Q_DECL_OVERRIDE;
    Data previousTransition(qint64 beforeMSecsSinceEpoch) const Q_DECL_OVERRIDE;

    QByteArray systemTimeZoneId() const Q_DECL_OVERRIDE;

    QSet<QByteArray> availableTimeZoneIds() const Q_DECL_OVERRIDE;
    QSet<QByteArray> availableTimeZoneIds(QLocale::Country country) const Q_DECL_OVERRIDE;

private:
    void init(const QByteArray &ianaId);

    struct QTzTransitionTime {
        qint64 atMSecsSinceEpoch;
        quint8 ruleIndex;
    };
    struct QTzTransitionRule {
        int stdOffset;
        int dstOffset;
        quint8 abbreviationIndex;
        bool operator==(const QTzTransitionRule &other) { return (stdOffset == other.stdOffset
        && dstOffset == other.dstOffset && abbreviationIndex == other.abbreviationIndex); }
    };
    Data dataForTzTransition(QTzTransitionTime tran) const;
    QList<QTzTransitionTime> m_tranTimes;
    QList<QTzTransitionRule> m_tranRules;
    QList<QByteArray> m_abbreviations;
#ifdef QT_USE_ICU
    mutable QSharedDataPointer<QTimeZonePrivate> m_icu;
#endif // QT_USE_ICU
    QByteArray m_posixRule;
};
#endif // Q_OS_UNIX

#ifdef Q_OS_MAC
class Q_AUTOTEST_EXPORT QMacTimeZonePrivate Q_DECL_FINAL : public QTimeZonePrivate
{
public:
    // Create default time zone
    QMacTimeZonePrivate();
    // Create named time zone
    QMacTimeZonePrivate(const QByteArray &ianaId);
    QMacTimeZonePrivate(const QMacTimeZonePrivate &other);
    ~QMacTimeZonePrivate();

    QTimeZonePrivate *clone();

    QString comment() const Q_DECL_OVERRIDE;

    QString displayName(QTimeZone::TimeType timeType, QTimeZone::NameType nameType,
                        const QLocale &locale) const Q_DECL_OVERRIDE;
    QString abbreviation(qint64 atMSecsSinceEpoch) const Q_DECL_OVERRIDE;

    int offsetFromUtc(qint64 atMSecsSinceEpoch) const Q_DECL_OVERRIDE;
    int standardTimeOffset(qint64 atMSecsSinceEpoch) const Q_DECL_OVERRIDE;
    int daylightTimeOffset(qint64 atMSecsSinceEpoch) const Q_DECL_OVERRIDE;

    bool hasDaylightTime() const Q_DECL_OVERRIDE;
    bool isDaylightTime(qint64 atMSecsSinceEpoch) const Q_DECL_OVERRIDE;

    Data data(qint64 forMSecsSinceEpoch) const Q_DECL_OVERRIDE;

    bool hasTransitions() const Q_DECL_OVERRIDE;
    Data nextTransition(qint64 afterMSecsSinceEpoch) const Q_DECL_OVERRIDE;
    Data previousTransition(qint64 beforeMSecsSinceEpoch) const Q_DECL_OVERRIDE;

    QByteArray systemTimeZoneId() const Q_DECL_OVERRIDE;

    QSet<QByteArray> availableTimeZoneIds() const Q_DECL_OVERRIDE;

private:
    void init(const QByteArray &zoneId);

    NSTimeZone *m_nstz;
};
#endif // Q_OS_MAC

#ifdef Q_OS_WIN
class Q_AUTOTEST_EXPORT QWinTimeZonePrivate Q_DECL_FINAL : public QTimeZonePrivate
{
public:
    struct QWinTransitionRule {
        int startYear;
        int standardTimeBias;
        int daylightTimeBias;
        SYSTEMTIME standardTimeRule;
        SYSTEMTIME daylightTimeRule;
    };

    // Create default time zone
    QWinTimeZonePrivate();
    // Create named time zone
    QWinTimeZonePrivate(const QByteArray &ianaId);
    QWinTimeZonePrivate(const QWinTimeZonePrivate &other);
    ~QWinTimeZonePrivate();

    QTimeZonePrivate *clone();

    QString comment() const Q_DECL_OVERRIDE;

    QString displayName(QTimeZone::TimeType timeType, QTimeZone::NameType nameType,
                        const QLocale &locale) const Q_DECL_OVERRIDE;
    QString abbreviation(qint64 atMSecsSinceEpoch) const Q_DECL_OVERRIDE;

    int offsetFromUtc(qint64 atMSecsSinceEpoch) const Q_DECL_OVERRIDE;
    int standardTimeOffset(qint64 atMSecsSinceEpoch) const Q_DECL_OVERRIDE;
    int daylightTimeOffset(qint64 atMSecsSinceEpoch) const Q_DECL_OVERRIDE;

    bool hasDaylightTime() const Q_DECL_OVERRIDE;
    bool isDaylightTime(qint64 atMSecsSinceEpoch) const Q_DECL_OVERRIDE;

    Data data(qint64 forMSecsSinceEpoch) const Q_DECL_OVERRIDE;

    bool hasTransitions() const Q_DECL_OVERRIDE;
    Data nextTransition(qint64 afterMSecsSinceEpoch) const Q_DECL_OVERRIDE;
    Data previousTransition(qint64 beforeMSecsSinceEpoch) const Q_DECL_OVERRIDE;

    QByteArray systemTimeZoneId() const Q_DECL_OVERRIDE;

    QSet<QByteArray> availableTimeZoneIds() const Q_DECL_OVERRIDE;

private:
    void init(const QByteArray &ianaId);
    QWinTransitionRule ruleForYear(int year) const;
    QTimeZonePrivate::Data ruleToData(const QWinTransitionRule &rule, qint64 atMSecsSinceEpoch,
                                      QTimeZone::TimeType type) const;

    QByteArray m_windowsId;
    QString m_displayName;
    QString m_standardName;
    QString m_daylightName;
    QList<QWinTransitionRule> m_tranRules;
};
#endif // Q_OS_WIN

QT_END_NAMESPACE

#endif // QTIMEZONEPRIVATE_P_H
