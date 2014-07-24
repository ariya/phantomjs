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

#include "qplatformdefs.h"
#include "private/qdatetime_p.h"
#include "private/qdatetimeparser_p.h"

#include "qdatastream.h"
#include "qset.h"
#include "qlocale.h"
#include "qdatetime.h"
#include "qtimezoneprivate_p.h"
#include "qregexp.h"
#include "qdebug.h"
#ifndef Q_OS_WIN
#include <locale.h>
#endif

#include <cmath>
#include <time.h>
#ifdef Q_OS_WIN
#  include <qt_windows.h>
#  ifdef Q_OS_WINCE
#    include "qfunctions_wince.h"
#  endif
#  ifdef Q_OS_WINRT
#    include "qfunctions_winrt.h"
#  endif
#endif

#if defined(Q_OS_MAC)
#include <private/qcore_mac_p.h>
#endif

QT_BEGIN_NAMESPACE

/*****************************************************************************
  Date/Time Constants
 *****************************************************************************/

enum {
    SECS_PER_DAY = 86400,
    MSECS_PER_DAY = 86400000,
    SECS_PER_HOUR = 3600,
    MSECS_PER_HOUR = 3600000,
    SECS_PER_MIN = 60,
    MSECS_PER_MIN = 60000,
    TIME_T_MAX = 2145916799,  // int maximum 2037-12-31T23:59:59 UTC
    JULIAN_DAY_FOR_EPOCH = 2440588 // result of julianDayFromDate(1970, 1, 1)
};

/*****************************************************************************
  QDate static helper functions
 *****************************************************************************/

static inline QDate fixedDate(int y, int m, int d)
{
    QDate result(y, m, 1);
    result.setDate(y, m, qMin(d, result.daysInMonth()));
    return result;
}

static inline qint64 floordiv(qint64 a, int b)
{
    return (a - (a < 0 ? b-1 : 0)) / b;
}

static inline int floordiv(int a, int b)
{
    return (a - (a < 0 ? b-1 : 0)) / b;
}

static inline qint64 julianDayFromDate(int year, int month, int day)
{
    // Adjust for no year 0
    if (year < 0)
        ++year;

/*
 * Math from The Calendar FAQ at http://www.tondering.dk/claus/cal/julperiod.php
 * This formula is correct for all julian days, when using mathematical integer
 * division (round to negative infinity), not c++11 integer division (round to zero)
 */
    int    a = floordiv(14 - month, 12);
    qint64 y = (qint64)year + 4800 - a;
    int    m = month + 12 * a - 3;
    return day + floordiv(153 * m + 2, 5) + 365 * y + floordiv(y, 4) - floordiv(y, 100) + floordiv(y, 400) - 32045;
}

static void getDateFromJulianDay(qint64 julianDay, int *yearp, int *monthp, int *dayp)
{
/*
 * Math from The Calendar FAQ at http://www.tondering.dk/claus/cal/julperiod.php
 * This formula is correct for all julian days, when using mathematical integer
 * division (round to negative infinity), not c++11 integer division (round to zero)
 */
    qint64 a = julianDay + 32044;
    qint64 b = floordiv(4 * a + 3, 146097);
    int    c = a - floordiv(146097 * b, 4);

    int    d = floordiv(4 * c + 3, 1461);
    int    e = c - floordiv(1461 * d, 4);
    int    m = floordiv(5 * e + 2, 153);

    int    day = e - floordiv(153 * m + 2, 5) + 1;
    int    month = m + 3 - 12 * floordiv(m, 10);
    int    year = 100 * b + d - 4800 + floordiv(m, 10);

    // Adjust for no year 0
    if (year <= 0)
        --year ;

    if (yearp)
        *yearp = year;
    if (monthp)
        *monthp = month;
    if (dayp)
        *dayp = day;
}

/*****************************************************************************
  Date/Time formatting helper functions
 *****************************************************************************/

static const char monthDays[] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

#ifndef QT_NO_TEXTDATE
static const char * const qt_shortMonthNames[] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

int qt_monthNumberFromShortName(const QString &shortName)
{
    for (unsigned int i = 0; i < sizeof(qt_shortMonthNames) / sizeof(qt_shortMonthNames[0]); ++i) {
        if (shortName == QLatin1String(qt_shortMonthNames[i]))
            return i + 1;
    }
    return -1;
}
#endif

#ifndef QT_NO_DATESTRING
static void rfcDateImpl(const QString &s, QDate *dd = 0, QTime *dt = 0, int *utcOffset = 0)
{
    int day = -1;
    int month = -1;
    int year = -1;
    int hour = -1;
    int min = -1;
    int sec = -1;
    int hourOffset = 0;
    int minOffset = 0;
    bool positiveOffset = false;

    // Matches "Wdy, DD Mon YYYY HH:MM:SS ±hhmm" (Wdy, being optional)
    QRegExp rex(QStringLiteral("^(?:[A-Z][a-z]+,)?[ \\t]*(\\d{1,2})[ \\t]+([A-Z][a-z]+)[ \\t]+(\\d\\d\\d\\d)(?:[ \\t]+(\\d\\d):(\\d\\d)(?::(\\d\\d))?)?[ \\t]*(?:([+-])(\\d\\d)(\\d\\d))?"));
    if (s.indexOf(rex) == 0) {
        if (dd) {
            day = rex.cap(1).toInt();
            month = qt_monthNumberFromShortName(rex.cap(2));
            year = rex.cap(3).toInt();
        }
        if (dt) {
            if (!rex.cap(4).isEmpty()) {
                hour = rex.cap(4).toInt();
                min = rex.cap(5).toInt();
                sec = rex.cap(6).toInt();
            }
            positiveOffset = (rex.cap(7) == QStringLiteral("+"));
            hourOffset = rex.cap(8).toInt();
            minOffset = rex.cap(9).toInt();
        }
        if (utcOffset)
            *utcOffset = ((hourOffset * 60 + minOffset) * (positiveOffset ? 60 : -60));
    } else {
        // Matches "Wdy Mon DD HH:MM:SS YYYY"
        QRegExp rex(QStringLiteral("^[A-Z][a-z]+[ \\t]+([A-Z][a-z]+)[ \\t]+(\\d\\d)(?:[ \\t]+(\\d\\d):(\\d\\d):(\\d\\d))?[ \\t]+(\\d\\d\\d\\d)[ \\t]*(?:([+-])(\\d\\d)(\\d\\d))?"));
        if (s.indexOf(rex) == 0) {
            if (dd) {
                month = qt_monthNumberFromShortName(rex.cap(1));
                day = rex.cap(2).toInt();
                year = rex.cap(6).toInt();
            }
            if (dt) {
                if (!rex.cap(3).isEmpty()) {
                    hour = rex.cap(3).toInt();
                    min = rex.cap(4).toInt();
                    sec = rex.cap(5).toInt();
                }
                positiveOffset = (rex.cap(7) == QStringLiteral("+"));
                hourOffset = rex.cap(8).toInt();
                minOffset = rex.cap(9).toInt();
            }
            if (utcOffset)
                *utcOffset = ((hourOffset * 60 + minOffset) * (positiveOffset ? 60 : -60));
        }
    }

    if (dd)
        *dd = QDate(year, month, day);
    if (dt)
        *dt = QTime(hour, min, sec);
}
#endif // QT_NO_DATESTRING

// Return offset in [+-]HH:MM format
// Qt::ISODate puts : between the hours and minutes, but Qt:TextDate does not
static QString toOffsetString(Qt::DateFormat format, int offset)
{
    QString result;
    if (format == Qt::TextDate)
        result = QStringLiteral("%1%2%3");
    else // Qt::ISODate
        result = QStringLiteral("%1%2:%3");

    return result.arg(offset >= 0 ? QLatin1Char('+') : QLatin1Char('-'))
                 .arg(qAbs(offset) / SECS_PER_HOUR, 2, 10, QLatin1Char('0'))
                 .arg((offset / 60) % 60, 2, 10, QLatin1Char('0'));
}

// Parse offset in [+-]HH[:]MM format
static int fromOffsetString(const QString &offsetString, bool *valid)
{
    *valid = false;

    const int size = offsetString.size();
    if (size < 2 || size > 6)
        return 0;

    // First char must be + or -
    const QChar sign = offsetString.at(0);
    if (sign != QLatin1Char('+') && sign != QLatin1Char('-'))
        return 0;

    // Split the hour and minute parts
    QStringList parts = offsetString.split(QLatin1Char(':'));
    if (parts.count() == 1) {
        // [+-]HHMM format
        parts.append(parts.at(0).mid(3));
        parts[0] = parts.at(0).left(3);
    }

    bool ok = false;
    const int hour = parts.at(0).toInt(&ok);
    if (!ok)
        return 0;

    const int minute = parts.at(1).toInt(&ok);
    if (!ok || minute < 0 || minute > 59)
        return 0;

    *valid = true;
    return ((hour * 60) + minute) * 60;
}

/*****************************************************************************
  QDate member functions
 *****************************************************************************/

/*!
    \since 4.5

    \enum QDate::MonthNameType

    This enum describes the types of the string representation used
    for the month name.

    \value DateFormat This type of name can be used for date-to-string formatting.
    \value StandaloneFormat This type is used when you need to enumerate months or weekdays.
           Usually standalone names are represented in singular forms with
           capitalized first letter.
*/

/*!
    \class QDate
    \inmodule QtCore
    \reentrant
    \brief The QDate class provides date functions.


    A QDate object contains a calendar date, i.e. year, month, and day
    numbers, in the Gregorian calendar. It can read the current date
    from the system clock. It provides functions for comparing dates,
    and for manipulating dates. For example, it is possible to add
    and subtract days, months, and years to dates.

    A QDate object is typically created by giving the year,
    month, and day numbers explicitly. Note that QDate interprets two
    digit years as is, i.e., years 0 - 99. A QDate can also be
    constructed with the static function currentDate(), which creates
    a QDate object containing the system clock's date.  An explicit
    date can also be set using setDate(). The fromString() function
    returns a QDate given a string and a date format which is used to
    interpret the date within the string.

    The year(), month(), and day() functions provide access to the
    year, month, and day numbers. Also, dayOfWeek() and dayOfYear()
    functions are provided. The same information is provided in
    textual format by the toString(), shortDayName(), longDayName(),
    shortMonthName(), and longMonthName() functions.

    QDate provides a full set of operators to compare two QDate
    objects where smaller means earlier, and larger means later.

    You can increment (or decrement) a date by a given number of days
    using addDays(). Similarly you can use addMonths() and addYears().
    The daysTo() function returns the number of days between two
    dates.

    The daysInMonth() and daysInYear() functions return how many days
    there are in this date's month and year, respectively. The
    isLeapYear() function indicates whether a date is in a leap year.

    \section1

    \section2 No Year 0

    There is no year 0. Dates in that year are considered invalid. The
    year -1 is the year "1 before Christ" or "1 before current era."
    The day before 1 January 1 CE is 31 December 1 BCE.

    \section2 Range of Valid Dates

    Dates are stored internally as a Julian Day number, an integer count of
    every day in a contiguous range, with 24 November 4714 BCE in the Gregorian
    calendar being Julian Day 0 (1 January 4713 BCE in the Julian calendar).
    As well as being an efficient and accurate way of storing an absolute date,
    it is suitable for converting a Date into other calendar systems such as
    Hebrew, Islamic or Chinese. The Julian Day number can be obtained using
    QDate::toJulianDay() and can be set using QDate::fromJulianDay().

    The range of dates able to be stored by QDate as a Julian Day number is
    for technical reasons limited to between -784350574879 and 784354017364,
    which means from before 2 billion BCE to after 2 billion CE.

    \sa QTime, QDateTime, QDateEdit, QDateTimeEdit, QCalendarWidget
*/

/*!
    \fn QDate::QDate()

    Constructs a null date. Null dates are invalid.

    \sa isNull(), isValid()
*/

/*!
    Constructs a date with year \a y, month \a m and day \a d.

    If the specified date is invalid, the date is not set and
    isValid() returns \c false.

    \warning Years 1 to 99 are interpreted as is. Year 0 is invalid.

    \sa isValid()
*/

QDate::QDate(int y, int m, int d)
{
    setDate(y, m, d);
}


/*!
    \fn bool QDate::isNull() const

    Returns \c true if the date is null; otherwise returns \c false. A null
    date is invalid.

    \note The behavior of this function is equivalent to isValid().

    \sa isValid()
*/


/*!
    \fn bool QDate::isValid() const

    Returns \c true if this date is valid; otherwise returns \c false.

    \sa isNull()
*/


/*!
    Returns the year of this date. Negative numbers indicate years
    before 1 CE, such that year -44 is 44 BCE.

    Returns 0 if the date is invalid.

    \sa month(), day()
*/

int QDate::year() const
{
    if (isNull())
        return 0;

    int y;
    getDateFromJulianDay(jd, &y, 0, 0);
    return y;
}

/*!
    Returns the number corresponding to the month of this date, using
    the following convention:

    \list
    \li 1 = "January"
    \li 2 = "February"
    \li 3 = "March"
    \li 4 = "April"
    \li 5 = "May"
    \li 6 = "June"
    \li 7 = "July"
    \li 8 = "August"
    \li 9 = "September"
    \li 10 = "October"
    \li 11 = "November"
    \li 12 = "December"
    \endlist

    Returns 0 if the date is invalid.

    \sa year(), day()
*/

int QDate::month() const
{
    if (isNull())
        return 0;

    int m;
    getDateFromJulianDay(jd, 0, &m, 0);
    return m;
}

/*!
    Returns the day of the month (1 to 31) of this date.

    Returns 0 if the date is invalid.

    \sa year(), month(), dayOfWeek()
*/

int QDate::day() const
{
    if (isNull())
        return 0;

    int d;
    getDateFromJulianDay(jd, 0, 0, &d);
    return d;
}

/*!
    Returns the weekday (1 = Monday to 7 = Sunday) for this date.

    Returns 0 if the date is invalid.

    \sa day(), dayOfYear(), Qt::DayOfWeek
*/

int QDate::dayOfWeek() const
{
    if (isNull())
        return 0;

    if (jd >= 0)
        return (jd % 7) + 1;
    else
        return ((jd + 1) % 7) + 7;
}

/*!
    Returns the day of the year (1 to 365 or 366 on leap years) for
    this date.

    Returns 0 if the date is invalid.

    \sa day(), dayOfWeek()
*/

int QDate::dayOfYear() const
{
    if (isNull())
        return 0;

    return jd - julianDayFromDate(year(), 1, 1) + 1;
}

/*!
    Returns the number of days in the month (28 to 31) for this date.

    Returns 0 if the date is invalid.

    \sa day(), daysInYear()
*/

int QDate::daysInMonth() const
{
    if (isNull())
        return 0;

    int y, m;
    getDateFromJulianDay(jd, &y, &m, 0);
    if (m == 2 && isLeapYear(y))
        return 29;
    else
        return monthDays[m];
}

/*!
    Returns the number of days in the year (365 or 366) for this date.

    Returns 0 if the date is invalid.

    \sa day(), daysInMonth()
*/

int QDate::daysInYear() const
{
    if (isNull())
        return 0;

    int y;
    getDateFromJulianDay(jd, &y, 0, 0);
    return isLeapYear(y) ? 366 : 365;
}

/*!
    Returns the week number (1 to 53), and stores the year in
    *\a{yearNumber} unless \a yearNumber is null (the default).

    Returns 0 if the date is invalid.

    In accordance with ISO 8601, weeks start on Monday and the first
    Thursday of a year is always in week 1 of that year. Most years
    have 52 weeks, but some have 53.

    *\a{yearNumber} is not always the same as year(). For example, 1
    January 2000 has week number 52 in the year 1999, and 31 December
    2002 has week number 1 in the year 2003.

    \legalese
    Copyright (c) 1989 The Regents of the University of California.
    All rights reserved.

    Redistribution and use in source and binary forms are permitted
    provided that the above copyright notice and this paragraph are
    duplicated in all such forms and that any documentation,
    advertising materials, and other materials related to such
    distribution and use acknowledge that the software was developed
    by the University of California, Berkeley.  The name of the
    University may not be used to endorse or promote products derived
    from this software without specific prior written permission.
    THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.

    \sa isValid()
*/

int QDate::weekNumber(int *yearNumber) const
{
    if (!isValid())
        return 0;

    int year = QDate::year();
    int yday = dayOfYear() - 1;
    int wday = dayOfWeek();
    if (wday == 7)
        wday = 0;
    int w;

    for (;;) {
        int len;
        int bot;
        int top;

        len = isLeapYear(year) ? 366 : 365;
        /*
        ** What yday (-3 ... 3) does
        ** the ISO year begin on?
        */
        bot = ((yday + 11 - wday) % 7) - 3;
        /*
        ** What yday does the NEXT
        ** ISO year begin on?
        */
        top = bot - (len % 7);
        if (top < -3)
            top += 7;
        top += len;
        if (yday >= top) {
            ++year;
            w = 1;
            break;
        }
        if (yday >= bot) {
            w = 1 + ((yday - bot) / 7);
            break;
        }
        --year;
        yday += isLeapYear(year) ? 366 : 365;
    }
    if (yearNumber != 0)
        *yearNumber = year;
    return w;
}

#ifndef QT_NO_TEXTDATE
/*!
    \since 4.5

    Returns the short name of the \a month for the representation specified
    by \a type.

    The months are enumerated using the following convention:

    \list
    \li 1 = "Jan"
    \li 2 = "Feb"
    \li 3 = "Mar"
    \li 4 = "Apr"
    \li 5 = "May"
    \li 6 = "Jun"
    \li 7 = "Jul"
    \li 8 = "Aug"
    \li 9 = "Sep"
    \li 10 = "Oct"
    \li 11 = "Nov"
    \li 12 = "Dec"
    \endlist

    The month names will be localized according to the system's
    locale settings, i.e. using QLocale::system().

    Returns an empty string if the date is invalid.

    \sa toString(), longMonthName(), shortDayName(), longDayName()
*/

QString QDate::shortMonthName(int month, QDate::MonthNameType type)
{
    if (month < 1 || month > 12)
        return QString();

    switch (type) {
    case QDate::DateFormat:
        return QLocale::system().monthName(month, QLocale::ShortFormat);
    case QDate::StandaloneFormat:
        return QLocale::system().standaloneMonthName(month, QLocale::ShortFormat);
    default:
        break;
    }
    return QString();
}

/*!
    \since 4.5

    Returns the long name of the \a month for the representation specified
    by \a type.

    The months are enumerated using the following convention:

    \list
    \li 1 = "January"
    \li 2 = "February"
    \li 3 = "March"
    \li 4 = "April"
    \li 5 = "May"
    \li 6 = "June"
    \li 7 = "July"
    \li 8 = "August"
    \li 9 = "September"
    \li 10 = "October"
    \li 11 = "November"
    \li 12 = "December"
    \endlist

    The month names will be localized according to the system's
    locale settings, i.e. using QLocale::system().

    Returns an empty string if the date is invalid.

    \sa toString(), shortMonthName(), shortDayName(), longDayName()
*/

QString QDate::longMonthName(int month, MonthNameType type)
{
    if (month < 1 || month > 12)
        return QString();

    switch (type) {
    case QDate::DateFormat:
        return QLocale::system().monthName(month, QLocale::LongFormat);
    case QDate::StandaloneFormat:
        return QLocale::system().standaloneMonthName(month, QLocale::LongFormat);
    default:
        break;
    }
    return QString();
}

/*!
    \since 4.5

    Returns the short name of the \a weekday for the representation specified
    by \a type.

    The days are enumerated using the following convention:

    \list
    \li 1 = "Mon"
    \li 2 = "Tue"
    \li 3 = "Wed"
    \li 4 = "Thu"
    \li 5 = "Fri"
    \li 6 = "Sat"
    \li 7 = "Sun"
    \endlist

    The day names will be localized according to the system's
    locale settings, i.e. using QLocale::system().

    Returns an empty string if the date is invalid.

    \sa toString(), shortMonthName(), longMonthName(), longDayName()
*/

QString QDate::shortDayName(int weekday, MonthNameType type)
{
    if (weekday < 1 || weekday > 7)
        return QString();

    switch (type) {
    case QDate::DateFormat:
        return QLocale::system().dayName(weekday, QLocale::ShortFormat);
    case QDate::StandaloneFormat:
        return QLocale::system().standaloneDayName(weekday, QLocale::ShortFormat);
    default:
        break;
    }
    return QString();
}

/*!
    \since 4.5

    Returns the long name of the \a weekday for the representation specified
    by \a type.

    The days are enumerated using the following convention:

    \list
    \li 1 = "Monday"
    \li 2 = "Tuesday"
    \li 3 = "Wednesday"
    \li 4 = "Thursday"
    \li 5 = "Friday"
    \li 6 = "Saturday"
    \li 7 = "Sunday"
    \endlist

    The day names will be localized according to the system's
    locale settings, i.e. using QLocale::system().

    Returns an empty string if the date is invalid.

    \sa toString(), shortDayName(), shortMonthName(), longMonthName()
*/

QString QDate::longDayName(int weekday, MonthNameType type)
{
    if (weekday < 1 || weekday > 7)
        return QString();

    switch (type) {
    case QDate::DateFormat:
        return QLocale::system().dayName(weekday, QLocale::LongFormat);
    case QDate::StandaloneFormat:
        return QLocale::system().standaloneDayName(weekday, QLocale::LongFormat);
    default:
        break;
    }
    return QLocale::system().dayName(weekday, QLocale::LongFormat);
}
#endif //QT_NO_TEXTDATE

#ifndef QT_NO_DATESTRING

/*!
    \fn QString QDate::toString(Qt::DateFormat format) const

    \overload

    Returns the date as a string. The \a format parameter determines
    the format of the string.

    If the \a format is Qt::TextDate, the string is formatted in
    the default way. QDate::shortDayName() and QDate::shortMonthName()
    are used to generate the string, so the day and month names will
    be localized names using the system locale, i.e. QLocale::system(). An
    example of this formatting is "Sat May 20 1995".

    If the \a format is Qt::ISODate, the string format corresponds
    to the ISO 8601 extended specification for representations of
    dates and times, taking the form YYYY-MM-DD, where YYYY is the
    year, MM is the month of the year (between 01 and 12), and DD is
    the day of the month between 01 and 31.

    If the \a format is Qt::SystemLocaleShortDate or
    Qt::SystemLocaleLongDate, the string format depends on the locale
    settings of the system. Identical to calling
    QLocale::system().toString(date, QLocale::ShortFormat) or
    QLocale::system().toString(date, QLocale::LongFormat).

    If the \a format is Qt::DefaultLocaleShortDate or
    Qt::DefaultLocaleLongDate, the string format depends on the
    default application locale. This is the locale set with
    QLocale::setDefault(), or the system locale if no default locale
    has been set. Identical to calling QLocale().toString(date,
    QLocale::ShortFormat) or QLocale().toString(date,
    QLocale::LongFormat).

    If the \a format is Qt::RFC2822Date, the string is formatted in
    an \l{RFC 2822} compatible way. An example of this formatting is
    "20 May 1995".

    If the date is invalid, an empty string will be returned.

    \warning The Qt::ISODate format is only valid for years in the
    range 0 to 9999. This restriction may apply to locale-aware
    formats as well, depending on the locale settings.

    \sa shortDayName(), shortMonthName()
*/
QString QDate::toString(Qt::DateFormat format) const
{
    if (!isValid())
        return QString();

    int y, m, d;

    switch (format) {
    case Qt::SystemLocaleDate:
    case Qt::SystemLocaleShortDate:
        return QLocale::system().toString(*this, QLocale::ShortFormat);
    case Qt::SystemLocaleLongDate:
        return QLocale::system().toString(*this, QLocale::LongFormat);
    case Qt::LocaleDate:
    case Qt::DefaultLocaleShortDate:
        return QLocale().toString(*this, QLocale::ShortFormat);
    case Qt::DefaultLocaleLongDate:
        return QLocale().toString(*this, QLocale::LongFormat);
    case Qt::RFC2822Date:
        return QLocale::c().toString(*this, QStringLiteral("dd MMM yyyy"));
    default:
#ifndef QT_NO_TEXTDATE
    case Qt::TextDate:
        getDateFromJulianDay(jd, &y, &m, &d);
        return QString::fromUtf8("%1 %2 %3 %4").arg(shortDayName(dayOfWeek()))
                                               .arg(shortMonthName(m))
                                               .arg(d)
                                               .arg(y);
#endif
    case Qt::ISODate:
        getDateFromJulianDay(jd, &y, &m, &d);
        if (y < 0 || y > 9999)
            return QString();
        return QString::fromUtf8("%1-%2-%3").arg(y, 4, 10, QLatin1Char('0'))
                                            .arg(m, 2, 10, QLatin1Char('0'))
                                            .arg(d, 2, 10, QLatin1Char('0'));
    }
}

/*!
    Returns the date as a string. The \a format parameter determines
    the format of the result string.

    These expressions may be used:

    \table
    \header \li Expression \li Output
    \row \li d \li the day as number without a leading zero (1 to 31)
    \row \li dd \li the day as number with a leading zero (01 to 31)
    \row \li ddd
         \li the abbreviated localized day name (e.g. 'Mon' to 'Sun').
             Uses the system locale to localize the name, i.e. QLocale::system().
    \row \li dddd
         \li the long localized day name (e.g. 'Monday' to 'Sunday').
             Uses the system locale to localize the name, i.e. QLocale::system().
    \row \li M \li the month as number without a leading zero (1 to 12)
    \row \li MM \li the month as number with a leading zero (01 to 12)
    \row \li MMM
         \li the abbreviated localized month name (e.g. 'Jan' to 'Dec').
             Uses the system locale to localize the name, i.e. QLocale::system().
    \row \li MMMM
         \li the long localized month name (e.g. 'January' to 'December').
             Uses the system locale to localize the name, i.e. QLocale::system().
    \row \li yy \li the year as two digit number (00 to 99)
    \row \li yyyy \li the year as four digit number. If the year is negative,
            a minus sign is prepended in addition.
    \endtable

    All other input characters will be ignored. Any sequence of characters that
    are enclosed in single quotes will be treated as text and not be used as an
    expression. Two consecutive single quotes ("''") are replaced by a singlequote
    in the output. Formats without separators (e.g. "ddMM") are currently not supported.

    Example format strings (assuming that the QDate is the 20 July
    1969):

    \table
    \header \li Format            \li Result
    \row    \li dd.MM.yyyy        \li 20.07.1969
    \row    \li ddd MMMM d yy     \li Sun July 20 69
    \row    \li 'The day is' dddd \li The day is Sunday
    \endtable

    If the datetime is invalid, an empty string will be returned.

    \sa QDateTime::toString(), QTime::toString(), QLocale::toString()

*/
QString QDate::toString(const QString& format) const
{
    return QLocale::system().toString(*this, format);
}
#endif //QT_NO_DATESTRING

/*!
    \fn bool QDate::setYMD(int y, int m, int d)

    \deprecated in 5.0, use setDate() instead.

    Sets the date's year \a y, month \a m, and day \a d.

    If \a y is in the range 0 to 99, it is interpreted as 1900 to
    1999.
    Returns \c false if the date is invalid.

    Use setDate() instead.
*/

/*!
    \since 4.2

    Sets the date's \a year, \a month, and \a day. Returns \c true if
    the date is valid; otherwise returns \c false.

    If the specified date is invalid, the QDate object is set to be
    invalid.

    \sa isValid()
*/
bool QDate::setDate(int year, int month, int day)
{
    if (isValid(year, month, day))
        jd = julianDayFromDate(year, month, day);
    else
        jd = nullJd();

    return isValid();
}

/*!
    \since 4.5

    Extracts the date's year, month, and day, and assigns them to
    *\a year, *\a month, and *\a day. The pointers may be null.

    Returns 0 if the date is invalid.

    \sa year(), month(), day(), isValid()
*/
void QDate::getDate(int *year, int *month, int *day)
{
    if (isValid()) {
        getDateFromJulianDay(jd, year, month, day);
    } else {
        if (year)
            *year = 0;
        if (month)
            *month = 0;
        if (day)
            *day = 0;
    }
}

/*!
    Returns a QDate object containing a date \a ndays later than the
    date of this object (or earlier if \a ndays is negative).

    Returns a null date if the current date is invalid or the new date is
    out of range.

    \sa addMonths(), addYears(), daysTo()
*/

QDate QDate::addDays(qint64 ndays) const
{
    if (isNull())
        return QDate();

    // Due to limits on minJd() and maxJd() we know that any overflow
    // will be invalid and caught by fromJulianDay().
    return fromJulianDay(jd + ndays);
}

/*!
    Returns a QDate object containing a date \a nmonths later than the
    date of this object (or earlier if \a nmonths is negative).

    \note If the ending day/month combination does not exist in the
    resulting month/year, this function will return a date that is the
    latest valid date.

    \sa addDays(), addYears()
*/

QDate QDate::addMonths(int nmonths) const
{
    if (!isValid())
        return QDate();
    if (!nmonths)
        return *this;

    int old_y, y, m, d;
    getDateFromJulianDay(jd, &y, &m, &d);
    old_y = y;

    bool increasing = nmonths > 0;

    while (nmonths != 0) {
        if (nmonths < 0 && nmonths + 12 <= 0) {
            y--;
            nmonths+=12;
        } else if (nmonths < 0) {
            m+= nmonths;
            nmonths = 0;
            if (m <= 0) {
                --y;
                m += 12;
            }
        } else if (nmonths - 12 >= 0) {
            y++;
            nmonths -= 12;
        } else if (m == 12) {
            y++;
            m = 0;
        } else {
            m += nmonths;
            nmonths = 0;
            if (m > 12) {
                ++y;
                m -= 12;
            }
        }
    }

    // was there a sign change?
    if ((old_y > 0 && y <= 0) ||
        (old_y < 0 && y >= 0))
        // yes, adjust the date by +1 or -1 years
        y += increasing ? +1 : -1;

    return fixedDate(y, m, d);
}

/*!
    Returns a QDate object containing a date \a nyears later than the
    date of this object (or earlier if \a nyears is negative).

    \note If the ending day/month combination does not exist in the
    resulting year (i.e., if the date was Feb 29 and the final year is
    not a leap year), this function will return a date that is the
    latest valid date (that is, Feb 28).

    \sa addDays(), addMonths()
*/

QDate QDate::addYears(int nyears) const
{
    if (!isValid())
        return QDate();

    int y, m, d;
    getDateFromJulianDay(jd, &y, &m, &d);

    int old_y = y;
    y += nyears;

    // was there a sign change?
    if ((old_y > 0 && y <= 0) ||
        (old_y < 0 && y >= 0))
        // yes, adjust the date by +1 or -1 years
        y += nyears > 0 ? +1 : -1;

    return fixedDate(y, m, d);
}

/*!
    Returns the number of days from this date to \a d (which is
    negative if \a d is earlier than this date).

    Returns 0 if either date is invalid.

    Example:
    \snippet code/src_corelib_tools_qdatetime.cpp 0

    \sa addDays()
*/

qint64 QDate::daysTo(const QDate &d) const
{
    if (isNull() || d.isNull())
        return 0;

    // Due to limits on minJd() and maxJd() we know this will never overflow
    return d.jd - jd;
}


/*!
    \fn bool QDate::operator==(const QDate &d) const

    Returns \c true if this date is equal to \a d; otherwise returns
    false.

*/

/*!
    \fn bool QDate::operator!=(const QDate &d) const

    Returns \c true if this date is different from \a d; otherwise
    returns \c false.
*/

/*!
    \fn bool QDate::operator<(const QDate &d) const

    Returns \c true if this date is earlier than \a d; otherwise returns
    false.
*/

/*!
    \fn bool QDate::operator<=(const QDate &d) const

    Returns \c true if this date is earlier than or equal to \a d;
    otherwise returns \c false.
*/

/*!
    \fn bool QDate::operator>(const QDate &d) const

    Returns \c true if this date is later than \a d; otherwise returns
    false.
*/

/*!
    \fn bool QDate::operator>=(const QDate &d) const

    Returns \c true if this date is later than or equal to \a d;
    otherwise returns \c false.
*/

/*!
    \fn QDate::currentDate()
    Returns the current date, as reported by the system clock.

    \sa QTime::currentTime(), QDateTime::currentDateTime()
*/

#ifndef QT_NO_DATESTRING
/*!
    \fn QDate QDate::fromString(const QString &string, Qt::DateFormat format)

    Returns the QDate represented by the \a string, using the
    \a format given, or an invalid date if the string cannot be
    parsed.

    Note for Qt::TextDate: It is recommended that you use the
    English short month names (e.g. "Jan"). Although localized month
    names can also be used, they depend on the user's locale settings.
*/
QDate QDate::fromString(const QString& string, Qt::DateFormat format)
{
    if (string.isEmpty())
        return QDate();

    switch (format) {
    case Qt::SystemLocaleDate:
    case Qt::SystemLocaleShortDate:
        return QLocale::system().toDate(string, QLocale::ShortFormat);
    case Qt::SystemLocaleLongDate:
        return QLocale::system().toDate(string, QLocale::LongFormat);
    case Qt::LocaleDate:
    case Qt::DefaultLocaleShortDate:
        return QLocale().toDate(string, QLocale::ShortFormat);
    case Qt::DefaultLocaleLongDate:
        return QLocale().toDate(string, QLocale::LongFormat);
    case Qt::RFC2822Date: {
        QDate date;
        rfcDateImpl(string, &date);
        return date;
    }
    default:
#ifndef QT_NO_TEXTDATE
    case Qt::TextDate: {
        QStringList parts = string.split(QLatin1Char(' '), QString::SkipEmptyParts);

        if (parts.count() != 4)
            return QDate();

        QString monthName = parts.at(1);
        int month = -1;
        // Assume that English monthnames are the default
        for (int i = 0; i < 12; ++i) {
            if (monthName == QLatin1String(qt_shortMonthNames[i])) {
                month = i + 1;
                break;
            }
        }
        // If English names can't be found, search the localized ones
        if (month == -1) {
            for (int i = 1; i <= 12; ++i) {
                if (monthName == QDate::shortMonthName(i)) {
                    month = i;
                    break;
                }
            }
            if (month == -1)
                // Month name matches neither English nor other localised name.
                return QDate();
        }

        bool ok = false;
        int year = parts.at(3).toInt(&ok);
        if (!ok)
            return QDate();

        return QDate(year, month, parts.at(2).toInt());
        }
#endif // QT_NO_TEXTDATE
    case Qt::ISODate: {
        // Semi-strict parsing, must be long enough and have non-numeric separators
        if (string.size() < 10 || string.at(4).isDigit() || string.at(7).isDigit()
            || (string.size() > 10 && string.at(10).isDigit())) {
            return QDate();
        }
        const int year = string.mid(0, 4).toInt();
        if (year <= 0 || year > 9999)
            return QDate();
        return QDate(year, string.mid(5, 2).toInt(), string.mid(8, 2).toInt());
        }
    }
    return QDate();
}

/*!
    \fn QDate::fromString(const QString &string, const QString &format)

    Returns the QDate represented by the \a string, using the \a
    format given, or an invalid date if the string cannot be parsed.

    These expressions may be used for the format:

    \table
    \header \li Expression \li Output
    \row \li d \li The day as a number without a leading zero (1 to 31)
    \row \li dd \li The day as a number with a leading zero (01 to 31)
    \row \li ddd
         \li The abbreviated localized day name (e.g. 'Mon' to 'Sun').
             Uses the system locale to localize the name, i.e. QLocale::system().
    \row \li dddd
         \li The long localized day name (e.g. 'Monday' to 'Sunday').
             Uses the system locale to localize the name, i.e. QLocale::system().
    \row \li M \li The month as a number without a leading zero (1 to 12)
    \row \li MM \li The month as a number with a leading zero (01 to 12)
    \row \li MMM
         \li The abbreviated localized month name (e.g. 'Jan' to 'Dec').
             Uses the system locale to localize the name, i.e. QLocale::system().
    \row \li MMMM
         \li The long localized month name (e.g. 'January' to 'December').
             Uses the system locale to localize the name, i.e. QLocale::system().
    \row \li yy \li The year as two digit number (00 to 99)
    \row \li yyyy \li The year as four digit number. If the year is negative,
            a minus sign is prepended in addition.
    \endtable

    All other input characters will be treated as text. Any sequence
    of characters that are enclosed in single quotes will also be
    treated as text and will not be used as an expression. For example:

    \snippet code/src_corelib_tools_qdatetime.cpp 1

    If the format is not satisfied, an invalid QDate is returned. The
    expressions that don't expect leading zeroes (d, M) will be
    greedy. This means that they will use two digits even if this
    will put them outside the accepted range of values and leaves too
    few digits for other sections. For example, the following format
    string could have meant January 30 but the M will grab two
    digits, resulting in an invalid date:

    \snippet code/src_corelib_tools_qdatetime.cpp 2

    For any field that is not represented in the format the following
    defaults are used:

    \table
    \header \li Field  \li Default value
    \row    \li Year   \li 1900
    \row    \li Month  \li 1
    \row    \li Day    \li 1
    \endtable

    The following examples demonstrate the default values:

    \snippet code/src_corelib_tools_qdatetime.cpp 3

    \sa QDateTime::fromString(), QTime::fromString(), QDate::toString(),
        QDateTime::toString(), QTime::toString()
*/

QDate QDate::fromString(const QString &string, const QString &format)
{
    QDate date;
#ifndef QT_BOOTSTRAPPED
    QDateTimeParser dt(QVariant::Date, QDateTimeParser::FromString);
    if (dt.parseFormat(format))
        dt.fromString(string, &date, 0);
#else
    Q_UNUSED(string);
    Q_UNUSED(format);
#endif
    return date;
}
#endif // QT_NO_DATESTRING

/*!
    \overload

    Returns \c true if the specified date (\a year, \a month, and \a
    day) is valid; otherwise returns \c false.

    Example:
    \snippet code/src_corelib_tools_qdatetime.cpp 4

    \sa isNull(), setDate()
*/

bool QDate::isValid(int year, int month, int day)
{
    // there is no year 0 in the Gregorian calendar
    if (year == 0)
        return false;

    return (day > 0 && month > 0 && month <= 12) &&
           (day <= monthDays[month] || (day == 29 && month == 2 && isLeapYear(year)));
}

/*!
    \fn bool QDate::isLeapYear(int year)

    Returns \c true if the specified \a year is a leap year; otherwise
    returns \c false.
*/

bool QDate::isLeapYear(int y)
{
    // No year 0 in Gregorian calendar, so -1, -5, -9 etc are leap years
    if ( y < 1)
        ++y;

    return (y % 4 == 0 && y % 100 != 0) || y % 400 == 0;
}

/*! \fn static QDate QDate::fromJulianDay(qint64 jd)

    Converts the Julian day \a jd to a QDate.

    \sa toJulianDay()
*/

/*! \fn int QDate::toJulianDay() const

    Converts the date to a Julian day.

    \sa fromJulianDay()
*/

/*****************************************************************************
  QTime member functions
 *****************************************************************************/

/*!
    \class QTime
    \inmodule QtCore
    \reentrant

    \brief The QTime class provides clock time functions.


    A QTime object contains a clock time, i.e. the number of hours,
    minutes, seconds, and milliseconds since midnight. It can read the
    current time from the system clock and measure a span of elapsed
    time. It provides functions for comparing times and for
    manipulating a time by adding a number of milliseconds.

    QTime uses the 24-hour clock format; it has no concept of AM/PM.
    Unlike QDateTime, QTime knows nothing about time zones or
    daylight savings time (DST).

    A QTime object is typically created either by giving the number
    of hours, minutes, seconds, and milliseconds explicitly, or by
    using the static function currentTime(), which creates a QTime
    object that contains the system's local time. Note that the
    accuracy depends on the accuracy of the underlying operating
    system; not all systems provide 1-millisecond accuracy.

    The hour(), minute(), second(), and msec() functions provide
    access to the number of hours, minutes, seconds, and milliseconds
    of the time. The same information is provided in textual format by
    the toString() function.

    QTime provides a full set of operators to compare two QTime
    objects. QTime A is considered smaller than QTime B if A is
    earlier than B.

    The addSecs() and addMSecs() functions provide the time a given
    number of seconds or milliseconds later than a given time.
    Correspondingly, the number of seconds or milliseconds
    between two times can be found using secsTo() or msecsTo().

    QTime can be used to measure a span of elapsed time using the
    start(), restart(), and elapsed() functions.

    \sa QDate, QDateTime
*/

/*!
    \fn QTime::QTime()

    Constructs a null time object. A null time can be a QTime(0, 0, 0, 0)
    (i.e., midnight) object, except that isNull() returns \c true and isValid()
    returns \c false.

    \sa isNull(), isValid()
*/

/*!
    Constructs a time with hour \a h, minute \a m, seconds \a s and
    milliseconds \a ms.

    \a h must be in the range 0 to 23, \a m and \a s must be in the
    range 0 to 59, and \a ms must be in the range 0 to 999.

    \sa isValid()
*/

QTime::QTime(int h, int m, int s, int ms)
{
    setHMS(h, m, s, ms);
}


/*!
    \fn bool QTime::isNull() const

    Returns \c true if the time is null (i.e., the QTime object was
    constructed using the default constructor); otherwise returns
    false. A null time is also an invalid time.

    \sa isValid()
*/

/*!
    Returns \c true if the time is valid; otherwise returns \c false. For example,
    the time 23:30:55.746 is valid, but 24:12:30 is invalid.

    \sa isNull()
*/

bool QTime::isValid() const
{
    return mds > NullTime && mds < MSECS_PER_DAY;
}


/*!
    Returns the hour part (0 to 23) of the time.

    Returns -1 if the time is invalid.

    \sa minute(), second(), msec()
*/

int QTime::hour() const
{
    if (!isValid())
        return -1;

    return ds() / MSECS_PER_HOUR;
}

/*!
    Returns the minute part (0 to 59) of the time.

    Returns -1 if the time is invalid.

    \sa hour(), second(), msec()
*/

int QTime::minute() const
{
    if (!isValid())
        return -1;

    return (ds() % MSECS_PER_HOUR) / MSECS_PER_MIN;
}

/*!
    Returns the second part (0 to 59) of the time.

    Returns -1 if the time is invalid.

    \sa hour(), minute(), msec()
*/

int QTime::second() const
{
    if (!isValid())
        return -1;

    return (ds() / 1000)%SECS_PER_MIN;
}

/*!
    Returns the millisecond part (0 to 999) of the time.

    Returns -1 if the time is invalid.

    \sa hour(), minute(), second()
*/

int QTime::msec() const
{
    if (!isValid())
        return -1;

    return ds() % 1000;
}

#ifndef QT_NO_DATESTRING
/*!
    \overload

    Returns the time as a string. The \a format parameter determines
    the format of the string.

    If \a format is Qt::TextDate, the string format is HH:MM:SS;
    e.g. 1 second before midnight would be "23:59:59".

    If \a format is Qt::ISODate, the string format corresponds to the
    ISO 8601 extended specification for representations of dates,
    which is also HH:MM:SS.

    If the \a format is Qt::SystemLocaleShortDate or
    Qt::SystemLocaleLongDate, the string format depends on the locale
    settings of the system. Identical to calling
    QLocale::system().toString(time, QLocale::ShortFormat) or
    QLocale::system().toString(time, QLocale::LongFormat).

    If the \a format is Qt::DefaultLocaleShortDate or
    Qt::DefaultLocaleLongDate, the string format depends on the
    default application locale. This is the locale set with
    QLocale::setDefault(), or the system locale if no default locale
    has been set. Identical to calling QLocale().toString(time,
    QLocale::ShortFormat) or QLocale().toString(time,
    QLocale::LongFormat).

    If the \a format is Qt::RFC2822Date, the string is formatted in
    an \l{RFC 2822} compatible way. An example of this formatting is
    "23:59:20".

    If the time is invalid, an empty string will be returned.

    \sa QDate::toString(), QDateTime::toString()
*/

QString QTime::toString(Qt::DateFormat format) const
{
    if (!isValid())
        return QString();

    switch (format) {
    case Qt::SystemLocaleDate:
    case Qt::SystemLocaleShortDate:
        return QLocale::system().toString(*this, QLocale::ShortFormat);
    case Qt::SystemLocaleLongDate:
        return QLocale::system().toString(*this, QLocale::LongFormat);
    case Qt::LocaleDate:
    case Qt::DefaultLocaleShortDate:
        return QLocale().toString(*this, QLocale::ShortFormat);
    case Qt::DefaultLocaleLongDate:
        return QLocale().toString(*this, QLocale::LongFormat);
    case Qt::RFC2822Date:
    case Qt::ISODate:
    case Qt::TextDate:
    default:
        return QString::fromUtf8("%1:%2:%3").arg(hour(), 2, 10, QLatin1Char('0'))
                                            .arg(minute(), 2, 10, QLatin1Char('0'))
                                            .arg(second(), 2, 10, QLatin1Char('0'));
    }
}

/*!
    Returns the time as a string. The \a format parameter determines
    the format of the result string.

    These expressions may be used:

    \table
    \header \li Expression \li Output
    \row \li h
         \li the hour without a leading zero (0 to 23 or 1 to 12 if AM/PM display)
    \row \li hh
         \li the hour with a leading zero (00 to 23 or 01 to 12 if AM/PM display)
    \row \li H
         \li the hour without a leading zero (0 to 23, even with AM/PM display)
    \row \li HH
         \li the hour with a leading zero (00 to 23, even with AM/PM display)
    \row \li m \li the minute without a leading zero (0 to 59)
    \row \li mm \li the minute with a leading zero (00 to 59)
    \row \li s \li the second without a leading zero (0 to 59)
    \row \li ss \li the second with a leading zero (00 to 59)
    \row \li z \li the milliseconds without leading zeroes (0 to 999)
    \row \li zzz \li the milliseconds with leading zeroes (000 to 999)
    \row \li AP or A
         \li use AM/PM display. \e A/AP will be replaced by either "AM" or "PM".
    \row \li ap or a
         \li use am/pm display. \e a/ap will be replaced by either "am" or "pm".
    \row \li t \li the timezone (for example "CEST")
    \endtable

    All other input characters will be ignored. Any sequence of characters that
    are enclosed in single quotes will be treated as text and not be used as an
    expression. Two consecutive single quotes ("''") are replaced by a singlequote
    in the output. Formats without separators (e.g. "HHmm") are currently not supported.

    Example format strings (assuming that the QTime is 14:13:09.042)

    \table
    \header \li Format \li Result
    \row \li hh:mm:ss.zzz \li 14:13:09.042
    \row \li h:m:s ap     \li 2:13:9 pm
    \row \li H:m:s a      \li 14:13:9 pm
    \endtable

    If the time is invalid, an empty string will be returned.
    If \a format is empty, the default format "hh:mm:ss" is used.

    \sa QDate::toString(), QDateTime::toString(), QLocale::toString()
*/
QString QTime::toString(const QString& format) const
{
    return QLocale::system().toString(*this, format);
}
#endif //QT_NO_DATESTRING
/*!
    Sets the time to hour \a h, minute \a m, seconds \a s and
    milliseconds \a ms.

    \a h must be in the range 0 to 23, \a m and \a s must be in the
    range 0 to 59, and \a ms must be in the range 0 to 999.
    Returns \c true if the set time is valid; otherwise returns \c false.

    \sa isValid()
*/

bool QTime::setHMS(int h, int m, int s, int ms)
{
#if defined(Q_OS_WINCE)
    startTick = NullTime;
#endif
    if (!isValid(h,m,s,ms)) {
        mds = NullTime;                // make this invalid
        return false;
    }
    mds = (h*SECS_PER_HOUR + m*SECS_PER_MIN + s)*1000 + ms;
    return true;
}

/*!
    Returns a QTime object containing a time \a s seconds later
    than the time of this object (or earlier if \a s is negative).

    Note that the time will wrap if it passes midnight.

    Returns a null time if this time is invalid.

    Example:

    \snippet code/src_corelib_tools_qdatetime.cpp 5

    \sa addMSecs(), secsTo(), QDateTime::addSecs()
*/

QTime QTime::addSecs(int s) const
{
    return addMSecs(s * 1000);
}

/*!
    Returns the number of seconds from this time to \a t.
    If \a t is earlier than this time, the number of seconds returned
    is negative.

    Because QTime measures time within a day and there are 86400
    seconds in a day, the result is always between -86400 and 86400.

    secsTo() does not take into account any milliseconds.

    Returns 0 if either time is invalid.

    \sa addSecs(), QDateTime::secsTo()
*/

int QTime::secsTo(const QTime &t) const
{
    if (!isValid() || !t.isValid())
        return 0;

    // Truncate milliseconds as we do not want to consider them.
    int ourSeconds = ds() / 1000;
    int theirSeconds = t.ds() / 1000;
    return theirSeconds - ourSeconds;
}

/*!
    Returns a QTime object containing a time \a ms milliseconds later
    than the time of this object (or earlier if \a ms is negative).

    Note that the time will wrap if it passes midnight. See addSecs()
    for an example.

    Returns a null time if this time is invalid.

    \sa addSecs(), msecsTo(), QDateTime::addMSecs()
*/

QTime QTime::addMSecs(int ms) const
{
    QTime t;
    if (isValid()) {
        if (ms < 0) {
            // % not well-defined for -ve, but / is.
            int negdays = (MSECS_PER_DAY - ms) / MSECS_PER_DAY;
            t.mds = (ds() + ms + negdays * MSECS_PER_DAY) % MSECS_PER_DAY;
        } else {
            t.mds = (ds() + ms) % MSECS_PER_DAY;
        }
    }
#if defined(Q_OS_WINCE)
    if (startTick > NullTime)
        t.startTick = (startTick + ms) % MSECS_PER_DAY;
#endif
    return t;
}

/*!
    Returns the number of milliseconds from this time to \a t.
    If \a t is earlier than this time, the number of milliseconds returned
    is negative.

    Because QTime measures time within a day and there are 86400
    seconds in a day, the result is always between -86400000 and
    86400000 ms.

    Returns 0 if either time is invalid.

    \sa secsTo(), addMSecs(), QDateTime::msecsTo()
*/

int QTime::msecsTo(const QTime &t) const
{
    if (!isValid() || !t.isValid())
        return 0;
#if defined(Q_OS_WINCE)
    // GetLocalTime() for Windows CE has no milliseconds resolution
    if (t.startTick > NullTime && startTick > NullTime)
        return t.startTick - startTick;
    else
#endif
        return t.ds() - ds();
}


/*!
    \fn bool QTime::operator==(const QTime &t) const

    Returns \c true if this time is equal to \a t; otherwise returns \c false.
*/

/*!
    \fn bool QTime::operator!=(const QTime &t) const

    Returns \c true if this time is different from \a t; otherwise returns \c false.
*/

/*!
    \fn bool QTime::operator<(const QTime &t) const

    Returns \c true if this time is earlier than \a t; otherwise returns \c false.
*/

/*!
    \fn bool QTime::operator<=(const QTime &t) const

    Returns \c true if this time is earlier than or equal to \a t;
    otherwise returns \c false.
*/

/*!
    \fn bool QTime::operator>(const QTime &t) const

    Returns \c true if this time is later than \a t; otherwise returns \c false.
*/

/*!
    \fn bool QTime::operator>=(const QTime &t) const

    Returns \c true if this time is later than or equal to \a t;
    otherwise returns \c false.
*/

/*!
    \fn QTime QTime::fromMSecsSinceStartOfDay(int msecs)

    Returns a new QTime instance with the time set to the number of \a msecs
    since the start of the day, i.e. since 00:00:00.

    If \a msecs falls outside the valid range an invalid QTime will be returned.

    \sa msecsSinceStartOfDay()
*/

/*!
    \fn int QTime::msecsSinceStartOfDay() const

    Returns the number of msecs since the start of the day, i.e. since 00:00:00.

    \sa fromMSecsSinceStartOfDay()
*/

/*!
    \fn QTime::currentTime()

    Returns the current time as reported by the system clock.

    Note that the accuracy depends on the accuracy of the underlying
    operating system; not all systems provide 1-millisecond accuracy.
*/

#ifndef QT_NO_DATESTRING

static QTime fromIsoTimeString(const QStringRef &string, Qt::DateFormat format, bool *isMidnight24)
{
    if (isMidnight24)
        *isMidnight24 = false;

    const int size = string.size();
    if (size < 5)
        return QTime();

    bool ok = false;
    int hour = string.mid(0, 2).toInt(&ok);
    if (!ok)
        return QTime();
    const int minute = string.mid(3, 2).toInt(&ok);
    if (!ok)
        return QTime();
    int second = 0;
    int msec = 0;

    if (size == 5) {
        // HH:MM format
        second = 0;
        msec = 0;
    } else if (string.at(5) == QLatin1Char(',') || string.at(5) == QLatin1Char('.')) {
        if (format == Qt::TextDate)
            return QTime();
        // ISODate HH:MM.SSSSSS format
        // We only want 5 digits worth of fraction of minute. This follows the existing
        // behavior that determines how milliseconds are read; 4 millisecond digits are
        // read and then rounded to 3. If we read at most 5 digits for fraction of minute,
        // the maximum amount of millisecond digits it will expand to once converted to
        // seconds is 4. E.g. 12:34,99999 will expand to 12:34:59.9994. The milliseconds
        // will then be rounded up AND clamped to 999.

        const QStringRef minuteFractionStr = string.mid(6, 5);
        const long minuteFractionInt = minuteFractionStr.toLong(&ok);
        if (!ok)
            return QTime();
        const float minuteFraction = double(minuteFractionInt) / (std::pow(double(10), minuteFractionStr.count()));

        const float secondWithMs = minuteFraction * 60;
        const float secondNoMs = std::floor(secondWithMs);
        const float secondFraction = secondWithMs - secondNoMs;
        second = secondNoMs;
        msec = qMin(qRound(secondFraction * 1000.0), 999);
    } else {
        // HH:MM:SS or HH:MM:SS.sssss
        second = string.mid(6, 2).toInt(&ok);
        if (!ok)
            return QTime();
        if (size > 8 && (string.at(8) == QLatin1Char(',') || string.at(8) == QLatin1Char('.'))) {
            const QStringRef msecStr(string.mid(9, 4));
            int msecInt = msecStr.isEmpty() ? 0 : msecStr.toInt(&ok);
            if (!ok)
                return QTime();
            const double secondFraction(msecInt / (std::pow(double(10), msecStr.count())));
            msec = qMin(qRound(secondFraction * 1000.0), 999);
        }
    }

    if (format == Qt::ISODate && hour == 24 && minute == 0 && second == 0 && msec == 0) {
        if (isMidnight24)
            *isMidnight24 = true;
        hour = 0;
    }

    return QTime(hour, minute, second, msec);
}

/*!
    \fn QTime QTime::fromString(const QString &string, Qt::DateFormat format)

    Returns the time represented in the \a string as a QTime using the
    \a format given, or an invalid time if this is not possible.

    Note that fromString() uses a "C" locale encoded string to convert
    milliseconds to a float value. If the default locale is not "C",
    this may result in two conversion attempts (if the conversion
    fails for the default locale). This should be considered an
    implementation detail.
*/
QTime QTime::fromString(const QString& string, Qt::DateFormat format)
{
    if (string.isEmpty())
        return QTime();

    switch (format) {
    case Qt::SystemLocaleDate:
    case Qt::SystemLocaleShortDate:
        return QLocale::system().toTime(string, QLocale::ShortFormat);
    case Qt::SystemLocaleLongDate:
        return QLocale::system().toTime(string, QLocale::LongFormat);
    case Qt::LocaleDate:
    case Qt::DefaultLocaleShortDate:
        return QLocale().toTime(string, QLocale::ShortFormat);
    case Qt::DefaultLocaleLongDate:
        return QLocale().toTime(string, QLocale::LongFormat);
    case Qt::RFC2822Date: {
        QTime time;
        rfcDateImpl(string, 0, &time);
        return time;
    }
    case Qt::ISODate:
    case Qt::TextDate:
    default:
        return fromIsoTimeString(&string, format, 0);
    }
}

/*!
    \fn QTime::fromString(const QString &string, const QString &format)

    Returns the QTime represented by the \a string, using the \a
    format given, or an invalid time if the string cannot be parsed.

    These expressions may be used for the format:

    \table
    \header \li Expression \li Output
    \row \li h
         \li the hour without a leading zero (0 to 23 or 1 to 12 if AM/PM display)
    \row \li hh
         \li the hour with a leading zero (00 to 23 or 01 to 12 if AM/PM display)
    \row \li m \li the minute without a leading zero (0 to 59)
    \row \li mm \li the minute with a leading zero (00 to 59)
    \row \li s \li the second without a leading zero (0 to 59)
    \row \li ss \li the second with a leading zero (00 to 59)
    \row \li z \li the milliseconds without leading zeroes (0 to 999)
    \row \li zzz \li the milliseconds with leading zeroes (000 to 999)
    \row \li AP
         \li interpret as an AM/PM time. \e AP must be either "AM" or "PM".
    \row \li ap
         \li Interpret as an AM/PM time. \e ap must be either "am" or "pm".
    \endtable

    All other input characters will be treated as text. Any sequence
    of characters that are enclosed in single quotes will also be
    treated as text and not be used as an expression.

    \snippet code/src_corelib_tools_qdatetime.cpp 6

    If the format is not satisfied, an invalid QTime is returned.
    Expressions that do not expect leading zeroes to be given (h, m, s
    and z) are greedy. This means that they will use two digits even if
    this puts them outside the range of accepted values and leaves too
    few digits for other sections. For example, the following string
    could have meant 00:07:10, but the m will grab two digits, resulting
    in an invalid time:

    \snippet code/src_corelib_tools_qdatetime.cpp 7

    Any field that is not represented in the format will be set to zero.
    For example:

    \snippet code/src_corelib_tools_qdatetime.cpp 8

    \sa QDateTime::fromString(), QDate::fromString(), QDate::toString(),
    QDateTime::toString(), QTime::toString()
*/

QTime QTime::fromString(const QString &string, const QString &format)
{
    QTime time;
#ifndef QT_BOOTSTRAPPED
    QDateTimeParser dt(QVariant::Time, QDateTimeParser::FromString);
    if (dt.parseFormat(format))
        dt.fromString(string, 0, &time);
#else
    Q_UNUSED(string);
    Q_UNUSED(format);
#endif
    return time;
}

#endif // QT_NO_DATESTRING


/*!
    \overload

    Returns \c true if the specified time is valid; otherwise returns
    false.

    The time is valid if \a h is in the range 0 to 23, \a m and
    \a s are in the range 0 to 59, and \a ms is in the range 0 to 999.

    Example:

    \snippet code/src_corelib_tools_qdatetime.cpp 9
*/

bool QTime::isValid(int h, int m, int s, int ms)
{
    return (uint)h < 24 && (uint)m < 60 && (uint)s < 60 && (uint)ms < 1000;
}


/*!
    Sets this time to the current time. This is practical for timing:

    \snippet code/src_corelib_tools_qdatetime.cpp 10

    \sa restart(), elapsed(), currentTime()
*/

void QTime::start()
{
    *this = currentTime();
}

/*!
    Sets this time to the current time and returns the number of
    milliseconds that have elapsed since the last time start() or
    restart() was called.

    This function is guaranteed to be atomic and is thus very handy
    for repeated measurements. Call start() to start the first
    measurement, and restart() for each later measurement.

    Note that the counter wraps to zero 24 hours after the last call
    to start() or restart().

    \warning If the system's clock setting has been changed since the
    last time start() or restart() was called, the result is
    undefined. This can happen when daylight savings time is turned on
    or off.

    \sa start(), elapsed(), currentTime()
*/

int QTime::restart()
{
    QTime t = currentTime();
    int n = msecsTo(t);
    if (n < 0)                                // passed midnight
        n += 86400*1000;
    *this = t;
    return n;
}

/*!
    Returns the number of milliseconds that have elapsed since the
    last time start() or restart() was called.

    Note that the counter wraps to zero 24 hours after the last call
    to start() or restart.

    Note that the accuracy depends on the accuracy of the underlying
    operating system; not all systems provide 1-millisecond accuracy.

    \warning If the system's clock setting has been changed since the
    last time start() or restart() was called, the result is
    undefined. This can happen when daylight savings time is turned on
    or off.

    \sa start(), restart()
*/

int QTime::elapsed() const
{
    int n = msecsTo(currentTime());
    if (n < 0)                                // passed midnight
        n += 86400 * 1000;
    return n;
}

/*****************************************************************************
  QDateTime static helper functions
 *****************************************************************************/

// Calls the platform variant of tzset
static void qt_tzset()
{
#if defined(Q_OS_WINCE)
    // WinCE doesn't use tzset
    return;
#elif defined(Q_OS_WIN)
    _tzset();
#else
    tzset();
#endif // Q_OS_WIN
}

// Returns the platform variant of timezone, i.e. the standard time offset
// The timezone external variable is documented as always holding the
// Standard Time offset as seconds west of Greenwich, i.e. UTC+01:00 is -3600
// Note this may not be historicaly accurate.
// Relies on tzset, mktime, or localtime having been called to populate timezone
static int qt_timezone()
{
#if defined(Q_OS_WINCE)
        TIME_ZONE_INFORMATION tzi;
        GetTimeZoneInformation(&tzi);
        // Expressed in minutes, convert to seconds
        return (tzi.Bias + tzi.StandardBias) * 60;
#elif defined(_MSC_VER) && _MSC_VER >= 1400
        long offset;
        _get_timezone(&offset);
        return offset;
#elif defined(Q_OS_BSD4) && !defined(Q_OS_DARWIN)
        time_t clock = time(NULL);
        struct tm t;
        localtime_r(&clock, &t);
        // QTBUG-36080 Workaround for systems without the POSIX timezone
        // variable. This solution is not very efficient but fixing it is up to
        // the libc implementations.
        //
        // tm_gmtoff has some important differences compared to the timezone
        // variable:
        // - It returns the number of seconds east of UTC, and we want the
        //   number of seconds west of UTC.
        // - It also takes DST into account, so we need to adjust it to always
        //   get the Standard Time offset.
        return -t.tm_gmtoff + (t.tm_isdst ? SECS_PER_HOUR : 0L);
#else
        return timezone;
#endif // Q_OS_WIN
}

// Returns the tzname, assume tzset has been called already
static QString qt_tzname(QDateTimePrivate::DaylightStatus daylightStatus)
{
#if defined(Q_OS_WINCE)
    TIME_ZONE_INFORMATION tzi;
    DWORD res = GetTimeZoneInformation(&tzi);
    if (res == TIME_ZONE_ID_UNKNOWN)
        return QString();
    else if (daylightStatus == QDateTimePrivate::DaylightTime)
        return QString::fromWCharArray(tzi.DaylightName);
    else
        return QString::fromWCharArray(tzi.StandardName);
#else
    int isDst = (daylightStatus == QDateTimePrivate::DaylightTime) ? 1 : 0;
#if defined(_MSC_VER) && _MSC_VER >= 1400
    size_t s = 0;
    char name[512];
    if (_get_tzname(&s, name, 512, isDst))
        return QString();
    return QString::fromLocal8Bit(name);
#else
    return QString::fromLocal8Bit(tzname[isDst]);
#endif // Q_OS_WIN
#endif // Q_OS_WINCE
}

// Calls the platform variant of mktime for the given date, time and daylightStatus,
// and updates the date, time, daylightStatus and abbreviation with the returned values
// If the date falls outside the 1970 to 2037 range supported by mktime / time_t
// then null date/time will be returned, you should adjust the date first if
// you need a guaranteed result.
static qint64 qt_mktime(QDate *date, QTime *time, QDateTimePrivate::DaylightStatus *daylightStatus,
                        QString *abbreviation, bool *ok)
{
    const qint64 msec = time->msec();
    int yy, mm, dd;
    date->getDate(&yy, &mm, &dd);

#if defined(Q_OS_WINCE)
    // WinCE doesn't provide standard C library time functions
    SYSTEMTIME st;
    memset(&st, 0, sizeof(SYSTEMTIME));
    st.wSecond = time->second();
    st.wMinute = time->minute();
    st.wHour = time->hour();
    st.wDay = dd;
    st.wMonth = mm;
    st.wYear = yy;
    FILETIME lft;
    bool valid = SystemTimeToFileTime(&st, &lft);
    FILETIME ft;
    if (valid)
        valid = LocalFileTimeToFileTime(&lft, &ft);
    const time_t secsSinceEpoch = ftToTime_t(ft);
    const time_t localSecs = ftToTime_t(lft);
    TIME_ZONE_INFORMATION tzi;
    GetTimeZoneInformation(&tzi);
    bool isDaylight = false;
    // Check for overflow
    qint64 localDiff = qAbs(localSecs - secsSinceEpoch);
    int daylightOffset = qAbs(tzi.Bias + tzi.DaylightBias) * 60;
    if (localDiff > daylightOffset)
        valid = false;
    else
        isDaylight = (localDiff == daylightOffset);
    if (daylightStatus) {
        if (isDaylight)
            *daylightStatus = QDateTimePrivate::DaylightTime;
        else
            *daylightStatus = QDateTimePrivate::StandardTime;
    }
    if (abbreviation) {
        if (isDaylight)
            *abbreviation = QString::fromWCharArray(tzi.DaylightName);
        else
            *abbreviation = QString::fromWCharArray(tzi.StandardName);
    }
    if (ok)
        *ok = valid;
#else
    // All other platforms provide standard C library time functions
    tm local;
    local.tm_sec = time->second();
    local.tm_min = time->minute();
    local.tm_hour = time->hour();
    local.tm_mday = dd;
    local.tm_mon = mm - 1;
    local.tm_year = yy - 1900;
    local.tm_wday = 0;
    local.tm_yday = 0;
    if (daylightStatus)
        local.tm_isdst = int(*daylightStatus);
    else
        local.tm_isdst = -1;
#if defined(Q_OS_WIN)
    int hh = local.tm_hour;
#endif // Q_OS_WIN
    time_t secsSinceEpoch = mktime(&local);
    if (secsSinceEpoch != time_t(-1)) {
        *date = QDate(local.tm_year + 1900, local.tm_mon + 1, local.tm_mday);
        *time = QTime(local.tm_hour, local.tm_min, local.tm_sec, msec);
#if defined(Q_OS_WIN)
        // Windows mktime for the missing hour subtracts 1 hour from the time
        // instead of adding 1 hour.  If time differs and is standard time then
        // this has happened, so add 2 hours to the time and 1 hour to the msecs
        if (local.tm_isdst == 0 && local.tm_hour != hh) {
            if (time->hour() >= 22)
                *date = date->addDays(1);
            *time = time->addSecs(2 * SECS_PER_HOUR);
            secsSinceEpoch += SECS_PER_HOUR;
            local.tm_isdst = 1;
        }
#endif // Q_OS_WIN
        if (local.tm_isdst >= 1) {
            if (daylightStatus)
                *daylightStatus = QDateTimePrivate::DaylightTime;
            if (abbreviation)
                *abbreviation = qt_tzname(QDateTimePrivate::DaylightTime);
        } else if (local.tm_isdst == 0) {
            if (daylightStatus)
                *daylightStatus = QDateTimePrivate::StandardTime;
            if (abbreviation)
                *abbreviation = qt_tzname(QDateTimePrivate::StandardTime);
        } else {
            if (daylightStatus)
                *daylightStatus = QDateTimePrivate::UnknownDaylightTime;
            if (abbreviation)
                *abbreviation = qt_tzname(QDateTimePrivate::StandardTime);
        }
        if (ok)
            *ok = true;
    } else {
        *date = QDate();
        *time = QTime();
        if (daylightStatus)
            *daylightStatus = QDateTimePrivate::UnknownDaylightTime;
        if (abbreviation)
            *abbreviation = QString();
        if (ok)
            *ok = false;
    }
#endif // Q_OS_WINCE

    return ((qint64)secsSinceEpoch * 1000) + msec;
}

// Calls the platform variant of localtime for the given msecs, and updates
// the date, time, and daylight status with the returned values.
static bool qt_localtime(qint64 msecsSinceEpoch, QDate *localDate, QTime *localTime,
                         QDateTimePrivate::DaylightStatus *daylightStatus)
{
    const time_t secsSinceEpoch = msecsSinceEpoch / 1000;
    const int msec = msecsSinceEpoch % 1000;

    tm local;
    bool valid = false;

#if defined(Q_OS_WINCE)
    FILETIME utcTime = time_tToFt(secsSinceEpoch);
    FILETIME resultTime;
    valid = FileTimeToLocalFileTime(&utcTime , &resultTime);
    SYSTEMTIME sysTime;
    if (valid)
        valid = FileTimeToSystemTime(&resultTime , &sysTime);

    if (valid) {
        local.tm_sec = sysTime.wSecond;
        local.tm_min = sysTime.wMinute;
        local.tm_hour = sysTime.wHour;
        local.tm_mday = sysTime.wDay;
        local.tm_mon = sysTime.wMonth - 1;
        local.tm_year = sysTime.wYear - 1900;
    }
#elif !defined(QT_NO_THREAD) && defined(_POSIX_THREAD_SAFE_FUNCTIONS)
    // Use the reentrant version of localtime() where available
    // as is thread-safe and doesn't use a shared static data area
    tm *res = 0;
    res = localtime_r(&secsSinceEpoch, &local);
    if (res)
        valid = true;
#elif defined(_MSC_VER) && _MSC_VER >= 1400
    if (!_localtime64_s(&local, &secsSinceEpoch))
        valid = true;
#else
    // Returns shared static data which may be overwritten at any time
    // So copy the result asap
    tm *res = 0;
    res = localtime(&secsSinceEpoch);
    if (res) {
        local = *res;
        valid = true;
    }
#endif
    if (valid) {
        *localDate = QDate(local.tm_year + 1900, local.tm_mon + 1, local.tm_mday);
        *localTime = QTime(local.tm_hour, local.tm_min, local.tm_sec, msec);
        if (daylightStatus) {
            if (local.tm_isdst > 0)
                *daylightStatus = QDateTimePrivate::DaylightTime;
            else if (local.tm_isdst < 0)
                *daylightStatus = QDateTimePrivate::UnknownDaylightTime;
            else
                *daylightStatus = QDateTimePrivate::StandardTime;
        }
        return true;
    } else {
        *localDate = QDate();
        *localTime = QTime();
        if (daylightStatus)
            *daylightStatus = QDateTimePrivate::UnknownDaylightTime;
        return false;
    }
}

// Converts an msecs value into a date and time
static void msecsToTime(qint64 msecs, QDate *date, QTime *time)
{
    qint64 jd = JULIAN_DAY_FOR_EPOCH;
    qint64 ds = 0;

    if (qAbs(msecs) >= MSECS_PER_DAY) {
        jd += (msecs / MSECS_PER_DAY);
        msecs %= MSECS_PER_DAY;
    }

    if (msecs < 0) {
        ds = MSECS_PER_DAY - msecs - 1;
        jd -= ds / MSECS_PER_DAY;
        ds = ds % MSECS_PER_DAY;
        ds = MSECS_PER_DAY - ds - 1;
    } else {
        ds = msecs;
    }

    if (date)
        *date = QDate::fromJulianDay(jd);
    if (time)
        *time = QTime::fromMSecsSinceStartOfDay(ds);
}

// Converts a date/time value into msecs
static qint64 timeToMSecs(const QDate &date, const QTime &time)
{
    return ((date.toJulianDay() - JULIAN_DAY_FOR_EPOCH) * MSECS_PER_DAY)
           + time.msecsSinceStartOfDay();
}

// Convert an MSecs Since Epoch into Local Time
static bool epochMSecsToLocalTime(qint64 msecs, QDate *localDate, QTime *localTime,
                                  QDateTimePrivate::DaylightStatus *daylightStatus = 0)
{
    if (msecs < 0) {
        // Docs state any LocalTime before 1970-01-01 will *not* have any Daylight Time applied
        // Instead just use the standard offset from UTC to convert to UTC time
        qt_tzset();
        msecsToTime(msecs - qt_timezone() * 1000, localDate, localTime);
        if (daylightStatus)
            *daylightStatus = QDateTimePrivate::StandardTime;
        return true;
    } else if (msecs > (qint64(TIME_T_MAX) * 1000)) {
        // Docs state any LocalTime after 2037-12-31 *will* have any Daylight Time applied
        // but this may fall outside the supported time_t range, so need to fake it.
        // Use existing method to fake the conversion, but this is deeply flawed as it may
        // apply the conversion from the wrong day number, e.g. if rule is last Sunday of month
        // TODO Use QTimeZone when available to apply the future rule correctly
        QDate utcDate;
        QTime utcTime;
        msecsToTime(msecs, &utcDate, &utcTime);
        int year, month, day;
        utcDate.getDate(&year, &month, &day);
        // 2037 is not a leap year, so make sure date isn't Feb 29
        if (month == 2 && day == 29)
            --day;
        QDate fakeDate(2037, month, day);
        qint64 fakeMsecs = QDateTime(fakeDate, utcTime, Qt::UTC).toMSecsSinceEpoch();
        bool res = qt_localtime(fakeMsecs, localDate, localTime, daylightStatus);
        *localDate = localDate->addDays(fakeDate.daysTo(utcDate));
        return res;
    } else {
        // Falls inside time_t suported range so can use localtime
        return qt_localtime(msecs, localDate, localTime, daylightStatus);
    }
}

// Convert a LocalTime expressed in local msecs encoding into a UTC epoch msecs
// Optionally populate the returned values from mktime for the adjusted local
// date and time and daylight status.  Uses daylightStatus in calculation if populated.
static qint64 localMSecsToEpochMSecs(qint64 localMsecs, QDate *localDate = 0, QTime *localTime = 0,
                                     QDateTimePrivate::DaylightStatus *daylightStatus = 0,
                                     QString *abbreviation = 0, bool *ok = 0)
{
    QDate dt;
    QTime tm;
    msecsToTime(localMsecs, &dt, &tm);

    qint64 msecsMax = qint64(TIME_T_MAX) * 1000;

    if (localMsecs <= qint64(MSECS_PER_DAY)) {

        // Docs state any LocalTime before 1970-01-01 will *not* have any Daylight Time applied

        // First, if localMsecs is within +/- 1 day of minimum time_t try mktime in case it does
        // fall after minimum and needs proper daylight conversion
        if (localMsecs >= -qint64(MSECS_PER_DAY)) {
            bool valid;
            qint64 utcMsecs = qt_mktime(&dt, &tm, daylightStatus, abbreviation, &valid);
            if (valid && utcMsecs >= 0) {
                // mktime worked and falls in valid range, so use it
                if (localDate)
                    *localDate = dt;
                if (localTime)
                    *localTime = tm;
                if (ok)
                    *ok = true;
                return utcMsecs;
            }
        } else {
            // If we don't call mktime then need to call tzset to get offset
            qt_tzset();
        }
        // Time is clearly before 1970-01-01 so just use standard offset to convert
        qint64 utcMsecs = localMsecs + qt_timezone() * 1000;
        if (localDate || localTime)
            msecsToTime(localMsecs, localDate, localTime);
        if (daylightStatus)
            *daylightStatus = QDateTimePrivate::StandardTime;
        if (abbreviation)
            *abbreviation = qt_tzname(QDateTimePrivate::StandardTime);
        if (ok)
            *ok = true;
        return utcMsecs;

    } else if (localMsecs >= msecsMax - MSECS_PER_DAY) {

        // Docs state any LocalTime after 2037-12-31 *will* have any Daylight Time applied
        // but this may fall outside the supported time_t range, so need to fake it.

        // First, if localMsecs is within +/- 1 day of maximum time_t try mktime in case it does
        // fall before maximum and can use proper daylight conversion
        if (localMsecs <= msecsMax + MSECS_PER_DAY) {
            bool valid;
            qint64 utcMsecs = qt_mktime(&dt, &tm, daylightStatus, abbreviation, &valid);
            if (valid && utcMsecs <= msecsMax) {
                // mktime worked and falls in valid range, so use it
                if (localDate)
                    *localDate = dt;
                if (localTime)
                    *localTime = tm;
                if (ok)
                    *ok = true;
                return utcMsecs;
            }
        }
        // Use existing method to fake the conversion, but this is deeply flawed as it may
        // apply the conversion from the wrong day number, e.g. if rule is last Sunday of month
        // TODO Use QTimeZone when available to apply the future rule correctly
        int year, month, day;
        dt.getDate(&year, &month, &day);
        // 2037 is not a leap year, so make sure date isn't Feb 29
        if (month == 2 && day == 29)
            --day;
        QDate fakeDate(2037, month, day);
        qint64 fakeDiff = fakeDate.daysTo(dt);
        qint64 utcMsecs = qt_mktime(&fakeDate, &tm, daylightStatus, abbreviation, ok);
        if (localDate)
            *localDate = fakeDate.addDays(fakeDiff);
        if (localTime)
            *localTime = tm;
        QDate utcDate;
        QTime utcTime;
        msecsToTime(utcMsecs, &utcDate, &utcTime);
        utcDate = utcDate.addDays(fakeDiff);
        utcMsecs = timeToMSecs(utcDate, utcTime);
        return utcMsecs;

    } else {

        // Clearly falls inside 1970-2037 suported range so can use mktime
        qint64 utcMsecs = qt_mktime(&dt, &tm, daylightStatus, abbreviation, ok);
        if (localDate)
            *localDate = dt;
        if (localTime)
            *localTime = tm;
        return utcMsecs;

    }
}

/*****************************************************************************
  QDateTimePrivate member functions
 *****************************************************************************/

QDateTimePrivate::QDateTimePrivate(const QDate &toDate, const QTime &toTime, Qt::TimeSpec toSpec,
                                   int offsetSeconds)
    : m_msecs(0),
      m_spec(Qt::LocalTime),
      m_offsetFromUtc(0),
      m_status(0)
{
    setTimeSpec(toSpec, offsetSeconds);
    setDateTime(toDate, toTime);
}

#ifndef QT_BOOTSTRAPPED
QDateTimePrivate::QDateTimePrivate(const QDate &toDate, const QTime &toTime,
                                   const QTimeZone &toTimeZone)
    : m_spec(Qt::TimeZone),
      m_offsetFromUtc(0),
      m_timeZone(toTimeZone),
      m_status(0)
{
    setDateTime(toDate, toTime);
}
#endif // QT_BOOTSTRAPPED

void QDateTimePrivate::setTimeSpec(Qt::TimeSpec spec, int offsetSeconds)
{
    clearValidDateTime();
    clearTimeZoneCached();
    clearSetToDaylightStatus();

#ifndef QT_BOOTSTRAPPED
    m_timeZone = QTimeZone();
#endif // QT_BOOTSTRAPPED

    switch (spec) {
    case Qt::OffsetFromUTC:
        if (offsetSeconds == 0) {
            m_spec = Qt::UTC;
            m_offsetFromUtc = 0;
        } else {
            m_spec = Qt::OffsetFromUTC;
            m_offsetFromUtc = offsetSeconds;
        }
        break;
    case Qt::TimeZone:
        // Use system time zone instead
        m_spec = Qt::LocalTime;
        m_offsetFromUtc = 0;
        break;
    case Qt::UTC:
    case Qt::LocalTime:
        m_spec = spec;
        m_offsetFromUtc = 0;
        break;
    }
}

void QDateTimePrivate::setDateTime(const QDate &date, const QTime &time)
{
    // If the date is valid and the time is not we set time to 00:00:00
    QTime useTime = time;
    if (!useTime.isValid() && date.isValid())
        useTime = QTime::fromMSecsSinceStartOfDay(0);

    // Reset the status
    m_status = 0;

    // Set date value and status
    qint64 days = 0;
    if (date.isValid()) {
        days = date.toJulianDay() - JULIAN_DAY_FOR_EPOCH;
        m_status = ValidDate;
    } else if (date.isNull()) {
        m_status = NullDate;
    }

    // Set time value and status
    int ds = 0;
    if (useTime.isValid()) {
        ds = useTime.msecsSinceStartOfDay();
        m_status = m_status | ValidTime;
    } else if (time.isNull()) {
        m_status = m_status | NullTime;
    }

    // Set msecs serial value
    m_msecs = (days * MSECS_PER_DAY) + ds;

    // Set if date and time are valid
    checkValidDateTime();
}

void QDateTimePrivate::getDateTime(QDate *date, QTime *time) const
{
    msecsToTime(m_msecs, date, time);

    if (date && isNullDate())
        *date = QDate();

    if (time && isNullTime())
        *time = QTime();
}

// Set the Daylight Status if LocalTime set via msecs
void QDateTimePrivate::setDaylightStatus(QDateTimePrivate::DaylightStatus status)
{
    if (status == DaylightTime) {
        m_status = m_status & ~SetToStandardTime;
        m_status = m_status | SetToDaylightTime;
    } else if (status == StandardTime) {
        m_status = m_status & ~SetToDaylightTime;
        m_status = m_status | SetToStandardTime;
    } else {
        clearSetToDaylightStatus();
    }
}

// Get the Daylight Status if LocalTime set via msecs
QDateTimePrivate::DaylightStatus QDateTimePrivate::daylightStatus() const
{
    if ((m_status & SetToDaylightTime) == SetToDaylightTime)
        return DaylightTime;
    if ((m_status & SetToStandardTime) == SetToStandardTime)
        return StandardTime;
    return UnknownDaylightTime;
}

// Check the UTC / offsetFromUTC validity
void QDateTimePrivate::checkValidDateTime()
{
    switch (m_spec) {
    case Qt::OffsetFromUTC:
    case Qt::UTC:
        if (isValidDate() && isValidTime())
            setValidDateTime();
        else
            clearValidDateTime();
        break;
    case Qt::TimeZone:
        // Defer checking until required as can be expensive
        clearValidDateTime();
        clearTimeZoneCached();
        m_offsetFromUtc = 0;
        break;
    case Qt::LocalTime:
        // Defer checking until required as can be expensive
        clearValidDateTime();
        m_offsetFromUtc = 0;
        break;
    }
}

// Refresh the LocalTime validity and offset
void QDateTimePrivate::refreshDateTime()
{
    switch (m_spec) {
    case Qt::OffsetFromUTC:
    case Qt::UTC:
        // Always set by setDateTime so just return
        return;
    case Qt::TimeZone:
        // If already cached then don't need to refresh as tz won't change
        if (isTimeZoneCached())
            return;
        // Flag that will have a cached result after calculations
        setTimeZoneCached();
        break;
    case Qt::LocalTime:
        break;
    }

    // If not valid date and time then is invalid
    if (!isValidDate() || !isValidTime()) {
        clearValidDateTime();
        m_offsetFromUtc = 0;
        return;
    }

#ifndef QT_BOOTSTRAPPED
    // If not valid time zone then is invalid
    if (m_spec == Qt::TimeZone && !m_timeZone.isValid()) {
        clearValidDateTime();
        m_offsetFromUtc = 0;
        return;
    }
#endif // QT_BOOTSTRAPPED

    // We have a valid date and time and a Qt::LocalTime or Qt::TimeZone that needs calculating
    // LocalTime and TimeZone might fall into "missing" DaylightTime transition hour
    // Calling toEpochMSecs will adjust the returned date/time if it does
    QDate testDate;
    QTime testTime;
    qint64 epochMSecs = 0;
    if (m_spec == Qt::LocalTime) {
        DaylightStatus status = daylightStatus();
        epochMSecs = localMSecsToEpochMSecs(m_msecs, &testDate, &testTime, &status);
#ifndef QT_BOOTSTRAPPED
    } else {
        epochMSecs = zoneMSecsToEpochMSecs(m_msecs, m_timeZone, &testDate, &testTime);
#endif // QT_BOOTSTRAPPED
    }
    if (timeToMSecs(testDate, testTime) == m_msecs) {
        setValidDateTime();
        // Cache the offset to use in toMSecsSinceEpoch()
        m_offsetFromUtc = (m_msecs - epochMSecs) / 1000;
    } else {
        clearValidDateTime();
        m_offsetFromUtc = 0;
    }
}

#ifndef QT_BOOTSTRAPPED
// Convert a TimeZone time expressed in zone msecs encoding into a UTC epoch msecs
qint64 QDateTimePrivate::zoneMSecsToEpochMSecs(qint64 zoneMSecs, const QTimeZone &zone,
                                               QDate *localDate, QTime *localTime)
{
    // Get the effective data from QTimeZone
    QTimeZonePrivate::Data data = zone.d->dataForLocalTime(zoneMSecs);
    // Docs state any LocalTime before 1970-01-01 will *not* have any Daylight Time applied
    // but all times afterwards will have Daylight Time applied.
    if (data.atMSecsSinceEpoch >= 0) {
        msecsToTime(data.atMSecsSinceEpoch + (data.offsetFromUtc * 1000), localDate, localTime);
        return data.atMSecsSinceEpoch;
    } else {
        msecsToTime(zoneMSecs, localDate, localTime);
        return zoneMSecs - (data.standardTimeOffset * 1000);
    }
}
#endif // QT_BOOTSTRAPPED

/*****************************************************************************
  QDateTime member functions
 *****************************************************************************/

/*!
    \class QDateTime
    \inmodule QtCore
    \ingroup shared
    \reentrant
    \brief The QDateTime class provides date and time functions.


    A QDateTime object contains a calendar date and a clock time (a
    "datetime"). It is a combination of the QDate and QTime classes.
    It can read the current datetime from the system clock. It
    provides functions for comparing datetimes and for manipulating a
    datetime by adding a number of seconds, days, months, or years.

    A QDateTime object is typically created either by giving a date
    and time explicitly in the constructor, or by using the static
    function currentDateTime() that returns a QDateTime object set
    to the system clock's time. The date and time can be changed with
    setDate() and setTime(). A datetime can also be set using the
    setTime_t() function that takes a POSIX-standard "number of
    seconds since 00:00:00 on January 1, 1970" value. The fromString()
    function returns a QDateTime, given a string and a date format
    used to interpret the date within the string.

    The date() and time() functions provide access to the date and
    time parts of the datetime. The same information is provided in
    textual format by the toString() function.

    QDateTime provides a full set of operators to compare two
    QDateTime objects, where smaller means earlier and larger means
    later.

    You can increment (or decrement) a datetime by a given number of
    milliseconds using addMSecs(), seconds using addSecs(), or days
    using addDays(). Similarly, you can use addMonths() and addYears().
    The daysTo() function returns the number of days between two datetimes,
    secsTo() returns the number of seconds between two datetimes, and
    msecsTo() returns the number of milliseconds between two datetimes.

    QDateTime can store datetimes as \l{Qt::LocalTime}{local time} or
    as \l{Qt::UTC}{UTC}. QDateTime::currentDateTime() returns a
    QDateTime expressed as local time; use toUTC() to convert it to
    UTC. You can also use timeSpec() to find out if a QDateTime
    object stores a UTC time or a local time. Operations such as
    addSecs() and secsTo() are aware of daylight saving time (DST).

    \note QDateTime does not account for leap seconds.

    \section1

    \section2 No Year 0

    There is no year 0. Dates in that year are considered invalid. The
    year -1 is the year "1 before Christ" or "1 before current era."
    The day before 1 January 1 CE is 31 December 1 BCE.

    \section2 Range of Valid Dates

    The range of valid values able to be stored in QDateTime is dependent on
    the internal storage implementation. QDateTime is currently stored in a
    qint64 as a serial msecs value encoding the date and time.  This restricts
    the date range to about +/- 292 million years, compared to the QDate range
    of +/- 2 billion years.  Care must be taken when creating a QDateTime with
    extreme values that you do not overflow the storage.  The exact range of
    supported values varies depending on the Qt::TimeSpec and time zone.

    \section2
    Use of System Timezone

    QDateTime uses the system's time zone information to determine the
    offset of local time from UTC. If the system is not configured
    correctly or not up-to-date, QDateTime will give wrong results as
    well.

    \section2 Daylight Savings Time (DST)

    QDateTime takes into account the system's time zone information
    when dealing with DST. On modern Unix systems, this means it
    applies the correct historical DST data whenever possible. On
    Windows and Windows CE, where the system doesn't support
    historical DST data, historical accuracy is not maintained with
    respect to DST.

    The range of valid dates taking DST into account is 1970-01-01 to
    the present, and rules are in place for handling DST correctly
    until 2037-12-31, but these could change. For dates falling
    outside that range, QDateTime makes a \e{best guess} using the
    rules for year 1970 or 2037, but we can't guarantee accuracy. This
    means QDateTime doesn't take into account changes in a locale's
    time zone before 1970, even if the system's time zone database
    supports that information.

    QDateTime takes into consideration the Standard Time to Daylight Time
    transition.  For example if the transition is at 2am and the clock goes
    forward to 3am, then there is a "missing" hour from 02:00:00 to 02:59:59.999
    which QDateTime considers to be invalid.  Any date maths performed
    will take this missing hour into account and return a valid result.

    \section2 Offset From UTC

    A Qt::TimeSpec of Qt::OffsetFromUTC is also supported. This allows you
    to define a QDateTime relative to UTC at a fixed offset of a given number
    of seconds from UTC.  For example, an offset of +3600 seconds is one hour
    ahead of UTC and is usually written in ISO standard notation as
    "UTC+01:00".  Daylight Savings Time never applies with this TimeSpec.

    There is no explicit size restriction to the offset seconds, but there is
    an implicit limit imposed when using the toString() and fromString()
    methods which use a format of [+|-]hh:mm, effectively limiting the range
    to +/- 99 hours and 59 minutes and whole minutes only.  Note that currently
    no time zone lies outside the range of +/- 14 hours.

    \section2 Time Zone Support

    A Qt::TimeSpec of Qt::TimeZone is also supported in conjunction with the
    QTimeZone class.  This allows you to define a datetime in a named time zone
    adhering to a consistent set of daylight savings transition rules.  For
    example a time zone of "Europe/Berlin" will apply the daylight savings
    rules as used in Germany since 1970.  Note that the transition rules
    applied depend on the platform support.  See the QTimeZone documentation
    for more details.

    \sa QDate, QTime, QDateTimeEdit, QTimeZone
*/

/*!
    Constructs a null datetime (i.e. null date and null time). A null
    datetime is invalid, since the date is invalid.

    \sa isValid()
*/
QDateTime::QDateTime()
    : d(new QDateTimePrivate)
{
}


/*!
    Constructs a datetime with the given \a date, a valid
    time(00:00:00.000), and sets the timeSpec() to Qt::LocalTime.
*/

QDateTime::QDateTime(const QDate &date)
    : d(new QDateTimePrivate(date, QTime(0, 0, 0), Qt::LocalTime, 0))
{
}

/*!
    Constructs a datetime with the given \a date and \a time, using
    the time specification defined by \a spec.

    If \a date is valid and \a time is not, the time will be set to midnight.

    If \a spec is Qt::OffsetFromUTC then it will be set to Qt::UTC, i.e. an
    offset of 0 seconds. To create a Qt::OffsetFromUTC datetime use the
    correct constructor.

    If \a spec is Qt::TimeZone then the spec will be set to Qt::LocalTime,
    i.e. the current system time zone.  To create a Qt::TimeZone datetime
    use the correct constructor.
*/

QDateTime::QDateTime(const QDate &date, const QTime &time, Qt::TimeSpec spec)
    : d(new QDateTimePrivate(date, time, spec, 0))
{
}

/*!
    \since 5.2

    Constructs a datetime with the given \a date and \a time, using
    the time specification defined by \a spec and \a offsetSeconds seconds.

    If \a date is valid and \a time is not, the time will be set to midnight.

    If the \a spec is not Qt::OffsetFromUTC then \a offsetSeconds will be ignored.

    If the \a spec is Qt::OffsetFromUTC and \a offsetSeconds is 0 then the
    timeSpec() will be set to Qt::UTC, i.e. an offset of 0 seconds.

    If \a spec is Qt::TimeZone then the spec will be set to Qt::LocalTime,
    i.e. the current system time zone.  To create a Qt::TimeZone datetime
    use the correct constructor.
*/

QDateTime::QDateTime(const QDate &date, const QTime &time, Qt::TimeSpec spec, int offsetSeconds)
         : d(new QDateTimePrivate(date, time, spec, offsetSeconds))
{
}

#ifndef QT_BOOTSTRAPPED
/*!
    \since 5.2

    Constructs a datetime with the given \a date and \a time, using
    the Time Zone specified by \a timeZone.

    If \a date is valid and \a time is not, the time will be set to 00:00:00.

    If \a timeZone is invalid then the datetime will be invalid.
*/

QDateTime::QDateTime(const QDate &date, const QTime &time, const QTimeZone &timeZone)
    : d(new QDateTimePrivate(date, time, timeZone))
{
}
#endif // QT_BOOTSTRAPPED

/*!
    Constructs a copy of the \a other datetime.
*/

QDateTime::QDateTime(const QDateTime &other)
    : d(other.d)
{
}

/*!
    Destroys the datetime.
*/
QDateTime::~QDateTime()
{
}

/*!
    Makes a copy of the \a other datetime and returns a reference to the
    copy.
*/

QDateTime &QDateTime::operator=(const QDateTime &other)
{
    d = other.d;
    return *this;
}
/*!
    \fn void QDateTime::swap(QDateTime &other)
    \since 5.0

    Swaps this datetime with \a other. This operation is very fast
    and never fails.
*/

/*!
    Returns \c true if both the date and the time are null; otherwise
    returns \c false. A null datetime is invalid.

    \sa QDate::isNull(), QTime::isNull(), isValid()
*/

bool QDateTime::isNull() const
{
    return d->isNullDate() && d->isNullTime();
}

/*!
    Returns \c true if both the date and the time are valid and they are valid in
    the current Qt::TimeSpec, otherwise returns \c false.

    If the timeSpec() is Qt::LocalTime or Qt::TimeZone then the date and time are
    checked to see if they fall in the Standard Time to Daylight Time transition
    hour, i.e. if the transition is at 2am and the clock goes forward to 3am
    then the time from 02:00:00 to 02:59:59.999 is considered to be invalid.

    \sa QDate::isValid(), QTime::isValid()
*/

bool QDateTime::isValid() const
{
    d->refreshDateTime();
    return (d->isValidDateTime());
}

/*!
    Returns the date part of the datetime.

    \sa setDate(), time(), timeSpec()
*/

QDate QDateTime::date() const
{
    if (d->isNullDate())
        return QDate();
    QDate dt;
    msecsToTime(d->m_msecs, &dt, 0);
    return dt;
}

/*!
    Returns the time part of the datetime.

    \sa setTime(), date(), timeSpec()
*/

QTime QDateTime::time() const
{
    if (d->isNullTime())
        return QTime();
    QTime tm;
    msecsToTime(d->m_msecs, 0, &tm);
    return tm;
}

/*!
    Returns the time specification of the datetime.

    \sa setTimeSpec(), date(), time(), Qt::TimeSpec
*/

Qt::TimeSpec QDateTime::timeSpec() const
{
    return d->m_spec;
}

#ifndef QT_BOOTSTRAPPED
/*!
    \since 5.2

    Returns the time zone of the datetime.

    If the timeSpec() is Qt::LocalTime then an instance of the current system
    time zone will be returned. Note however that if you copy this time zone
    the instance will not remain in sync if the system time zone changes.

    \sa setTimeZone(), Qt::TimeSpec
*/

QTimeZone QDateTime::timeZone() const
{
    switch (d->m_spec) {
    case Qt::OffsetFromUTC:
        if (!d->m_timeZone.isValid())
            d->m_timeZone = QTimeZone(d->m_offsetFromUtc);
        return d->m_timeZone;
    case Qt::UTC:
        if (!d->m_timeZone.isValid())
            d->m_timeZone = QTimeZone("UTC");
        return d->m_timeZone;
    case Qt::TimeZone :
        return d->m_timeZone;
    case Qt::LocalTime:
        return QTimeZone(QTimeZone::systemTimeZoneId());
    }
    return QTimeZone();
}
#endif // QT_BOOTSTRAPPED

/*!
    \since 5.2

    Returns the current Offset From UTC in seconds.

    If the timeSpec() is Qt::OffsetFromUTC this will be the value originally set.

    If the timeSpec() is Qt::TimeZone this will be the offset effective in the
    Time Zone including any Daylight Saving Offset.

    If the timeSpec() is Qt::LocalTime this will be the difference between the
    Local Time and UTC including any Daylight Saving Offset.

    If the timeSpec() is Qt::UTC this will be 0.

    \sa setOffsetFromUtc()
*/

int QDateTime::offsetFromUtc() const
{
    d->refreshDateTime();
    return d->m_offsetFromUtc;
}

/*!
    \since 5.2

    Returns the Time Zone Abbreviation for the datetime.

    If the timeSpec() is Qt::UTC this will be "UTC".

    If the timeSpec() is Qt::OffsetFromUTC this will be in the format
    "UTC[+-]00:00".

    If the timeSpec() is Qt::LocalTime then the host system is queried for the
    correct abbreviation.

    Note that abbreviations may or may not be localized.

    Note too that the abbreviation is not guaranteed to be a unique value,
    i.e. different time zones may have the same abbreviation.

    \sa timeSpec()
*/

QString QDateTime::timeZoneAbbreviation() const
{
    switch (d->m_spec) {
    case Qt::UTC:
        return QStringLiteral("UTC");
    case Qt::OffsetFromUTC:
        return QLatin1String("UTC") + toOffsetString(Qt::ISODate, d->m_offsetFromUtc);
    case Qt::TimeZone:
#ifndef QT_BOOTSTRAPPED
        return d->m_timeZone.d->abbreviation(d->toMSecsSinceEpoch());
#endif // QT_BOOTSTRAPPED
    case Qt::LocalTime:  {
        QString abbrev;
        QDateTimePrivate::DaylightStatus status = d->daylightStatus();
        localMSecsToEpochMSecs(d->m_msecs, 0, 0, &status, &abbrev);
        return abbrev;
        }
    }
    return QString();
}

/*!
    \since 5.2

    Returns if this datetime falls in Daylight Savings Time.

    If the Qt::TimeSpec is not Qt::LocalTime or Qt::TimeZone then will always
    return false.

    \sa timeSpec()
*/

bool QDateTime::isDaylightTime() const
{
    switch (d->m_spec) {
    case Qt::UTC:
    case Qt::OffsetFromUTC:
        return false;
    case Qt::TimeZone:
#ifndef QT_BOOTSTRAPPED
        return d->m_timeZone.d->isDaylightTime(toMSecsSinceEpoch());
#endif // QT_BOOTSTRAPPED
    case Qt::LocalTime: {
        QDateTimePrivate::DaylightStatus status = d->daylightStatus();
        if (status == QDateTimePrivate::UnknownDaylightTime)
            localMSecsToEpochMSecs(d->m_msecs, 0, 0, &status, 0);
        return (status == QDateTimePrivate::DaylightTime);
        }
    }
    return false;
}

/*!
    Sets the date part of this datetime to \a date.
    If no time is set, it is set to midnight.

    \sa date(), setTime(), setTimeSpec()
*/

void QDateTime::setDate(const QDate &date)
{
    detach();
    d->setDateTime(date, time());
}

/*!
    Sets the time part of this datetime to \a time.

    \sa time(), setDate(), setTimeSpec()
*/

void QDateTime::setTime(const QTime &time)
{
    detach();
    d->setDateTime(date(), time);
}

/*!
    Sets the time specification used in this datetime to \a spec.
    The datetime will refer to a different point in time.

    If \a spec is Qt::OffsetFromUTC then the timeSpec() will be set
    to Qt::UTC, i.e. an effective offset of 0.

    If \a spec is Qt::TimeZone then the spec will be set to Qt::LocalTime,
    i.e. the current system time zone.

    Example:
    \snippet code/src_corelib_tools_qdatetime.cpp 19

    \sa timeSpec(), setDate(), setTime(), setTimeZone(), Qt::TimeSpec
*/

void QDateTime::setTimeSpec(Qt::TimeSpec spec)
{
    detach();
    d->setTimeSpec(spec, 0);
    d->checkValidDateTime();
}

/*!
    \since 5.2

    Sets the timeSpec() to Qt::OffsetFromUTC and the offset to \a offsetSeconds.
    The datetime will refer to a different point in time.

    The maximum and minimum offset is 14 positive or negative hours.  If
    \a offsetSeconds is larger or smaller than that, then the result is
    undefined.

    If \a offsetSeconds is 0 then the timeSpec() will be set to Qt::UTC.

    \sa isValid(), offsetFromUtc()
*/

void QDateTime::setOffsetFromUtc(int offsetSeconds)
{
    detach();
    d->setTimeSpec(Qt::OffsetFromUTC, offsetSeconds);
    d->checkValidDateTime();
}

#ifndef QT_BOOTSTRAPPED
/*!
    \since 5.2

    Sets the time zone used in this datetime to \a toZone.
    The datetime will refer to a different point in time.

    If \a toZone is invalid then the datetime will be invalid.

    \sa timeZone(), Qt::TimeSpec
*/

void QDateTime::setTimeZone(const QTimeZone &toZone)
{
    detach();
    d->m_spec = Qt::TimeZone;
    d->m_offsetFromUtc = 0;
    d->m_timeZone = toZone;
    d->m_status = d->m_status & ~QDateTimePrivate::ValidDateTime & ~QDateTimePrivate::TimeZoneCached;
}
#endif // QT_BOOTSTRAPPED

/*!
    \since 4.7

    Returns the datetime as the number of milliseconds that have passed
    since 1970-01-01T00:00:00.000, Coordinated Universal Time (Qt::UTC).

    On systems that do not support time zones, this function will
    behave as if local time were Qt::UTC.

    The behavior for this function is undefined if the datetime stored in
    this object is not valid. However, for all valid dates, this function
    returns a unique value.

    \sa toTime_t(), setMSecsSinceEpoch()
*/
qint64 QDateTime::toMSecsSinceEpoch() const
{
    d->refreshDateTime();
    return d->toMSecsSinceEpoch();
}

/*!
    Returns the datetime as the number of seconds that have passed
    since 1970-01-01T00:00:00, Coordinated Universal Time (Qt::UTC).

    On systems that do not support time zones, this function will
    behave as if local time were Qt::UTC.

    \note This function returns a 32-bit unsigned integer, so it does not
    support dates before 1970, but it does support dates after
    2038-01-19T03:14:06, which may not be valid time_t values. Be careful
    when passing those time_t values to system functions, which could
    interpret them as negative dates.

    If the date is outside the range 1970-01-01T00:00:00 to
    2106-02-07T06:28:14, this function returns -1 cast to an unsigned integer
    (i.e., 0xFFFFFFFF).

    To get an extended range, use toMSecsSinceEpoch().

    \sa toMSecsSinceEpoch(), setTime_t()
*/

uint QDateTime::toTime_t() const
{
    if (!isValid())
        return uint(-1);
    qint64 retval = d->toMSecsSinceEpoch() / 1000;
    if (quint64(retval) >= Q_UINT64_C(0xFFFFFFFF))
        return uint(-1);
    return uint(retval);
}

/*!
    \since 4.7

    Sets the date and time given the number of milliseconds \a msecs that have
    passed since 1970-01-01T00:00:00.000, Coordinated Universal Time
    (Qt::UTC). On systems that do not support time zones this function
    will behave as if local time were Qt::UTC.

    Note that passing the minimum of \c qint64
    (\c{std::numeric_limits<qint64>::min()}) to \a msecs will result in
    undefined behavior.

    \sa toMSecsSinceEpoch(), setTime_t()
*/
void QDateTime::setMSecsSinceEpoch(qint64 msecs)
{
    detach();

    d->m_status = 0;
    switch (d->m_spec) {
    case Qt::UTC:
        d->m_msecs = msecs;
        d->m_status = d->m_status
                    | QDateTimePrivate::ValidDate
                    | QDateTimePrivate::ValidTime
                    | QDateTimePrivate::ValidDateTime;
        break;
    case Qt::OffsetFromUTC:
        d->m_msecs = msecs + (d->m_offsetFromUtc * 1000);
        d->m_status = d->m_status
                    | QDateTimePrivate::ValidDate
                    | QDateTimePrivate::ValidTime
                    | QDateTimePrivate::ValidDateTime;
        break;
    case Qt::TimeZone:
#ifndef QT_BOOTSTRAPPED
        // Docs state any LocalTime before 1970-01-01 will *not* have any Daylight Time applied
        // but all times afterwards will have Daylight Time applied.
        if (msecs >= 0)
            d->m_offsetFromUtc = d->m_timeZone.d->offsetFromUtc(msecs);
        else
            d->m_offsetFromUtc = d->m_timeZone.d->standardTimeOffset(msecs);
        d->m_msecs = msecs + (d->m_offsetFromUtc * 1000);
        d->m_status = d->m_status
                    | QDateTimePrivate::ValidDate
                    | QDateTimePrivate::ValidTime
                    | QDateTimePrivate::ValidDateTime
                    | QDateTimePrivate::TimeZoneCached;
#endif // QT_BOOTSTRAPPED
        break;
    case Qt::LocalTime: {
        QDate dt;
        QTime tm;
        QDateTimePrivate::DaylightStatus status;
        epochMSecsToLocalTime(msecs, &dt, &tm, &status);
        d->setDateTime(dt, tm);
        d->setDaylightStatus(status);
        break;
        }
    }
}

/*!
    \fn void QDateTime::setTime_t(uint seconds)

    Sets the date and time given the number of \a seconds that have
    passed since 1970-01-01T00:00:00, Coordinated Universal Time
    (Qt::UTC). On systems that do not support time zones this function
    will behave as if local time were Qt::UTC.

    \sa toTime_t()
*/

void QDateTime::setTime_t(uint secsSince1Jan1970UTC)
{
    setMSecsSinceEpoch((qint64)secsSince1Jan1970UTC * 1000);
}

#ifndef QT_NO_DATESTRING
/*!
    \fn QString QDateTime::toString(Qt::DateFormat format) const

    \overload

    Returns the datetime as a string in the \a format given.

    If the \a format is Qt::TextDate, the string is formatted in
    the default way. QDate::shortDayName(), QDate::shortMonthName(),
    and QTime::toString() are used to generate the string, so the
    day and month names will be localized names using the system locale,
    i.e. QLocale::system(). An example of this formatting is
    "Wed May 20 03:40:13 1998".

    If the \a format is Qt::ISODate, the string format corresponds
    to the ISO 8601 extended specification for representations of
    dates and times, taking the form YYYY-MM-DDTHH:MM:SS[Z|[+|-]HH:MM],
    depending on the timeSpec() of the QDateTime. If the timeSpec()
    is Qt::UTC, Z will be appended to the string; if the timeSpec() is
    Qt::OffsetFromUTC, the offset in hours and minutes from UTC will
    be appended to the string.

    If the \a format is Qt::SystemLocaleShortDate or
    Qt::SystemLocaleLongDate, the string format depends on the locale
    settings of the system. Identical to calling
    QLocale::system().toString(datetime, QLocale::ShortFormat) or
    QLocale::system().toString(datetime, QLocale::LongFormat).

    If the \a format is Qt::DefaultLocaleShortDate or
    Qt::DefaultLocaleLongDate, the string format depends on the
    default application locale. This is the locale set with
    QLocale::setDefault(), or the system locale if no default locale
    has been set. Identical to calling QLocale().toString(datetime,
    QLocale::ShortFormat) or QLocale().toString(datetime,
    QLocale::LongFormat).

    If the \a format is Qt::RFC2822Date, the string is formatted
    following \l{RFC 2822}.

    If the datetime is invalid, an empty string will be returned.

    \warning The Qt::ISODate format is only valid for years in the
    range 0 to 9999. This restriction may apply to locale-aware
    formats as well, depending on the locale settings.

    \sa QDate::toString(), QTime::toString(), Qt::DateFormat
*/

QString QDateTime::toString(Qt::DateFormat format) const
{
    QString buf;
    if (!isValid())
        return buf;

    switch (format) {
    case Qt::SystemLocaleDate:
    case Qt::SystemLocaleShortDate:
        return QLocale::system().toString(*this, QLocale::ShortFormat);
    case Qt::SystemLocaleLongDate:
        return QLocale::system().toString(*this, QLocale::LongFormat);
    case Qt::LocaleDate:
    case Qt::DefaultLocaleShortDate:
        return QLocale().toString(*this, QLocale::ShortFormat);
    case Qt::DefaultLocaleLongDate:
        return QLocale().toString(*this, QLocale::LongFormat);
    case Qt::RFC2822Date: {
        buf = QLocale::c().toString(*this, QStringLiteral("dd MMM yyyy hh:mm:ss "));
        buf += toOffsetString(Qt::TextDate, d->m_offsetFromUtc);
        return buf;
    }
    default:
#ifndef QT_NO_TEXTDATE
    case Qt::TextDate: {
        QDate dt;
        QTime tm;
        d->getDateTime(&dt, &tm);
        //We cant use date.toString(Qt::TextDate) as we need to insert the time before the year
        buf = QString::fromUtf8("%1 %2 %3 %4 %5").arg(dt.shortDayName(dt.dayOfWeek()))
                                                 .arg(dt.shortMonthName(dt.month()))
                                                 .arg(dt.day())
                                                 .arg(tm.toString(Qt::TextDate))
                                                 .arg(dt.year());
        if (timeSpec() != Qt::LocalTime) {
            buf += QStringLiteral(" GMT");
            if (d->m_spec == Qt::OffsetFromUTC)
                buf += toOffsetString(Qt::TextDate, d->m_offsetFromUtc);
        }
        return buf;
    }
#endif
    case Qt::ISODate: {
        QDate dt;
        QTime tm;
        d->getDateTime(&dt, &tm);
        buf = dt.toString(Qt::ISODate);
        if (buf.isEmpty())
            return QString();   // failed to convert
        buf += QLatin1Char('T');
        buf += tm.toString(Qt::ISODate);
        switch (d->m_spec) {
        case Qt::UTC:
            buf += QLatin1Char('Z');
            break;
        case Qt::OffsetFromUTC:
            buf += toOffsetString(Qt::ISODate, d->m_offsetFromUtc);
            break;
        default:
            break;
        }
        return buf;
    }
    }
}

/*!
    Returns the datetime as a string. The \a format parameter
    determines the format of the result string.

    These expressions may be used for the date:

    \table
    \header \li Expression \li Output
    \row \li d \li the day as number without a leading zero (1 to 31)
    \row \li dd \li the day as number with a leading zero (01 to 31)
    \row \li ddd
            \li the abbreviated localized day name (e.g. 'Mon' to 'Sun').
            Uses the system locale to localize the name, i.e. QLocale::system().
    \row \li dddd
            \li the long localized day name (e.g. 'Monday' to 'Qt::Sunday').
            Uses the system locale to localize the name, i.e. QLocale::system().
    \row \li M \li the month as number without a leading zero (1-12)
    \row \li MM \li the month as number with a leading zero (01-12)
    \row \li MMM
            \li the abbreviated localized month name (e.g. 'Jan' to 'Dec').
            Uses the system locale to localize the name, i.e. QLocale::system().
    \row \li MMMM
            \li the long localized month name (e.g. 'January' to 'December').
            Uses the system locale to localize the name, i.e. QLocale::system().
    \row \li yy \li the year as two digit number (00-99)
    \row \li yyyy \li the year as four digit number
    \endtable

    These expressions may be used for the time:

    \table
    \header \li Expression \li Output
    \row \li h
         \li the hour without a leading zero (0 to 23 or 1 to 12 if AM/PM display)
    \row \li hh
         \li the hour with a leading zero (00 to 23 or 01 to 12 if AM/PM display)
    \row \li H
         \li the hour without a leading zero (0 to 23, even with AM/PM display)
    \row \li HH
         \li the hour with a leading zero (00 to 23, even with AM/PM display)
    \row \li m \li the minute without a leading zero (0 to 59)
    \row \li mm \li the minute with a leading zero (00 to 59)
    \row \li s \li the second without a leading zero (0 to 59)
    \row \li ss \li the second with a leading zero (00 to 59)
    \row \li z \li the milliseconds without leading zeroes (0 to 999)
    \row \li zzz \li the milliseconds with leading zeroes (000 to 999)
    \row \li AP or A
         \li use AM/PM display. \e A/AP will be replaced by either "AM" or "PM".
    \row \li ap or a
         \li use am/pm display. \e a/ap will be replaced by either "am" or "pm".
    \row \li t \li the timezone (for example "CEST")
    \endtable

    All other input characters will be ignored. Any sequence of characters that
    are enclosed in single quotes will be treated as text and not be used as an
    expression. Two consecutive single quotes ("''") are replaced by a singlequote
    in the output. Formats without separators (e.g. "HHmm") are currently not supported.

    Example format strings (assumed that the QDateTime is 21 May 2001
    14:13:09):

    \table
    \header \li Format       \li Result
    \row \li dd.MM.yyyy      \li 21.05.2001
    \row \li ddd MMMM d yy   \li Tue May 21 01
    \row \li hh:mm:ss.zzz    \li 14:13:09.042
    \row \li h:m:s ap        \li 2:13:9 pm
    \endtable

    If the datetime is invalid, an empty string will be returned.

    \sa QDate::toString(), QTime::toString(), QLocale::toString()
*/
QString QDateTime::toString(const QString& format) const
{
    return QLocale::system().toString(*this, format);
}
#endif //QT_NO_DATESTRING

/*!
    Returns a QDateTime object containing a datetime \a ndays days
    later than the datetime of this object (or earlier if \a ndays is
    negative).

    If the timeSpec() is Qt::LocalTime and the resulting
    date and time fall in the Standard Time to Daylight Time transition
    hour then the result will be adjusted accordingly, i.e. if the transition
    is at 2am and the clock goes forward to 3am and the result falls between
    2am and 3am then the result will be adjusted to fall after 3am.

    \sa daysTo(), addMonths(), addYears(), addSecs()
*/

QDateTime QDateTime::addDays(qint64 ndays) const
{
    QDateTime dt(*this);
    dt.detach();
    QDate date;
    QTime time;
    d->getDateTime(&date, &time);
    date = date.addDays(ndays);
    // Result might fall into "missing" DaylightTime transition hour,
    // so call conversion and use the adjusted returned time
    if (d->m_spec == Qt::LocalTime)
        localMSecsToEpochMSecs(timeToMSecs(date, time), &date, &time);
#ifndef QT_BOOTSTRAPPED
    else if (d->m_spec == Qt::TimeZone)
        d->zoneMSecsToEpochMSecs(timeToMSecs(date, time), d->m_timeZone, &date, &time);
#endif // QT_BOOTSTRAPPED
    dt.d->setDateTime(date, time);
    return dt;
}

/*!
    Returns a QDateTime object containing a datetime \a nmonths months
    later than the datetime of this object (or earlier if \a nmonths
    is negative).

    If the timeSpec() is Qt::LocalTime and the resulting
    date and time fall in the Standard Time to Daylight Time transition
    hour then the result will be adjusted accordingly, i.e. if the transition
    is at 2am and the clock goes forward to 3am and the result falls between
    2am and 3am then the result will be adjusted to fall after 3am.

    \sa daysTo(), addDays(), addYears(), addSecs()
*/

QDateTime QDateTime::addMonths(int nmonths) const
{
    QDateTime dt(*this);
    dt.detach();
    QDate date;
    QTime time;
    d->getDateTime(&date, &time);
    date = date.addMonths(nmonths);
    // Result might fall into "missing" DaylightTime transition hour,
    // so call conversion and use the adjusted returned time
    if (d->m_spec == Qt::LocalTime)
        localMSecsToEpochMSecs(timeToMSecs(date, time), &date, &time);
#ifndef QT_BOOTSTRAPPED
    else if (d->m_spec == Qt::TimeZone)
        d->zoneMSecsToEpochMSecs(timeToMSecs(date, time), d->m_timeZone, &date, &time);
#endif // QT_BOOTSTRAPPED
    dt.d->setDateTime(date, time);
    return dt;
}

/*!
    Returns a QDateTime object containing a datetime \a nyears years
    later than the datetime of this object (or earlier if \a nyears is
    negative).

    If the timeSpec() is Qt::LocalTime and the resulting
    date and time fall in the Standard Time to Daylight Time transition
    hour then the result will be adjusted accordingly, i.e. if the transition
    is at 2am and the clock goes forward to 3am and the result falls between
    2am and 3am then the result will be adjusted to fall after 3am.

    \sa daysTo(), addDays(), addMonths(), addSecs()
*/

QDateTime QDateTime::addYears(int nyears) const
{
    QDateTime dt(*this);
    dt.detach();
    QDate date;
    QTime time;
    d->getDateTime(&date, &time);
    date = date.addYears(nyears);
    // Result might fall into "missing" DaylightTime transition hour,
    // so call conversion and use the adjusted returned time
    if (d->m_spec == Qt::LocalTime)
        localMSecsToEpochMSecs(timeToMSecs(date, time), &date, &time);
#ifndef QT_BOOTSTRAPPED
    else if (d->m_spec == Qt::TimeZone)
        d->zoneMSecsToEpochMSecs(timeToMSecs(date, time), d->m_timeZone, &date, &time);
#endif // QT_BOOTSTRAPPED
    dt.d->setDateTime(date, time);
    return dt;
}

/*!
    Returns a QDateTime object containing a datetime \a s seconds
    later than the datetime of this object (or earlier if \a s is
    negative).

    If this datetime is invalid, an invalid datetime will be returned.

    \sa addMSecs(), secsTo(), addDays(), addMonths(), addYears()
*/

QDateTime QDateTime::addSecs(qint64 s) const
{
    return addMSecs(s * 1000);
}

/*!
    Returns a QDateTime object containing a datetime \a msecs miliseconds
    later than the datetime of this object (or earlier if \a msecs is
    negative).

    If this datetime is invalid, an invalid datetime will be returned.

    \sa addSecs(), msecsTo(), addDays(), addMonths(), addYears()
*/
QDateTime QDateTime::addMSecs(qint64 msecs) const
{
    if (!isValid())
        return QDateTime();

    QDateTime dt(*this);
    dt.detach();
    if (d->m_spec == Qt::LocalTime || d->m_spec == Qt::TimeZone)
        // Convert to real UTC first in case crosses daylight transition
        dt.setMSecsSinceEpoch(d->toMSecsSinceEpoch() + msecs);
    else
        // No need to convert, just add on
        dt.d->m_msecs = dt.d->m_msecs + msecs;
    return dt;
}

/*!
    Returns the number of days from this datetime to the \a other
    datetime. The number of days is counted as the number of times
    midnight is reached between this datetime to the \a other
    datetime. This means that a 10 minute difference from 23:55 to
    0:05 the next day counts as one day.

    If the \a other datetime is earlier than this datetime,
    the value returned is negative.

    Example:
    \snippet code/src_corelib_tools_qdatetime.cpp 15

    \sa addDays(), secsTo(), msecsTo()
*/

qint64 QDateTime::daysTo(const QDateTime &other) const
{
    return date().daysTo(other.date());
}

/*!
    Returns the number of seconds from this datetime to the \a other
    datetime. If the \a other datetime is earlier than this datetime,
    the value returned is negative.

    Before performing the comparison, the two datetimes are converted
    to Qt::UTC to ensure that the result is correct if one of the two
    datetimes has daylight saving time (DST) and the other doesn't.

    Returns 0 if either datetime is invalid.

    Example:
    \snippet code/src_corelib_tools_qdatetime.cpp 11

    \sa addSecs(), daysTo(), QTime::secsTo()
*/

qint64 QDateTime::secsTo(const QDateTime &other) const
{
    return (msecsTo(other) / 1000);
}

/*!
    Returns the number of milliseconds from this datetime to the \a other
    datetime. If the \a other datetime is earlier than this datetime,
    the value returned is negative.

    Before performing the comparison, the two datetimes are converted
    to Qt::UTC to ensure that the result is correct if one of the two
    datetimes has daylight saving time (DST) and the other doesn't.

    Returns 0 if either datetime is invalid.

    \sa addMSecs(), daysTo(), QTime::msecsTo()
*/

qint64 QDateTime::msecsTo(const QDateTime &other) const
{
    if (!isValid() || !other.isValid())
        return 0;

    return other.d->toMSecsSinceEpoch() - d->toMSecsSinceEpoch();
}

/*!
    \fn QDateTime QDateTime::toTimeSpec(Qt::TimeSpec spec) const

    Returns a copy of this datetime converted to the given time
    \a spec.

    If \a spec is Qt::OffsetFromUTC then it is set to Qt::UTC.  To set to a
    spec of Qt::OffsetFromUTC use toOffsetFromUtc().

    If \a spec is Qt::TimeZone then it is set to Qt::LocalTime,
    i.e. the local Time Zone.

    Example:
    \snippet code/src_corelib_tools_qdatetime.cpp 16

    \sa timeSpec(), toTimeZone(), toUTC(), toLocalTime()
*/

QDateTime QDateTime::toTimeSpec(Qt::TimeSpec spec) const
{
    if (d->m_spec == spec && (spec == Qt::UTC || spec == Qt::LocalTime))
        return *this;

    if (!isValid()) {
        QDateTime ret = *this;
        ret.setTimeSpec(spec);
        return ret;
    }

    return fromMSecsSinceEpoch(d->toMSecsSinceEpoch(), spec, 0);
}

/*!
    \since 5.2

    \fn QDateTime QDateTime::toOffsetFromUtc(int offsetSeconds) const

    Returns a copy of this datetime converted to a spec of Qt::OffsetFromUTC
    with the given \a offsetSeconds.

    If the \a offsetSeconds equals 0 then a UTC datetime will be returned

    \sa setOffsetFromUtc(), offsetFromUtc(), toTimeSpec()
*/

QDateTime QDateTime::toOffsetFromUtc(int offsetSeconds) const
{
    if (d->m_spec == Qt::OffsetFromUTC && d->m_offsetFromUtc == offsetSeconds)
        return *this;

    if (!isValid()) {
        QDateTime ret = *this;
        ret.setOffsetFromUtc(offsetSeconds);
        return ret;
    }

    return fromMSecsSinceEpoch(d->toMSecsSinceEpoch(), Qt::OffsetFromUTC, offsetSeconds);
}

#ifndef QT_BOOTSTRAPPED
/*!
    \since 5.2

    Returns a copy of this datetime converted to the given \a timeZone

    \sa timeZone(), toTimeSpec()
*/

QDateTime QDateTime::toTimeZone(const QTimeZone &timeZone) const
{
    if (d->m_spec == Qt::TimeZone && d->m_timeZone == timeZone)
        return *this;

    if (!isValid()) {
        QDateTime ret = *this;
        ret.setTimeZone(timeZone);
        return ret;
    }

    return fromMSecsSinceEpoch(d->toMSecsSinceEpoch(), timeZone);
}
#endif // QT_BOOTSTRAPPED

/*!
    Returns \c true if this datetime is equal to the \a other datetime;
    otherwise returns \c false.

    \sa operator!=()
*/

bool QDateTime::operator==(const QDateTime &other) const
{
    if (d->m_spec == Qt::LocalTime
        && other.d->m_spec == Qt::LocalTime
        && d->m_status == other.d->m_status) {
        return (d->m_msecs == other.d->m_msecs);
    }
    // Convert to UTC and compare
    return (toMSecsSinceEpoch() == other.toMSecsSinceEpoch());
}

/*!
    \fn bool QDateTime::operator!=(const QDateTime &other) const

    Returns \c true if this datetime is different from the \a other
    datetime; otherwise returns \c false.

    Two datetimes are different if either the date, the time, or the
    time zone components are different.

    \sa operator==()
*/

/*!
    Returns \c true if this datetime is earlier than the \a other
    datetime; otherwise returns \c false.
*/

bool QDateTime::operator<(const QDateTime &other) const
{
    if (d->m_spec == Qt::LocalTime
        && other.d->m_spec == Qt::LocalTime
        && d->m_status == other.d->m_status) {
        return (d->m_msecs < other.d->m_msecs);
    }
    // Convert to UTC and compare
    return (toMSecsSinceEpoch() < other.toMSecsSinceEpoch());
}

/*!
    \fn bool QDateTime::operator<=(const QDateTime &other) const

    Returns \c true if this datetime is earlier than or equal to the
    \a other datetime; otherwise returns \c false.
*/

/*!
    \fn bool QDateTime::operator>(const QDateTime &other) const

    Returns \c true if this datetime is later than the \a other datetime;
    otherwise returns \c false.
*/

/*!
    \fn bool QDateTime::operator>=(const QDateTime &other) const

    Returns \c true if this datetime is later than or equal to the
    \a other datetime; otherwise returns \c false.
*/

/*!
    \fn QDateTime QDateTime::currentDateTime()
    Returns the current datetime, as reported by the system clock, in
    the local time zone.

    \sa currentDateTimeUtc(), QDate::currentDate(), QTime::currentTime(), toTimeSpec()
*/

/*!
    \fn QDateTime QDateTime::currentDateTimeUtc()
    \since 4.7
    Returns the current datetime, as reported by the system clock, in
    UTC.

    \sa currentDateTime(), QDate::currentDate(), QTime::currentTime(), toTimeSpec()
*/

/*!
    \fn qint64 QDateTime::currentMSecsSinceEpoch()
    \since 4.7

    Returns the number of milliseconds since 1970-01-01T00:00:00 Universal
    Coordinated Time. This number is like the POSIX time_t variable, but
    expressed in milliseconds instead.

    \sa currentDateTime(), currentDateTimeUtc(), toTime_t(), toTimeSpec()
*/

#if defined(Q_OS_WIN)
static inline uint msecsFromDecomposed(int hour, int minute, int sec, int msec = 0)
{
    return MSECS_PER_HOUR * hour + MSECS_PER_MIN * minute + 1000 * sec + msec;
}

QDate QDate::currentDate()
{
    QDate d;
    SYSTEMTIME st;
    memset(&st, 0, sizeof(SYSTEMTIME));
    GetLocalTime(&st);
    d.jd = julianDayFromDate(st.wYear, st.wMonth, st.wDay);
    return d;
}

QTime QTime::currentTime()
{
    QTime ct;
    SYSTEMTIME st;
    memset(&st, 0, sizeof(SYSTEMTIME));
    GetLocalTime(&st);
    ct.setHMS(st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
#if defined(Q_OS_WINCE)
    ct.startTick = GetTickCount() % MSECS_PER_DAY;
#endif
    return ct;
}

QDateTime QDateTime::currentDateTime()
{
    QDate d;
    QTime t;
    SYSTEMTIME st;
    memset(&st, 0, sizeof(SYSTEMTIME));
    GetLocalTime(&st);
    d.jd = julianDayFromDate(st.wYear, st.wMonth, st.wDay);
    t.mds = msecsFromDecomposed(st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
    return QDateTime(d, t);
}

QDateTime QDateTime::currentDateTimeUtc()
{
    QDate d;
    QTime t;
    SYSTEMTIME st;
    memset(&st, 0, sizeof(SYSTEMTIME));
    GetSystemTime(&st);
    d.jd = julianDayFromDate(st.wYear, st.wMonth, st.wDay);
    t.mds = msecsFromDecomposed(st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
    return QDateTime(d, t, Qt::UTC);
}

qint64 QDateTime::currentMSecsSinceEpoch() Q_DECL_NOTHROW
{
    QDate d;
    QTime t;
    SYSTEMTIME st;
    memset(&st, 0, sizeof(SYSTEMTIME));
    GetSystemTime(&st);

    return msecsFromDecomposed(st.wHour, st.wMinute, st.wSecond, st.wMilliseconds) +
            qint64(julianDayFromDate(st.wYear, st.wMonth, st.wDay)
                   - julianDayFromDate(1970, 1, 1)) * Q_INT64_C(86400000);
}

#elif defined(Q_OS_UNIX)
QDate QDate::currentDate()
{
    return QDateTime::currentDateTime().date();
}

QTime QTime::currentTime()
{
    return QDateTime::currentDateTime().time();
}

QDateTime QDateTime::currentDateTime()
{
    return fromMSecsSinceEpoch(currentMSecsSinceEpoch(), Qt::LocalTime);
}

QDateTime QDateTime::currentDateTimeUtc()
{
    return fromMSecsSinceEpoch(currentMSecsSinceEpoch(), Qt::UTC);
}

qint64 QDateTime::currentMSecsSinceEpoch() Q_DECL_NOTHROW
{
    // posix compliant system
    // we have milliseconds
    struct timeval tv;
    gettimeofday(&tv, 0);
    return qint64(tv.tv_sec) * Q_INT64_C(1000) + tv.tv_usec / 1000;
}

#else
#error "What system is this?"
#endif

/*!
  \since 4.2

  Returns a datetime whose date and time are the number of \a seconds
  that have passed since 1970-01-01T00:00:00, Coordinated Universal
  Time (Qt::UTC) and converted to Qt::LocalTime.  On systems that do not
  support time zones, the time will be set as if local time were Qt::UTC.

  \sa toTime_t(), setTime_t()
*/
QDateTime QDateTime::fromTime_t(uint seconds)
{
    return fromMSecsSinceEpoch((qint64)seconds * 1000, Qt::LocalTime);
}

/*!
  \since 5.2

  Returns a datetime whose date and time are the number of \a seconds
  that have passed since 1970-01-01T00:00:00, Coordinated Universal
  Time (Qt::UTC) and converted to the given \a spec.

  If the \a spec is not Qt::OffsetFromUTC then the \a offsetSeconds will be
  ignored.  If the \a spec is Qt::OffsetFromUTC and the \a offsetSeconds is 0
  then the spec will be set to Qt::UTC, i.e. an offset of 0 seconds.

  \sa toTime_t(), setTime_t()
*/
QDateTime QDateTime::fromTime_t(uint seconds, Qt::TimeSpec spec, int offsetSeconds)
{
    return fromMSecsSinceEpoch((qint64)seconds * 1000, spec, offsetSeconds);
}

#ifndef QT_BOOTSTRAPPED
/*!
    \since 5.2

    Returns a datetime whose date and time are the number of \a seconds
    that have passed since 1970-01-01T00:00:00, Coordinated Universal
    Time (Qt::UTC) and with the given \a timeZone.

    \sa toTime_t(), setTime_t()
*/
QDateTime QDateTime::fromTime_t(uint seconds, const QTimeZone &timeZone)
{
    return fromMSecsSinceEpoch((qint64)seconds * 1000, timeZone);
}
#endif

/*!
  \since 4.7

  Returns a datetime whose date and time are the number of milliseconds, \a msecs,
  that have passed since 1970-01-01T00:00:00.000, Coordinated Universal
  Time (Qt::UTC), and converted to Qt::LocalTime.  On systems that do not
  support time zones, the time will be set as if local time were Qt::UTC.

  Note that there are possible values for \a msecs that lie outside the valid
  range of QDateTime, both negative and positive. The behavior of this
  function is undefined for those values.

  \sa toTime_t(), setTime_t()
*/
QDateTime QDateTime::fromMSecsSinceEpoch(qint64 msecs)
{
    return fromMSecsSinceEpoch(msecs, Qt::LocalTime);
}

/*!
  \since 5.2

  Returns a datetime whose date and time are the number of milliseconds \a msecs
  that have passed since 1970-01-01T00:00:00.000, Coordinated Universal
  Time (Qt::UTC) and converted to the given \a spec.

  Note that there are possible values for \a msecs that lie outside the valid
  range of QDateTime, both negative and positive. The behavior of this
  function is undefined for those values.

  If the \a spec is not Qt::OffsetFromUTC then the \a offsetSeconds will be
  ignored.  If the \a spec is Qt::OffsetFromUTC and the \a offsetSeconds is 0
  then the spec will be set to Qt::UTC, i.e. an offset of 0 seconds.

  If \a spec is Qt::TimeZone then the spec will be set to Qt::LocalTime,
  i.e. the current system time zone.

  \sa fromTime_t()
*/
QDateTime QDateTime::fromMSecsSinceEpoch(qint64 msecs, Qt::TimeSpec spec, int offsetSeconds)
{
    QDateTime dt;
    dt.detach();
    dt.d->setTimeSpec(spec, offsetSeconds);
    dt.setMSecsSinceEpoch(msecs);
    return dt;
}

#ifndef QT_BOOTSTRAPPED
/*!
    \since 5.2

    Returns a datetime whose date and time are the number of milliseconds \a msecs
    that have passed since 1970-01-01T00:00:00.000, Coordinated Universal
    Time (Qt::UTC) and with the given \a timeZone.

    \sa fromTime_t()
*/
QDateTime QDateTime::fromMSecsSinceEpoch(qint64 msecs, const QTimeZone &timeZone)
{
    QDateTime dt;
    dt.setTimeZone(timeZone);
    dt.setMSecsSinceEpoch(msecs);
    return dt;
}
#endif

#if QT_DEPRECATED_SINCE(5, 2)
/*!
    \since 4.4
    \internal
    \obsolete

    This method was added in 4.4 but never documented as public. It was replaced
    in 5.2 with public method setOffsetFromUtc() for consistency with QTimeZone.

    This method should never be made public.

    \sa setOffsetFromUtc()
 */
void QDateTime::setUtcOffset(int seconds)
{
    setOffsetFromUtc(seconds);
}

/*!
    \since 4.4
    \internal
    \obsolete

    This method was added in 4.4 but never documented as public. It was replaced
    in 5.1 with public method offsetFromUTC() for consistency with QTimeZone.

    This method should never be made public.

    \sa offsetFromUTC()
*/
int QDateTime::utcOffset() const
{
    return offsetFromUtc();
}
#endif // QT_DEPRECATED_SINCE

#ifndef QT_NO_DATESTRING

static int fromShortMonthName(const QString &monthName)
{
    // Assume that English monthnames are the default
    for (int i = 0; i < 12; ++i) {
        if (monthName == QLatin1String(qt_shortMonthNames[i]))
            return i + 1;
    }
    // If English names can't be found, search the localized ones
    for (int i = 1; i <= 12; ++i) {
        if (monthName == QDate::shortMonthName(i))
            return i;
    }
    return -1;
}

/*!
    \fn QDateTime QDateTime::fromString(const QString &string, Qt::DateFormat format)

    Returns the QDateTime represented by the \a string, using the
    \a format given, or an invalid datetime if this is not possible.

    Note for Qt::TextDate: It is recommended that you use the
    English short month names (e.g. "Jan"). Although localized month
    names can also be used, they depend on the user's locale settings.
*/
QDateTime QDateTime::fromString(const QString& string, Qt::DateFormat format)
{
    if (string.isEmpty())
        return QDateTime();

    switch (format) {
    case Qt::SystemLocaleDate:
    case Qt::SystemLocaleShortDate:
        return QLocale::system().toDateTime(string, QLocale::ShortFormat);
    case Qt::SystemLocaleLongDate:
        return QLocale::system().toDateTime(string, QLocale::LongFormat);
    case Qt::LocaleDate:
    case Qt::DefaultLocaleShortDate:
        return QLocale().toDateTime(string, QLocale::ShortFormat);
    case Qt::DefaultLocaleLongDate:
        return QLocale().toDateTime(string, QLocale::LongFormat);
    case Qt::RFC2822Date: {
        QDate date;
        QTime time;
        int utcOffset = 0;
        rfcDateImpl(string, &date, &time, &utcOffset);

        if (!date.isValid() || !time.isValid())
            return QDateTime();

        QDateTime dateTime(date, time, Qt::UTC);
        dateTime.setOffsetFromUtc(utcOffset);
        return dateTime;
    }
    case Qt::ISODate: {
        const int size = string.size();
        if (size < 10)
            return QDateTime();

        QStringRef isoString(&string);
        Qt::TimeSpec spec = Qt::LocalTime;

        QDate date = QDate::fromString(string.left(10), Qt::ISODate);
        if (!date.isValid())
            return QDateTime();
        if (size == 10)
            return QDateTime(date);

        isoString = isoString.right(11);
        int offset = 0;
        // Check end of string for Time Zone definition, either Z for UTC or [+-]HH:MM for Offset
        if (isoString.endsWith(QLatin1Char('Z'))) {
            spec = Qt::UTC;
            isoString = isoString.left(isoString.size() - 1);
        } else {
            // the loop below is faster but functionally equal to:
            // const int signIndex = isoString.indexOf(QRegExp(QStringLiteral("[+-]")));
            const int sizeOfTimeZoneString = 4;
            int signIndex = isoString.size() - sizeOfTimeZoneString - 1;
            bool found = false;
            {
                const QChar plus = QLatin1Char('+');
                const QChar minus = QLatin1Char('-');
                do {
                    QChar character(isoString.at(signIndex));
                    found = character == plus || character == minus;
                } while (--signIndex >= 0 && !found);
                ++signIndex;
            }

            if (found) {
                bool ok;
                offset = fromOffsetString(isoString.mid(signIndex).toString(), &ok);
                if (!ok)
                    return QDateTime();
                isoString = isoString.left(signIndex);
                spec = Qt::OffsetFromUTC;
            }
        }

        // Might be end of day (24:00, including variants), which QTime considers invalid.
        // ISO 8601 (section 4.2.3) says that 24:00 is equivalent to 00:00 the next day.
        bool isMidnight24 = false;
        QTime time = fromIsoTimeString(isoString, Qt::ISODate, &isMidnight24);
        if (!time.isValid())
            return QDateTime();
        if (isMidnight24)
            date = date.addDays(1);
        return QDateTime(date, time, spec, offset);
    }
#if !defined(QT_NO_TEXTDATE)
    case Qt::TextDate: {
        QStringList parts = string.split(QLatin1Char(' '), QString::SkipEmptyParts);

        if ((parts.count() < 5) || (parts.count() > 6))
            return QDateTime();

        // Accept "Sun Dec 1 13:02:00 1974" and "Sun 1. Dec 13:02:00 1974"
        int month = 0;
        int day = 0;
        bool ok = false;

        // First try month then day
        month = fromShortMonthName(parts.at(1));
        if (month)
            day = parts.at(2).toInt();

        // If failed try day then month
        if (!month || !day) {
            month = fromShortMonthName(parts.at(2));
            if (month) {
                QString dayStr = parts.at(1);
                if (dayStr.endsWith(QLatin1Char('.'))) {
                    dayStr.chop(1);
                    day = dayStr.toInt();
                }
            }
        }

        // If both failed, give up
        if (!month || !day)
            return QDateTime();

        // Year can be before or after time, "Sun Dec 1 1974 13:02:00" or "Sun Dec 1 13:02:00 1974"
        // Guess which by looking for ':' in the time
        int year = 0;
        int yearPart = 0;
        int timePart = 0;
        if (parts.at(3).contains(QLatin1Char(':'))) {
            yearPart = 4;
            timePart = 3;
        } else if (parts.at(4).contains(QLatin1Char(':'))) {
            yearPart = 3;
            timePart = 4;
        } else {
            return QDateTime();
        }

        year = parts.at(yearPart).toInt(&ok);
        if (!ok)
            return QDateTime();

        QDate date(year, month, day);
        if (!date.isValid())
            return QDateTime();

        QStringList timeParts = parts.at(timePart).split(QLatin1Char(':'));
        if (timeParts.count() < 2 || timeParts.count() > 3)
            return QDateTime();

        int hour = timeParts.at(0).toInt(&ok);
        if (!ok)
            return QDateTime();

        int minute = timeParts.at(1).toInt(&ok);
        if (!ok)
            return QDateTime();

        int second = 0;
        int millisecond = 0;
        if (timeParts.count() > 2) {
            QStringList secondParts = timeParts.at(2).split(QLatin1Char('.'));
            if (secondParts.size() > 2) {
                return QDateTime();
            }

            second = secondParts.first().toInt(&ok);
            if (!ok) {
                return QDateTime();
            }

            if (secondParts.size() > 1) {
                millisecond = secondParts.last().toInt(&ok);
                if (!ok) {
                    return QDateTime();
                }
            }
        }

        QTime time(hour, minute, second, millisecond);
        if (!time.isValid())
            return QDateTime();

        if (parts.count() == 5)
            return QDateTime(date, time, Qt::LocalTime);

        QString tz = parts.at(5);
        if (!tz.startsWith(QLatin1String("GMT"), Qt::CaseInsensitive))
            return QDateTime();
        tz.remove(0, 3);
        if (!tz.isEmpty()) {
            int offset = fromOffsetString(tz, &ok);
            if (!ok)
                return QDateTime();
            return QDateTime(date, time, Qt::OffsetFromUTC, offset);
        } else {
            return QDateTime(date, time, Qt::UTC);
        }
    }
#endif //QT_NO_TEXTDATE
    }

    return QDateTime();
}

/*!
    \fn QDateTime::fromString(const QString &string, const QString &format)

    Returns the QDateTime represented by the \a string, using the \a
    format given, or an invalid datetime if the string cannot be parsed.

    These expressions may be used for the date part of the format string:

    \table
    \header \li Expression \li Output
    \row \li d \li the day as number without a leading zero (1 to 31)
    \row \li dd \li the day as number with a leading zero (01 to 31)
    \row \li ddd
            \li the abbreviated localized day name (e.g. 'Mon' to 'Sun').
            Uses QDate::shortDayName().
    \row \li dddd
            \li the long localized day name (e.g. 'Monday' to 'Sunday').
            Uses QDate::longDayName().
    \row \li M \li the month as number without a leading zero (1-12)
    \row \li MM \li the month as number with a leading zero (01-12)
    \row \li MMM
            \li the abbreviated localized month name (e.g. 'Jan' to 'Dec').
            Uses QDate::shortMonthName().
    \row \li MMMM
            \li the long localized month name (e.g. 'January' to 'December').
            Uses QDate::longMonthName().
    \row \li yy \li the year as two digit number (00-99)
    \row \li yyyy \li the year as four digit number
    \endtable

    \note Unlike the other version of this function, day and month names must
    be given in the user's local language. It is only possible to use the English
    names if the user's language is English.

    These expressions may be used for the time part of the format string:

    \table
    \header \li Expression \li Output
    \row \li h
            \li the hour without a leading zero (0 to 23 or 1 to 12 if AM/PM display)
    \row \li hh
            \li the hour with a leading zero (00 to 23 or 01 to 12 if AM/PM display)
    \row \li H
            \li the hour without a leading zero (0 to 23, even with AM/PM display)
    \row \li HH
            \li the hour with a leading zero (00 to 23, even with AM/PM display)
    \row \li m \li the minute without a leading zero (0 to 59)
    \row \li mm \li the minute with a leading zero (00 to 59)
    \row \li s \li the second without a leading zero (0 to 59)
    \row \li ss \li the second with a leading zero (00 to 59)
    \row \li z \li the milliseconds without leading zeroes (0 to 999)
    \row \li zzz \li the milliseconds with leading zeroes (000 to 999)
    \row \li AP or A
         \li interpret as an AM/PM time. \e AP must be either "AM" or "PM".
    \row \li ap or a
         \li Interpret as an AM/PM time. \e ap must be either "am" or "pm".
    \endtable

    All other input characters will be treated as text. Any sequence
    of characters that are enclosed in single quotes will also be
    treated as text and not be used as an expression.

    \snippet code/src_corelib_tools_qdatetime.cpp 12

    If the format is not satisfied, an invalid QDateTime is returned.
    The expressions that don't have leading zeroes (d, M, h, m, s, z) will be
    greedy. This means that they will use two digits even if this will
    put them outside the range and/or leave too few digits for other
    sections.

    \snippet code/src_corelib_tools_qdatetime.cpp 13

    This could have meant 1 January 00:30.00 but the M will grab
    two digits.

    Incorrectly specified fields of the \a string will cause an invalid
    QDateTime to be returned. For example, consider the following code,
    where the two digit year 12 is read as 1912 (see the table below for all
    field defaults); the resulting datetime is invalid because 23 April 1912
    was a Tuesday, not a Monday:

    \snippet code/src_corelib_tools_qdatetime.cpp 20

    The correct code is:

    \snippet code/src_corelib_tools_qdatetime.cpp 21

    For any field that is not represented in the format, the following
    defaults are used:

    \table
    \header \li Field  \li Default value
    \row    \li Year   \li 1900
    \row    \li Month  \li 1 (January)
    \row    \li Day    \li 1
    \row    \li Hour   \li 0
    \row    \li Minute \li 0
    \row    \li Second \li 0
    \endtable

    For example:

    \snippet code/src_corelib_tools_qdatetime.cpp 14

    \sa QDate::fromString(), QTime::fromString(), QDate::toString(),
    QDateTime::toString(), QTime::toString()
*/

QDateTime QDateTime::fromString(const QString &string, const QString &format)
{
#ifndef QT_BOOTSTRAPPED
    QTime time;
    QDate date;

    QDateTimeParser dt(QVariant::DateTime, QDateTimeParser::FromString);
    if (dt.parseFormat(format) && dt.fromString(string, &date, &time))
        return QDateTime(date, time);
#else
    Q_UNUSED(string);
    Q_UNUSED(format);
#endif
    return QDateTime(QDate(), QTime(-1, -1, -1));
}

#endif // QT_NO_DATESTRING
/*!
    \fn QDateTime QDateTime::toLocalTime() const

    Returns a datetime containing the date and time information in
    this datetime, but specified using the Qt::LocalTime definition.

    Example:

    \snippet code/src_corelib_tools_qdatetime.cpp 17

    \sa toTimeSpec()
*/

/*!
    \fn QDateTime QDateTime::toUTC() const

    Returns a datetime containing the date and time information in
    this datetime, but specified using the Qt::UTC definition.

    Example:

    \snippet code/src_corelib_tools_qdatetime.cpp 18

    \sa toTimeSpec()
*/

/*!
    \internal
 */
void QDateTime::detach()
{
    d.detach();
}

/*****************************************************************************
  Date/time stream functions
 *****************************************************************************/

#ifndef QT_NO_DATASTREAM
/*!
    \relates QDate

    Writes the \a date to stream \a out.

    \sa {Serializing Qt Data Types}
*/

QDataStream &operator<<(QDataStream &out, const QDate &date)
{
    if (out.version() < QDataStream::Qt_5_0)
        return out << quint32(date.jd);
    else
        return out << qint64(date.jd);
}

/*!
    \relates QDate

    Reads a date from stream \a in into the \a date.

    \sa {Serializing Qt Data Types}
*/

QDataStream &operator>>(QDataStream &in, QDate &date)
{
    if (in.version() < QDataStream::Qt_5_0) {
        quint32 jd;
        in >> jd;
        // Older versions consider 0 an invalid jd.
        date.jd = (jd != 0 ? jd : QDate::nullJd());
    } else {
        qint64 jd;
        in >> jd;
        date.jd = jd;
    }

    return in;
}

/*!
    \relates QTime

    Writes \a time to stream \a out.

    \sa {Serializing Qt Data Types}
*/

QDataStream &operator<<(QDataStream &out, const QTime &time)
{
    return out << quint32(time.mds);
}

/*!
    \relates QTime

    Reads a time from stream \a in into the given \a time.

    \sa {Serializing Qt Data Types}
*/

QDataStream &operator>>(QDataStream &in, QTime &time)
{
    quint32 ds;
    in >> ds;
    time.mds = int(ds);
    return in;
}

/*!
    \relates QDateTime

    Writes \a dateTime to the \a out stream.

    \sa {Serializing Qt Data Types}
*/
QDataStream &operator<<(QDataStream &out, const QDateTime &dateTime)
{
    QDate dt;
    QTime tm;

    if (out.version() >= QDataStream::Qt_5_2) {

        // In 5.2 we switched to using Qt::TimeSpec and added offset support
        dateTime.d->getDateTime(&dt, &tm);
        out << dt << tm << qint8(dateTime.timeSpec());
        if (dateTime.timeSpec() == Qt::OffsetFromUTC)
            out << qint32(dateTime.offsetFromUtc());
#ifndef QT_BOOTSTRAPPED
        else if (dateTime.timeSpec() == Qt::TimeZone)
            out << dateTime.timeZone();
#endif // QT_BOOTSTRAPPED

    } else if (out.version() == QDataStream::Qt_5_0) {

        // In Qt 5.0 we incorrectly serialised all datetimes as UTC.
        // This approach is wrong and should not be used again; it breaks
        // the guarantee that a deserialised local datetime is the same time
        // of day, regardless of which timezone it was serialised in.
        if (dateTime.isValid())
            dateTime.toUTC().d->getDateTime(&dt, &tm);
        else
            dateTime.d->getDateTime(&dt, &tm);
        out << dt << tm << qint8(dateTime.timeSpec());

    } else if (out.version() >= QDataStream::Qt_4_0) {

        // From 4.0 to 5.1 (except 5.0) we used QDateTimePrivate::Spec
        dateTime.d->getDateTime(&dt, &tm);
        out << dt << tm;
        if (out.version() >= QDataStream::Qt_4_0) {
            switch (dateTime.timeSpec()) {
            case Qt::UTC:
                out << (qint8)QDateTimePrivate::UTC;
                break;
            case Qt::OffsetFromUTC:
                out << (qint8)QDateTimePrivate::OffsetFromUTC;
                break;
            case Qt::TimeZone:
#ifndef QT_BOOTSTRAPPED
                out << (qint8)QDateTimePrivate::TimeZone;
                break;
#endif // QT_BOOTSTRAPPED
            case Qt::LocalTime:
                out << (qint8)QDateTimePrivate::LocalUnknown;
                break;
            }
        }

    } else { // version < QDataStream::Qt_4_0

        // Before 4.0 there was no TimeSpec, only Qt::LocalTime was supported
        dateTime.d->getDateTime(&dt, &tm);
        out << dt << tm;

    }

    return out;
}

/*!
    \relates QDateTime

    Reads a datetime from the stream \a in into \a dateTime.

    \sa {Serializing Qt Data Types}
*/

QDataStream &operator>>(QDataStream &in, QDateTime &dateTime)
{
    dateTime.detach();

    QDate dt;
    QTime tm;
    qint8 ts = 0;
    Qt::TimeSpec spec = Qt::LocalTime;
    qint32 offset = 0;
#ifndef QT_BOOTSTRAPPED
    QTimeZone tz;
#endif // QT_BOOTSTRAPPED

    if (in.version() >= QDataStream::Qt_5_2) {

        // In 5.2 we switched to using Qt::TimeSpec and added offset support
        in >> dt >> tm >> ts;
        spec = static_cast<Qt::TimeSpec>(ts);
        if (spec == Qt::OffsetFromUTC) {
            in >> offset;
            dateTime = QDateTime(dt, tm, spec, offset);
#ifndef QT_BOOTSTRAPPED
        } else if (spec == Qt::TimeZone) {
            in >> tz;
            dateTime = QDateTime(dt, tm, tz);
#endif // QT_BOOTSTRAPPED
        } else {
            dateTime = QDateTime(dt, tm, spec);
        }

    } else if (in.version() == QDataStream::Qt_5_0) {

        // In Qt 5.0 we incorrectly serialised all datetimes as UTC
        in >> dt >> tm >> ts;
        spec = static_cast<Qt::TimeSpec>(ts);
        dateTime = QDateTime(dt, tm, Qt::UTC);
        dateTime = dateTime.toTimeSpec(spec);

    } else if (in.version() >= QDataStream::Qt_4_0) {

        // From 4.0 to 5.1 (except 5.0) we used QDateTimePrivate::Spec
        in >> dt >> tm >> ts;
        switch ((QDateTimePrivate::Spec)ts) {
        case QDateTimePrivate::UTC:
            spec = Qt::UTC;
            break;
        case QDateTimePrivate::OffsetFromUTC:
            spec = Qt::OffsetFromUTC;
            break;
        case QDateTimePrivate::TimeZone:
#ifndef QT_BOOTSTRAPPED
            spec = Qt::TimeZone;
            break;
#endif // QT_BOOTSTRAPPED
        case QDateTimePrivate::LocalUnknown:
        case QDateTimePrivate::LocalStandard:
        case QDateTimePrivate::LocalDST:
            spec = Qt::LocalTime;
            break;
        }
        dateTime = QDateTime(dt, tm, spec, offset);

    } else { // version < QDataStream::Qt_4_0

        // Before 4.0 there was no TimeSpec, only Qt::LocalTime was supported
        in >> dt >> tm;
        dateTime = QDateTime(dt, tm, spec, offset);

    }

    return in;
}
#endif // QT_NO_DATASTREAM

/*****************************************************************************
  Date / Time Debug Streams
*****************************************************************************/

#if !defined(QT_NO_DEBUG_STREAM) && !defined(QT_NO_DATESTRING)
QDebug operator<<(QDebug dbg, const QDate &date)
{
    dbg.nospace() << "QDate(" << date.toString(QStringLiteral("yyyy-MM-dd")) << ')';
    return dbg.space();
}

QDebug operator<<(QDebug dbg, const QTime &time)
{
    dbg.nospace() << "QTime(" << time.toString(QStringLiteral("HH:mm:ss.zzz")) << ')';
    return dbg.space();
}

QDebug operator<<(QDebug dbg, const QDateTime &date)
{
    QString spec;
    switch (date.d->m_spec) {
    case Qt::UTC:
        spec = QStringLiteral(" Qt::UTC");
        break;
    case Qt::OffsetFromUTC:
        spec = QString::fromUtf8(" Qt::OffsetFromUTC %1s").arg(date.offsetFromUtc());
        break;
    case Qt::TimeZone:
#ifndef QT_BOOTSTRAPPED
        spec = QStringLiteral(" Qt::TimeZone ") + QString::fromLatin1(date.timeZone().id());
        break;
#endif // QT_BOOTSTRAPPED
    case Qt::LocalTime:
        spec = QStringLiteral(" Qt::LocalTime");
        break;
    }
    QString output = date.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz t")) + spec;
    dbg.nospace() << "QDateTime(" << output << ')';
    return dbg.space();
}
#endif

/*! \fn uint qHash(const QDateTime &key, uint seed = 0)
    \relates QHash
    \since 5.0

    Returns the hash value for the \a key, using \a seed to seed the calculation.
*/
uint qHash(const QDateTime &key, uint seed)
{
    // Use to toMSecsSinceEpoch instead of individual qHash functions for
    // QDate/QTime/spec/offset because QDateTime::operator== converts both arguments
    // to the same timezone. If we don't, qHash would return different hashes for
    // two QDateTimes that are equivalent once converted to the same timezone.
    return qHash(key.toMSecsSinceEpoch(), seed);
}

/*! \fn uint qHash(const QDate &key, uint seed = 0)
    \relates QHash
    \since 5.0

    Returns the hash value for the \a key, using \a seed to seed the calculation.
*/
uint qHash(const QDate &key, uint seed) Q_DECL_NOTHROW
{
    return qHash(key.toJulianDay(), seed);
}

/*! \fn uint qHash(const QTime &key, uint seed = 0)
    \relates QHash
    \since 5.0

    Returns the hash value for the \a key, using \a seed to seed the calculation.
*/
uint qHash(const QTime &key, uint seed) Q_DECL_NOTHROW
{
    return qHash(key.msecsSinceStartOfDay(), seed);
}

QT_END_NAMESPACE
