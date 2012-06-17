/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qcalendarwidget.h"

#ifndef QT_NO_CALENDARWIDGET

#include <qabstractitemmodel.h>
#include <qitemdelegate.h>
#include <qdatetime.h>
#include <qtableview.h>
#include <qlayout.h>
#include <qevent.h>
#include <qtextformat.h>
#include <qheaderview.h>
#include <private/qwidget_p.h>
#include <qpushbutton.h>
#include <qtoolbutton.h>
#include <qlabel.h>
#include <qspinbox.h>
#include <qmenu.h>
#include <qapplication.h>
#include <qbasictimer.h>
#include <qstylepainter.h>
#include <private/qcalendartextnavigator_p.h>

QT_BEGIN_NAMESPACE

enum {
    RowCount = 6,
    ColumnCount = 7,
    HeaderColumn = 0,
    HeaderRow = 0,
    MinimumDayOffset = 1
};

class QCalendarDateSectionValidator
{
public:

    enum Section {
        NextSection,
        ThisSection,
        PrevSection
    };

    QCalendarDateSectionValidator() {}
    virtual ~QCalendarDateSectionValidator() {}
    virtual Section handleKey(int key) = 0;
    virtual QDate applyToDate(const QDate &date) const = 0;
    virtual void setDate(const QDate &date) = 0;
    virtual QString text() const = 0;
    virtual QString text(const QDate &date, int repeat) const = 0;

    QLocale m_locale;

protected:
    QString highlightString(const QString &str, int pos) const;
private:
};

QString QCalendarDateSectionValidator::highlightString(const QString &str, int pos) const
{
    if (pos == 0)
        return QLatin1String("<b>") + str + QLatin1String("</b>");
    int startPos = str.length() - pos;
    return str.mid(0, startPos) + QLatin1String("<b>") + str.mid(startPos, pos) + QLatin1String("</b>");

}

class QCalendarDayValidator : public QCalendarDateSectionValidator
{

public:
    QCalendarDayValidator();
    virtual Section handleKey(int key);
    virtual QDate applyToDate(const QDate &date) const;
    virtual void setDate(const QDate &date);
    virtual QString text() const;
    virtual QString text(const QDate &date, int repeat) const;
private:
    int m_pos;
    int m_day;
    int m_oldDay;
};

QCalendarDayValidator::QCalendarDayValidator()
    : QCalendarDateSectionValidator(), m_pos(0), m_day(1), m_oldDay(1)
{
}

QCalendarDateSectionValidator::Section QCalendarDayValidator::handleKey(int key)
{
    if (key == Qt::Key_Right || key == Qt::Key_Left) {
        m_pos = 0;
        return QCalendarDateSectionValidator::ThisSection;
    } else if (key == Qt::Key_Up) {
        m_pos = 0;
        ++m_day;
        if (m_day > 31)
            m_day = 1;
        return QCalendarDateSectionValidator::ThisSection;
    } else if (key == Qt::Key_Down) {
        m_pos = 0;
        --m_day;
        if (m_day < 1)
            m_day = 31;
        return QCalendarDateSectionValidator::ThisSection;
    } else if (key == Qt::Key_Back || key == Qt::Key_Backspace) {
        --m_pos;
        if (m_pos < 0)
            m_pos = 1;

        if (m_pos == 0)
            m_day = m_oldDay;
        else
            m_day = m_day / 10;
            //m_day = m_oldDay / 10 * 10 + m_day / 10;

        if (m_pos == 0)
            return QCalendarDateSectionValidator::PrevSection;
        return QCalendarDateSectionValidator::ThisSection;
    }
    if (key < Qt::Key_0 || key > Qt::Key_9)
        return QCalendarDateSectionValidator::ThisSection;
    int pressedKey = key - Qt::Key_0;
    if (m_pos == 0)
        m_day = pressedKey;
    else
        m_day = m_day % 10 * 10 + pressedKey;
    if (m_day > 31)
        m_day = 31;
    ++m_pos;
    if (m_pos > 1) {
        m_pos = 0;
        return QCalendarDateSectionValidator::NextSection;
    }
    return QCalendarDateSectionValidator::ThisSection;
}

QDate QCalendarDayValidator::applyToDate(const QDate &date) const
{
    int day = m_day;
    if (day < 1)
        day = 1;
    else if (day > 31)
        day = 31;
    if (day > date.daysInMonth())
        day = date.daysInMonth();
    return QDate(date.year(), date.month(), day);
}

void QCalendarDayValidator::setDate(const QDate &date)
{
    m_day = m_oldDay = date.day();
    m_pos = 0;
}

QString QCalendarDayValidator::text() const
{
    QString str;
    if (m_day / 10 == 0)
        str += QLatin1Char('0');
    str += QString::number(m_day);
    return highlightString(str, m_pos);
}

QString QCalendarDayValidator::text(const QDate &date, int repeat) const
{
    if (repeat <= 1) {
        return QString::number(date.day());
    } else if (repeat == 2) {
        QString str;
        if (date.day() / 10 == 0)
            str += QLatin1Char('0');
        return str + QString::number(date.day());
    } else if (repeat == 3) {
        return m_locale.dayName(date.dayOfWeek(), QLocale::ShortFormat);
    } else if (repeat >= 4) {
        return m_locale.dayName(date.dayOfWeek(), QLocale::LongFormat);
    }
    return QString();
}

//////////////////////////////////

class QCalendarMonthValidator : public QCalendarDateSectionValidator
{

public:
    QCalendarMonthValidator();
    virtual Section handleKey(int key);
    virtual QDate applyToDate(const QDate &date) const;
    virtual void setDate(const QDate &date);
    virtual QString text() const;
    virtual QString text(const QDate &date, int repeat) const;
private:
    int m_pos;
    int m_month;
    int m_oldMonth;
};

QCalendarMonthValidator::QCalendarMonthValidator()
    : QCalendarDateSectionValidator(), m_pos(0), m_month(1), m_oldMonth(1)
{
}

QCalendarDateSectionValidator::Section QCalendarMonthValidator::handleKey(int key)
{
    if (key == Qt::Key_Right || key == Qt::Key_Left) {
        m_pos = 0;
        return QCalendarDateSectionValidator::ThisSection;
    } else if (key == Qt::Key_Up) {
        m_pos = 0;
        ++m_month;
        if (m_month > 12)
            m_month = 1;
        return QCalendarDateSectionValidator::ThisSection;
    } else if (key == Qt::Key_Down) {
        m_pos = 0;
        --m_month;
        if (m_month < 1)
            m_month = 12;
        return QCalendarDateSectionValidator::ThisSection;
    } else if (key == Qt::Key_Back || key == Qt::Key_Backspace) {
        --m_pos;
        if (m_pos < 0)
            m_pos = 1;

        if (m_pos == 0)
            m_month = m_oldMonth;
        else
            m_month = m_month / 10;
            //m_month = m_oldMonth / 10 * 10 + m_month / 10;

        if (m_pos == 0)
            return QCalendarDateSectionValidator::PrevSection;
        return QCalendarDateSectionValidator::ThisSection;
    }
    if (key < Qt::Key_0 || key > Qt::Key_9)
        return QCalendarDateSectionValidator::ThisSection;
    int pressedKey = key - Qt::Key_0;
    if (m_pos == 0)
        m_month = pressedKey;
    else
        m_month = m_month % 10 * 10 + pressedKey;
    if (m_month > 12)
        m_month = 12;
    ++m_pos;
    if (m_pos > 1) {
        m_pos = 0;
        return QCalendarDateSectionValidator::NextSection;
    }
    return QCalendarDateSectionValidator::ThisSection;
}

QDate QCalendarMonthValidator::applyToDate(const QDate &date) const
{
    int month = m_month;
    if (month < 1)
        month = 1;
    else if (month > 12)
        month = 12;
    QDate newDate(date.year(), m_month, 1);
    int day = date.day();
    if (day > newDate.daysInMonth())
        day = newDate.daysInMonth();
    return QDate(date.year(), month, day);
}

void QCalendarMonthValidator::setDate(const QDate &date)
{
    m_month = m_oldMonth = date.month();
    m_pos = 0;
}

QString QCalendarMonthValidator::text() const
{
    QString str;
    if (m_month / 10 == 0)
        str += QLatin1Char('0');
    str += QString::number(m_month);
    return highlightString(str, m_pos);
}

QString QCalendarMonthValidator::text(const QDate &date, int repeat) const
{
    if (repeat <= 1) {
        return QString::number(date.month());
    } else if (repeat == 2) {
        QString str;
        if (date.month() / 10 == 0)
            str += QLatin1Char('0');
        return str + QString::number(date.month());
    } else if (repeat == 3) {
        return m_locale.standaloneMonthName(date.month(), QLocale::ShortFormat);
    } else /*if (repeat >= 4)*/ {
        return m_locale.standaloneMonthName(date.month(), QLocale::LongFormat);
    }
}

//////////////////////////////////

class QCalendarYearValidator : public QCalendarDateSectionValidator
{

public:
    QCalendarYearValidator();
    virtual Section handleKey(int key);
    virtual QDate applyToDate(const QDate &date) const;
    virtual void setDate(const QDate &date);
    virtual QString text() const;
    virtual QString text(const QDate &date, int repeat) const;
private:
    int pow10(int n);
    int m_pos;
    int m_year;
    int m_oldYear;
};

QCalendarYearValidator::QCalendarYearValidator()
    : QCalendarDateSectionValidator(), m_pos(0), m_year(2000), m_oldYear(2000)
{
}

int QCalendarYearValidator::pow10(int n)
{
    int power = 1;
    for (int i = 0; i < n; i++)
        power *= 10;
    return power;
}

QCalendarDateSectionValidator::Section QCalendarYearValidator::handleKey(int key)
{
    if (key == Qt::Key_Right || key == Qt::Key_Left) {
        m_pos = 0;
        return QCalendarDateSectionValidator::ThisSection;
    } else if (key == Qt::Key_Up) {
        m_pos = 0;
        ++m_year;
        return QCalendarDateSectionValidator::ThisSection;
    } else if (key == Qt::Key_Down) {
        m_pos = 0;
        --m_year;
        return QCalendarDateSectionValidator::ThisSection;
    } else if (key == Qt::Key_Back || key == Qt::Key_Backspace) {
        --m_pos;
        if (m_pos < 0)
            m_pos = 3;

        int pow = pow10(m_pos);
        m_year = m_oldYear / pow * pow + m_year % (pow * 10) / 10;

        if (m_pos == 0)
            return QCalendarDateSectionValidator::PrevSection;
        return QCalendarDateSectionValidator::ThisSection;
    }
    if (key < Qt::Key_0 || key > Qt::Key_9)
        return QCalendarDateSectionValidator::ThisSection;
    int pressedKey = key - Qt::Key_0;
    int pow = pow10(m_pos);
    m_year = m_year / (pow * 10) * (pow * 10) + m_year % pow * 10 + pressedKey;
    ++m_pos;
    if (m_pos > 3) {
        m_pos = 0;
        return QCalendarDateSectionValidator::NextSection;
    }
    return QCalendarDateSectionValidator::ThisSection;
}

QDate QCalendarYearValidator::applyToDate(const QDate &date) const
{
    int year = m_year;
    if (year < 1)
        year = 1;
    QDate newDate(year, date.month(), 1);
    int day = date.day();
    if (day > newDate.daysInMonth())
        day = newDate.daysInMonth();
    return QDate(year, date.month(), day);
}

void QCalendarYearValidator::setDate(const QDate &date)
{
    m_year = m_oldYear = date.year();
    m_pos = 0;
}

QString QCalendarYearValidator::text() const
{
    QString str;
    int pow = 10;
    for (int i = 0; i < 3; i++) {
        if (m_year / pow == 0)
            str += QLatin1Char('0');
        pow *= 10;
    }
    str += QString::number(m_year);
    return highlightString(str, m_pos);
}

QString QCalendarYearValidator::text(const QDate &date, int repeat) const
{
    if (repeat < 4) {
        QString str;
        int year = date.year() % 100;
        if (year / 10 == 0)
            str = QLatin1Char('0');
        return str + QString::number(year);
    }
    return QString::number(date.year());
}

///////////////////////////////////

class QCalendarDateValidator
{
public:
    QCalendarDateValidator();
    ~QCalendarDateValidator();

    void handleKeyEvent(QKeyEvent *keyEvent);
    QString currentText() const;
    QDate currentDate() const { return m_currentDate; }
    void setFormat(const QString &format);
    void setInitialDate(const QDate &date);

    void setLocale(const QLocale &locale);

private:

    struct SectionToken {
        SectionToken(QCalendarDateSectionValidator *val, int rep) : validator(val), repeat(rep) {}
        QCalendarDateSectionValidator *validator;
        int repeat;
    };

    void toNextToken();
    void toPreviousToken();
    void applyToDate();

    int countRepeat(const QString &str, int index) const;
    void clear();

    QStringList m_separators;
    QList<SectionToken *> m_tokens;
    QCalendarDateSectionValidator *m_yearValidator;
    QCalendarDateSectionValidator *m_monthValidator;
    QCalendarDateSectionValidator *m_dayValidator;

    SectionToken *m_currentToken;

    QDate m_initialDate;
    QDate m_currentDate;

    QCalendarDateSectionValidator::Section m_lastSectionMove;
};

QCalendarDateValidator::QCalendarDateValidator()
    : m_currentToken(0), m_lastSectionMove(QCalendarDateSectionValidator::ThisSection)
{
    m_initialDate = m_currentDate = QDate::currentDate();
    m_yearValidator = new QCalendarYearValidator();
    m_monthValidator = new QCalendarMonthValidator();
    m_dayValidator = new QCalendarDayValidator();
}

void QCalendarDateValidator::setLocale(const QLocale &locale)
{
    m_yearValidator->m_locale = locale;
    m_monthValidator->m_locale = locale;
    m_dayValidator->m_locale = locale;
}

QCalendarDateValidator::~QCalendarDateValidator()
{
    delete m_yearValidator;
    delete m_monthValidator;
    delete m_dayValidator;
    clear();
}

// from qdatetime.cpp
int QCalendarDateValidator::countRepeat(const QString &str, int index) const
{
    Q_ASSERT(index >= 0 && index < str.size());
    int count = 1;
    const QChar ch = str.at(index);
    while (index + count < str.size() && str.at(index + count) == ch)
        ++count;
    return count;
}

void QCalendarDateValidator::setInitialDate(const QDate &date)
{
    m_yearValidator->setDate(date);
    m_monthValidator->setDate(date);
    m_dayValidator->setDate(date);
    m_initialDate = date;
    m_currentDate = date;
    m_lastSectionMove = QCalendarDateSectionValidator::ThisSection;
}

QString QCalendarDateValidator::currentText() const
{
    QString str;
    QStringListIterator itSep(m_separators);
    QListIterator<SectionToken *> itTok(m_tokens);
    while (itSep.hasNext()) {
        str += itSep.next();
        if (itTok.hasNext()) {
            SectionToken *token = itTok.next();
            QCalendarDateSectionValidator *validator = token->validator;
            if (m_currentToken == token)
                str += validator->text();
            else
                str += validator->text(m_currentDate, token->repeat);
        }
    }
    return str;
}

void QCalendarDateValidator::clear()
{
    QListIterator<SectionToken *> it(m_tokens);
    while (it.hasNext())
        delete it.next();

    m_tokens.clear();
    m_separators.clear();

    m_currentToken = 0;
}

void QCalendarDateValidator::setFormat(const QString &format)
{
    clear();

    int pos = 0;
    const QLatin1Char quote('\'');
    bool quoting = false;
    QString separator;
    while (pos < format.size()) {
        QString mid = format.mid(pos);
        int offset = 1;

        if (mid.startsWith(quote)) {
            quoting = !quoting;
        } else {
            const QChar nextChar = format.at(pos);
            if (quoting) {
                separator += nextChar;
            } else {
                SectionToken *token = 0;
                if (nextChar == QLatin1Char('d')) {
                    offset = qMin(4, countRepeat(format, pos));
                    token = new SectionToken(m_dayValidator, offset);
                } else if (nextChar == QLatin1Char('M')) {
                    offset = qMin(4, countRepeat(format, pos));
                    token = new SectionToken(m_monthValidator, offset);
                } else if (nextChar == QLatin1Char('y')) {
                    offset = qMin(4, countRepeat(format, pos));
                    token = new SectionToken(m_yearValidator, offset);
                } else {
                    separator += nextChar;
                }
                if (token) {
                    m_tokens.append(token);
                    m_separators.append(separator);
                    separator = QString();
                    if (!m_currentToken)
                        m_currentToken = token;

                }
            }
        }
        pos += offset;
    }
    m_separators += separator;
}

void QCalendarDateValidator::applyToDate()
{
    m_currentDate = m_yearValidator->applyToDate(m_currentDate);
    m_currentDate = m_monthValidator->applyToDate(m_currentDate);
    m_currentDate = m_dayValidator->applyToDate(m_currentDate);
}

void QCalendarDateValidator::toNextToken()
{
    const int idx = m_tokens.indexOf(m_currentToken);
    if (idx == -1)
        return;
    if (idx + 1 >= m_tokens.count())
        m_currentToken = m_tokens.first();
    else
        m_currentToken = m_tokens.at(idx + 1);
}

void QCalendarDateValidator::toPreviousToken()
{
    const int idx = m_tokens.indexOf(m_currentToken);
    if (idx == -1)
        return;
    if (idx - 1 < 0)
        m_currentToken = m_tokens.last();
    else
        m_currentToken = m_tokens.at(idx - 1);
}

void QCalendarDateValidator::handleKeyEvent(QKeyEvent *keyEvent)
{
    if (!m_currentToken)
        return;

    int key = keyEvent->key();
    if (m_lastSectionMove == QCalendarDateSectionValidator::NextSection) {
        if (key == Qt::Key_Back || key == Qt::Key_Backspace)
            toPreviousToken();
    }
    if (key == Qt::Key_Right)
        toNextToken();
    else if (key == Qt::Key_Left)
        toPreviousToken();

    m_lastSectionMove = m_currentToken->validator->handleKey(key);

    applyToDate();
    if (m_lastSectionMove == QCalendarDateSectionValidator::NextSection)
        toNextToken();
    else if (m_lastSectionMove == QCalendarDateSectionValidator::PrevSection)
        toPreviousToken();
}

QWidget *QCalendarTextNavigator::widget() const
{
    return m_widget;
}

void QCalendarTextNavigator::setWidget(QWidget *widget)
{
    m_widget = widget;
}

QDate QCalendarTextNavigator::date() const
{
    return m_date;
}

void QCalendarTextNavigator::setDate(const QDate &date)
{
    m_date = date;
}

void QCalendarTextNavigator::updateDateLabel()
{
    if (!m_widget)
        return;

    m_acceptTimer.start(m_editDelay, this);

    m_dateText->setText(m_dateValidator->currentText());

    QSize s = m_dateFrame->sizeHint();
    QRect r = m_widget->geometry(); // later, just the table section
    QRect newRect((r.width() - s.width()) / 2, (r.height() - s.height()) / 2, s.width(), s.height());
    m_dateFrame->setGeometry(newRect);
    // need to set palette after geometry update as phonestyle sets transparency
    // effect in move event.
    QPalette p = m_dateFrame->palette();
    p.setBrush(QPalette::Window, m_dateFrame->window()->palette().brush(QPalette::Window));
    m_dateFrame->setPalette(p);

    m_dateFrame->raise();
    m_dateFrame->show();
}

void QCalendarTextNavigator::applyDate()
{
    QDate date = m_dateValidator->currentDate();
    if (m_date == date)
        return;

    m_date = date;
    emit dateChanged(date);
}

void QCalendarTextNavigator::createDateLabel()
{
    if (m_dateFrame)
        return;
    m_dateFrame = new QFrame(m_widget);
    QVBoxLayout *vl = new QVBoxLayout;
    m_dateText = new QLabel;
    vl->addWidget(m_dateText);
    m_dateFrame->setLayout(vl);
    m_dateFrame->setFrameShadow(QFrame::Plain);
    m_dateFrame->setFrameShape(QFrame::Box);
    m_dateValidator = new QCalendarDateValidator();
    m_dateValidator->setLocale(m_widget->locale());
    m_dateValidator->setFormat(m_widget->locale().dateFormat(QLocale::ShortFormat));
    m_dateValidator->setInitialDate(m_date);

    m_dateFrame->setAutoFillBackground(true);
    m_dateFrame->setBackgroundRole(QPalette::Window);
}

void QCalendarTextNavigator::removeDateLabel()
{
    if (!m_dateFrame)
        return;
    m_acceptTimer.stop();
    m_dateFrame->hide();
    m_dateFrame->deleteLater();
    delete m_dateValidator;
    m_dateFrame = 0;
    m_dateText = 0;
    m_dateValidator = 0;
}

bool QCalendarTextNavigator::eventFilter(QObject *o, QEvent *e)
{
    if (m_widget) {
        if (e->type() == QEvent::KeyPress || e->type() == QEvent::KeyRelease) {
            QKeyEvent* ke = (QKeyEvent*)e;
            if ((ke->text().length() > 0 && ke->text()[0].isPrint()) || m_dateFrame) {
                if (ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter || ke->key() == Qt::Key_Select) {
                    applyDate();
                    emit editingFinished();
                    removeDateLabel();
                } else if (ke->key() == Qt::Key_Escape) {
                    removeDateLabel();
                } else if (e->type() == QEvent::KeyPress) {
                    createDateLabel();
                    m_dateValidator->handleKeyEvent(ke);
                    updateDateLabel();
                }
                ke->accept();
                return true;
            }
            // If we are navigating let the user finish his date in old locate.
            // If we change our mind and want it to update immediately simply uncomment below
            /*
        } else if (e->type() == QEvent::LocaleChange) {
            if (m_dateValidator) {
                m_dateValidator->setLocale(m_widget->locale());
                m_dateValidator->setFormat(m_widget->locale().dateFormat(QLocale::ShortFormat));
                updateDateLabel();
            }
            */
        }
    }
    return QObject::eventFilter(o,e);
}

void QCalendarTextNavigator::timerEvent(QTimerEvent *e)
{
    if (e->timerId() == m_acceptTimer.timerId()) {
        applyDate();
        removeDateLabel();
    }
}

int QCalendarTextNavigator::dateEditAcceptDelay() const
{
    return m_editDelay;
}

void QCalendarTextNavigator::setDateEditAcceptDelay(int delay)
{
    m_editDelay = delay;
}

class QCalendarView;

class QCalendarModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    QCalendarModel(QObject *parent = 0);

    int rowCount(const QModelIndex &) const
        { return RowCount + m_firstRow; }
    int columnCount(const QModelIndex &) const
        { return ColumnCount + m_firstColumn; }
    QVariant data(const QModelIndex &index, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;

    bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex())
    {
        beginInsertRows(parent, row, row + count - 1);
        endInsertRows();
        return true;
    }
    bool insertColumns(int column, int count, const QModelIndex &parent = QModelIndex())
    {
        beginInsertColumns(parent, column, column + count - 1);
        endInsertColumns();
        return true;
    }
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex())
    {
        beginRemoveRows(parent, row, row + count - 1);
        endRemoveRows();
        return true;
    }
    bool removeColumns(int column, int count, const QModelIndex &parent = QModelIndex())
    {
        beginRemoveColumns(parent, column, column + count - 1);
        endRemoveColumns();
        return true;
    }

    void showMonth(int year, int month);
    void setDate(const QDate &d);

    void setMinimumDate(const QDate &date);
    void setMaximumDate(const QDate &date);

    void setRange(const QDate &min, const QDate &max);

    void setHorizontalHeaderFormat(QCalendarWidget::HorizontalHeaderFormat format);

    void setFirstColumnDay(Qt::DayOfWeek dayOfWeek);
    Qt::DayOfWeek firstColumnDay() const;

    bool weekNumbersShown() const;
    void setWeekNumbersShown(bool show);

    QTextCharFormat formatForCell(int row, int col) const;
    Qt::DayOfWeek dayOfWeekForColumn(int section) const;
    int columnForDayOfWeek(Qt::DayOfWeek day) const;
    QDate dateForCell(int row, int column) const;
    void cellForDate(const QDate &date, int *row, int *column) const;
    QString dayName(Qt::DayOfWeek day) const;

    void setView(QCalendarView *view)
        { m_view = view; }

    void internalUpdate();
    QDate referenceDate() const;
    int columnForFirstOfMonth(const QDate &date) const;

    int m_firstColumn;
    int m_firstRow;
    QDate m_date;
    QDate m_minimumDate;
    QDate m_maximumDate;
    int m_shownYear;
    int m_shownMonth;
    Qt::DayOfWeek m_firstDay;
    QCalendarWidget::HorizontalHeaderFormat m_horizontalHeaderFormat;
    bool m_weekNumbersShown;
    QMap<Qt::DayOfWeek, QTextCharFormat> m_dayFormats;
    QMap<QDate, QTextCharFormat> m_dateFormats;
    QTextCharFormat m_headerFormat;
    QCalendarView *m_view;
};

class QCalendarView : public QTableView
{
    Q_OBJECT
public:
    QCalendarView(QWidget *parent = 0);

    void internalUpdate() { updateGeometries(); }
    void setReadOnly(bool enable);
    virtual void keyboardSearch(const QString & search) { Q_UNUSED(search) }

signals:
    void showDate(const QDate &date);
    void changeDate(const QDate &date, bool changeMonth);
    void clicked(const QDate &date);
    void editingFinished();
protected:
    QModelIndex moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers);
    void mouseDoubleClickEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
#ifndef QT_NO_WHEELEVENT
    void wheelEvent(QWheelEvent *event);
#endif
    void keyPressEvent(QKeyEvent *event);
    bool event(QEvent *event);

    QDate handleMouseEvent(QMouseEvent *event);
public:
    bool readOnly;
private:
    bool validDateClicked;
#ifdef QT_KEYPAD_NAVIGATION
    QDate origDate;
#endif
};

QCalendarModel::QCalendarModel(QObject *parent)
    : QAbstractTableModel(parent)
{
    m_date = QDate::currentDate();
    m_minimumDate = QDate::fromJulianDay(1);
    m_maximumDate = QDate(7999, 12, 31);
    m_shownYear = m_date.year();
    m_shownMonth = m_date.month();
    m_firstDay = Qt::Sunday;
    m_horizontalHeaderFormat = QCalendarWidget::ShortDayNames;
    m_weekNumbersShown = true;
    m_firstColumn = 1;
    m_firstRow = 1;
    m_view = 0;
}

Qt::DayOfWeek QCalendarModel::dayOfWeekForColumn(int column) const
{
    int col = column - m_firstColumn;
    if (col < 0 || col > 6)
        return Qt::Sunday;
    int day = m_firstDay + col;
    if (day > 7)
        day -= 7;
    return Qt::DayOfWeek(day);
}

int QCalendarModel::columnForDayOfWeek(Qt::DayOfWeek day) const
{
    if (day < 1 || day > 7)
        return -1;
    int column = (int)day - (int)m_firstDay;
    if (column < 0)
        column += 7;
    return column + m_firstColumn;
}

/*
This simple algorithm tries to generate a valid date from the month shown.
Some months don't contain a first day (e.g. Jan of -4713 year,
so QDate (-4713, 1, 1) would be invalid). In that case we try to generate
another valid date for that month. Later, returned date's day is the number of cells
calendar widget will reserve for days before referenceDate. (E.g. if returned date's
day is 16, that day will be placed in 3rd or 4th row, not in the 1st or 2nd row).
Depending on referenceData we can change behaviour of Oct 1582. If referenceDate is 1st
of Oct we render 1 Oct in 1st or 2nd row. If referenceDate is 17 of Oct we show always 16
dates before 17 of Oct, and since this month contains the hole 5-14 Oct, the first of Oct
will be rendered in 2nd or 3rd row, showing more dates from previous month.
*/
QDate QCalendarModel::referenceDate() const
{
    int refDay = 1;
    while (refDay <= 31) {
        QDate refDate(m_shownYear, m_shownMonth, refDay);
        if (refDate.isValid())
            return refDate;
        refDay += 1;
    }
    return QDate();
}

int QCalendarModel::columnForFirstOfMonth(const QDate &date) const
{
    return (columnForDayOfWeek(static_cast<Qt::DayOfWeek>(date.dayOfWeek())) - (date.day() % 7) + 8) % 7;
}

QDate QCalendarModel::dateForCell(int row, int column) const
{
    if (row < m_firstRow || row > m_firstRow + RowCount - 1 ||
                column < m_firstColumn || column > m_firstColumn + ColumnCount - 1)
        return QDate();
    const QDate refDate = referenceDate();
    if (!refDate.isValid())
        return QDate();

    const int columnForFirstOfShownMonth = columnForFirstOfMonth(refDate);
    if (columnForFirstOfShownMonth - m_firstColumn < MinimumDayOffset)
        row -= 1;

    const int requestedDay = 7 * (row - m_firstRow) + column - columnForFirstOfShownMonth - refDate.day() + 1;
    return refDate.addDays(requestedDay);
}

void QCalendarModel::cellForDate(const QDate &date, int *row, int *column) const
{
    if (!row && !column)
        return;

    if (row)
        *row = -1;
    if (column)
        *column = -1;

    const QDate refDate = referenceDate();
    if (!refDate.isValid())
        return;

    const int columnForFirstOfShownMonth = columnForFirstOfMonth(refDate);
    const int requestedPosition = refDate.daysTo(date) - m_firstColumn + columnForFirstOfShownMonth + refDate.day() - 1;

    int c = requestedPosition % 7;
    int r = requestedPosition / 7;
    if (c < 0) {
        c += 7;
        r -= 1;
    }

    if (columnForFirstOfShownMonth - m_firstColumn < MinimumDayOffset)
        r += 1;

    if (r < 0 || r > RowCount - 1 || c < 0 || c > ColumnCount - 1)
        return;

    if (row)
        *row = r + m_firstRow;
    if (column)
        *column = c + m_firstColumn;
}

QString QCalendarModel::dayName(Qt::DayOfWeek day) const
{
    switch (m_horizontalHeaderFormat) {
        case QCalendarWidget::SingleLetterDayNames: {
            QString standaloneDayName = m_view->locale().standaloneDayName(day, QLocale::NarrowFormat);
            if (standaloneDayName == m_view->locale().dayName(day, QLocale::NarrowFormat))
                return standaloneDayName.left(1);
            return standaloneDayName;
        }
        case QCalendarWidget::ShortDayNames:
            return m_view->locale().dayName(day, QLocale::ShortFormat);
        case QCalendarWidget::LongDayNames:
            return m_view->locale().dayName(day, QLocale::LongFormat);
        default:
            break;
    }
    return QString();
}

QTextCharFormat QCalendarModel::formatForCell(int row, int col) const
{
    QPalette pal;
    QPalette::ColorGroup cg = QPalette::Active;
    if (m_view) {
        pal = m_view->palette();
        if (!m_view->isEnabled())
            cg = QPalette::Disabled;
        else if (!m_view->isActiveWindow())
            cg = QPalette::Inactive;
    }

    QTextCharFormat format;
    format.setFont(m_view->font());
    bool header = (m_weekNumbersShown && col == HeaderColumn)
                  || (m_horizontalHeaderFormat != QCalendarWidget::NoHorizontalHeader && row == HeaderRow);
    format.setBackground(pal.brush(cg, header ? QPalette::AlternateBase : QPalette::Base));
    format.setForeground(pal.brush(cg, QPalette::Text));
    if (header) {
        format.merge(m_headerFormat);
    }

    if (col >= m_firstColumn && col < m_firstColumn + ColumnCount) {
        Qt::DayOfWeek dayOfWeek = dayOfWeekForColumn(col);
        if (m_dayFormats.contains(dayOfWeek))
            format.merge(m_dayFormats.value(dayOfWeek));
    }

    if (!header) {
        QDate date = dateForCell(row, col);
        format.merge(m_dateFormats.value(date));
        if(date < m_minimumDate || date > m_maximumDate)
            format.setBackground(pal.brush(cg, QPalette::Window));
        if (m_shownMonth != date.month())
            format.setForeground(pal.brush(QPalette::Disabled, QPalette::Text));
    }
    return format;
}

QVariant QCalendarModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::TextAlignmentRole)
        return (int) Qt::AlignCenter;

    int row = index.row();
    int column = index.column();

    if(role == Qt::DisplayRole) {
        if (m_weekNumbersShown && column == HeaderColumn
            && row >= m_firstRow && row < m_firstRow + RowCount) {
            QDate date = dateForCell(row, columnForDayOfWeek(Qt::Monday));
            if (date.isValid())
                return date.weekNumber();
        }
        if (m_horizontalHeaderFormat != QCalendarWidget::NoHorizontalHeader && row == HeaderRow
            && column >= m_firstColumn && column < m_firstColumn + ColumnCount)
            return dayName(dayOfWeekForColumn(column));
        QDate date = dateForCell(row, column);
        if (date.isValid())
            return date.day();
        return QString();
    }

    QTextCharFormat fmt = formatForCell(row, column);
    if (role == Qt::BackgroundColorRole)
        return fmt.background().color();
    if (role == Qt::TextColorRole)
        return fmt.foreground().color();
    if (role == Qt::FontRole)
        return fmt.font();
    if (role == Qt::ToolTipRole)
        return fmt.toolTip();
    return QVariant();
}

Qt::ItemFlags QCalendarModel::flags(const QModelIndex &index) const
{
    QDate date = dateForCell(index.row(), index.column());
    if (!date.isValid())
        return QAbstractTableModel::flags(index);
    if (date < m_minimumDate)
        return 0;
    if (date > m_maximumDate)
        return 0;
    return QAbstractTableModel::flags(index);
}

void QCalendarModel::setDate(const QDate &d)
{
    m_date = d;
    if (m_date < m_minimumDate)
        m_date = m_minimumDate;
    else if (m_date > m_maximumDate)
        m_date = m_maximumDate;
}

void QCalendarModel::showMonth(int year, int month)
{
    if (m_shownYear == year && m_shownMonth == month)
        return;

    m_shownYear = year;
    m_shownMonth = month;

    internalUpdate();
}

void QCalendarModel::setMinimumDate(const QDate &d)
{
    if (!d.isValid() || d == m_minimumDate)
        return;

    m_minimumDate = d;
    if (m_maximumDate < m_minimumDate)
        m_maximumDate = m_minimumDate;
    if (m_date < m_minimumDate)
        m_date = m_minimumDate;
    internalUpdate();
}

void QCalendarModel::setMaximumDate(const QDate &d)
{
    if (!d.isValid() || d == m_maximumDate)
        return;

    m_maximumDate = d;
    if (m_minimumDate > m_maximumDate)
        m_minimumDate = m_maximumDate;
    if (m_date > m_maximumDate)
        m_date = m_maximumDate;
    internalUpdate();
}

void QCalendarModel::setRange(const QDate &min, const QDate &max)
{
    m_minimumDate = min;
    m_maximumDate = max;
    if (m_minimumDate > m_maximumDate)
        qSwap(m_minimumDate, m_maximumDate);
    if (m_date < m_minimumDate)
        m_date = m_minimumDate;
    if (m_date > m_maximumDate)
        m_date = m_maximumDate;
    internalUpdate();
}

void QCalendarModel::internalUpdate()
{
    QModelIndex begin = index(0, 0);
    QModelIndex end = index(m_firstRow + RowCount - 1, m_firstColumn + ColumnCount - 1);
    emit dataChanged(begin, end);
    emit headerDataChanged(Qt::Vertical, 0, m_firstRow + RowCount - 1);
    emit headerDataChanged(Qt::Horizontal, 0, m_firstColumn + ColumnCount - 1);
}

void QCalendarModel::setHorizontalHeaderFormat(QCalendarWidget::HorizontalHeaderFormat format)
{
    if (m_horizontalHeaderFormat == format)
        return;

    int oldFormat = m_horizontalHeaderFormat;
    m_horizontalHeaderFormat = format;
    if (oldFormat == QCalendarWidget::NoHorizontalHeader) {
        m_firstRow = 1;
        insertRow(0);
    } else if (m_horizontalHeaderFormat == QCalendarWidget::NoHorizontalHeader) {
        m_firstRow = 0;
        removeRow(0);
    }
    internalUpdate();
}

void QCalendarModel::setFirstColumnDay(Qt::DayOfWeek dayOfWeek)
{
    if (m_firstDay == dayOfWeek)
        return;

    m_firstDay = dayOfWeek;
    internalUpdate();
}

Qt::DayOfWeek QCalendarModel::firstColumnDay() const
{
    return m_firstDay;
}

bool QCalendarModel::weekNumbersShown() const
{
    return m_weekNumbersShown;
}

void QCalendarModel::setWeekNumbersShown(bool show)
{
    if (m_weekNumbersShown == show)
        return;

    m_weekNumbersShown = show;
    if (show) {
        m_firstColumn = 1;
        insertColumn(0);
    } else {
        m_firstColumn = 0;
        removeColumn(0);
    }
    internalUpdate();
}

QCalendarView::QCalendarView(QWidget *parent)
    : QTableView(parent),
    readOnly(false),
    validDateClicked(false)
{
    setTabKeyNavigation(false);
    setShowGrid(false);
    verticalHeader()->setVisible(false);
    horizontalHeader()->setVisible(false);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

QModelIndex QCalendarView::moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers)
{
    QCalendarModel *calendarModel = qobject_cast<QCalendarModel *>(model());
    if (!calendarModel)
        return QTableView::moveCursor(cursorAction, modifiers);

    if (readOnly)
        return currentIndex();

    QModelIndex index = currentIndex();
    QDate currentDate = static_cast<QCalendarModel*>(model())->dateForCell(index.row(), index.column());
    switch (cursorAction) {
        case QAbstractItemView::MoveUp:
            currentDate = currentDate.addDays(-7);
            break;
        case QAbstractItemView::MoveDown:
            currentDate = currentDate.addDays(7);
            break;
        case QAbstractItemView::MoveLeft:
            currentDate = currentDate.addDays(isRightToLeft() ? 1 : -1);
            break;
        case QAbstractItemView::MoveRight:
            currentDate = currentDate.addDays(isRightToLeft() ? -1 : 1);
            break;
        case QAbstractItemView::MoveHome:
            currentDate = QDate(currentDate.year(), currentDate.month(), 1);
            break;
        case QAbstractItemView::MoveEnd:
            currentDate = QDate(currentDate.year(), currentDate.month(), currentDate.daysInMonth());
            break;
        case QAbstractItemView::MovePageUp:
            currentDate = currentDate.addMonths(-1);
            break;
        case QAbstractItemView::MovePageDown:
            currentDate = currentDate.addMonths(1);
            break;
        case QAbstractItemView::MoveNext:
        case QAbstractItemView::MovePrevious:
            return currentIndex();
        default:
            break;
    }
    emit changeDate(currentDate, true);
    return currentIndex();
}

void QCalendarView::keyPressEvent(QKeyEvent *event)
{
#ifdef QT_KEYPAD_NAVIGATION
    if (event->key() == Qt::Key_Select) {
        if (QApplication::keypadNavigationEnabled()) {
            if (!hasEditFocus()) {
                setEditFocus(true);
                return;
            }
        }
    } else if (event->key() == Qt::Key_Back) {
        if (QApplication::keypadNavigationEnabled() && hasEditFocus()) {
            if (qobject_cast<QCalendarModel *>(model())) {
                emit changeDate(origDate, true); //changes selection back to origDate, but doesn't activate
                setEditFocus(false);
                return;
            }
        }
    }
#endif

    if (!readOnly) {
        switch (event->key()) {
            case Qt::Key_Return:
            case Qt::Key_Enter:
            case Qt::Key_Select:
                emit editingFinished();
                return;
            default:
                break;
        }
    }
    QTableView::keyPressEvent(event);
}

#ifndef QT_NO_WHEELEVENT
void QCalendarView::wheelEvent(QWheelEvent *event)
{
    const int numDegrees = event->delta() / 8;
    const int numSteps = numDegrees / 15;
    const QModelIndex index = currentIndex();
    QDate currentDate = static_cast<QCalendarModel*>(model())->dateForCell(index.row(), index.column());
    currentDate = currentDate.addMonths(-numSteps);
    emit showDate(currentDate);
}
#endif

bool QCalendarView::event(QEvent *event)
{
#ifdef QT_KEYPAD_NAVIGATION
    if (event->type() == QEvent::FocusIn) {
        if (QCalendarModel *calendarModel = qobject_cast<QCalendarModel *>(model())) {
            origDate = calendarModel->m_date;
        }
    }
#endif

    return QTableView::event(event);
}

QDate QCalendarView::handleMouseEvent(QMouseEvent *event)
{
    QCalendarModel *calendarModel = qobject_cast<QCalendarModel *>(model());
    if (!calendarModel)
        return QDate();

    QPoint pos = event->pos();
    QModelIndex index = indexAt(pos);
    QDate date = calendarModel->dateForCell(index.row(), index.column());
    if (date.isValid() && date >= calendarModel->m_minimumDate
            && date <= calendarModel->m_maximumDate) {
        return date;
    }
    return QDate();
}

void QCalendarView::mouseDoubleClickEvent(QMouseEvent *event)
{
    QCalendarModel *calendarModel = qobject_cast<QCalendarModel *>(model());
    if (!calendarModel) {
        QTableView::mouseDoubleClickEvent(event);
        return;
    }

    if (readOnly)
        return;

    QDate date = handleMouseEvent(event);
    validDateClicked = false;
    if (date == calendarModel->m_date && !style()->styleHint(QStyle::SH_ItemView_ActivateItemOnSingleClick)) {
        emit editingFinished();
    }
}

void QCalendarView::mousePressEvent(QMouseEvent *event)
{
    QCalendarModel *calendarModel = qobject_cast<QCalendarModel *>(model());
    if (!calendarModel) {
        QTableView::mousePressEvent(event);
        return;
    }

    if (readOnly)
        return;

    if (event->button() != Qt::LeftButton)
        return;

    QDate date = handleMouseEvent(event);
    if (date.isValid()) {
        validDateClicked = true;
        int row = -1, col = -1;
        static_cast<QCalendarModel *>(model())->cellForDate(date, &row, &col);
        if (row != -1 && col != -1) {
            selectionModel()->setCurrentIndex(model()->index(row, col), QItemSelectionModel::NoUpdate);
        }
    } else {
        validDateClicked = false;
        event->ignore();
    }
}

void QCalendarView::mouseMoveEvent(QMouseEvent *event)
{
    QCalendarModel *calendarModel = qobject_cast<QCalendarModel *>(model());
    if (!calendarModel) {
        QTableView::mouseMoveEvent(event);
        return;
    }

    if (readOnly)
        return;

    if (validDateClicked) {
       QDate date = handleMouseEvent(event);
        if (date.isValid()) {
            int row = -1, col = -1;
            static_cast<QCalendarModel *>(model())->cellForDate(date, &row, &col);
            if (row != -1 && col != -1) {
                selectionModel()->setCurrentIndex(model()->index(row, col), QItemSelectionModel::NoUpdate);
            }
        }
    } else {
        event->ignore();
    }
}

void QCalendarView::mouseReleaseEvent(QMouseEvent *event)
{
    QCalendarModel *calendarModel = qobject_cast<QCalendarModel *>(model());
    if (!calendarModel) {
        QTableView::mouseReleaseEvent(event);
        return;
    }

    if (event->button() != Qt::LeftButton)
        return;

    if (readOnly)
        return;

    if (validDateClicked) {
        QDate date = handleMouseEvent(event);
        if (date.isValid()) {
            emit changeDate(date, true);
            emit clicked(date);
            if (style()->styleHint(QStyle::SH_ItemView_ActivateItemOnSingleClick))
                emit editingFinished();
        }
        validDateClicked = false;
    } else {
        event->ignore();
    }
}

class QCalendarDelegate : public QItemDelegate
{
    Q_OBJECT
public:
    QCalendarDelegate(QCalendarWidgetPrivate *w, QObject *parent = 0)
        : QItemDelegate(parent), calendarWidgetPrivate(w)
            { }
    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option,
                const QModelIndex &index) const;
    void paintCell(QPainter *painter, const QRect &rect, const QDate &date) const;

private:
    QCalendarWidgetPrivate *calendarWidgetPrivate;
    mutable QStyleOptionViewItemV4 storedOption;
};

//Private tool button class
class QCalToolButton: public QToolButton
{
public:
    QCalToolButton(QWidget * parent)
        : QToolButton(parent)
          {  }
protected:
    void paintEvent(QPaintEvent *e)
    {
        Q_UNUSED(e)

#ifndef Q_WS_MAC
        QStyleOptionToolButton opt;
        initStyleOption(&opt);

        if (opt.state & QStyle::State_MouseOver || isDown()) {
            //act as normal button
            setPalette(QPalette());
        } else {
            //set the highlight color for button text
            QPalette toolPalette = palette();
            toolPalette.setColor(QPalette::ButtonText, toolPalette.color(QPalette::HighlightedText));
            setPalette(toolPalette);
        }
#endif
        QToolButton::paintEvent(e);
    }
};

class QPrevNextCalButton : public QToolButton
{
    Q_OBJECT
public:
    QPrevNextCalButton(QWidget *parent) : QToolButton(parent) {}
protected:
    void paintEvent(QPaintEvent *) {
        QStylePainter painter(this);
        QStyleOptionToolButton opt;
        initStyleOption(&opt);
        opt.state &= ~QStyle::State_HasFocus;
        painter.drawComplexControl(QStyle::CC_ToolButton, opt);
    }
};

class QCalendarWidgetPrivate : public QWidgetPrivate
{
    Q_DECLARE_PUBLIC(QCalendarWidget)
public:
    QCalendarWidgetPrivate();

    void showMonth(int year, int month);
    void update();
    void paintCell(QPainter *painter, const QRect &rect, const QDate &date) const;

    void _q_slotShowDate(const QDate &date);
    void _q_slotChangeDate(const QDate &date);
    void _q_slotChangeDate(const QDate &date, bool changeMonth);
    void _q_editingFinished();
    void _q_monthChanged(QAction*);
    void _q_prevMonthClicked();
    void _q_nextMonthClicked();
    void _q_yearEditingFinished();
    void _q_yearClicked();

    void createNavigationBar(QWidget *widget);
    void updateButtonIcons();
    void updateMonthMenu();
    void updateMonthMenuNames();
    void updateNavigationBar();
    void updateCurrentPage(const QDate &newDate);
    inline QDate getCurrentDate();
    void setNavigatorEnabled(bool enable);

    QCalendarModel *m_model;
    QCalendarView *m_view;
    QCalendarDelegate *m_delegate;
    QItemSelectionModel *m_selection;
    QCalendarTextNavigator *m_navigator;
    bool m_dateEditEnabled;

    QToolButton *nextMonth;
    QToolButton *prevMonth;
    QCalToolButton *monthButton;
    QMenu *monthMenu;
    QMap<int, QAction *> monthToAction;
    QCalToolButton *yearButton;
    QSpinBox *yearEdit;
    QWidget *navBarBackground;
    QSpacerItem *spaceHolder;

    bool navBarVisible;
    mutable QSize cachedSizeHint;
    Qt::FocusPolicy oldFocusPolicy;
};

void QCalendarDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
            const QModelIndex &index) const
{
    QDate date = calendarWidgetPrivate->m_model->dateForCell(index.row(), index.column());
    if (date.isValid()) {
        storedOption = option;
        QRect rect = option.rect;
        calendarWidgetPrivate->paintCell(painter, rect, date);
    } else {
        QItemDelegate::paint(painter, option, index);
    }
}

void QCalendarDelegate::paintCell(QPainter *painter, const QRect &rect, const QDate &date) const
{
    storedOption.rect = rect;
    int row = -1;
    int col = -1;
    calendarWidgetPrivate->m_model->cellForDate(date, &row, &col);
    QModelIndex idx = calendarWidgetPrivate->m_model->index(row, col);
    QItemDelegate::paint(painter, storedOption, idx);
}

QCalendarWidgetPrivate::QCalendarWidgetPrivate()
    : QWidgetPrivate()
{
    m_model = 0;
    m_view = 0;
    m_delegate = 0;
    m_selection = 0;
    m_navigator = 0;
    m_dateEditEnabled = false;
    navBarVisible = true;
    oldFocusPolicy = Qt::StrongFocus;
}

void QCalendarWidgetPrivate::setNavigatorEnabled(bool enable)
{
    Q_Q(QCalendarWidget);

    bool navigatorEnabled = (m_navigator->widget() != 0);
    if (enable == navigatorEnabled)
        return;

    if (enable) {
        m_navigator->setWidget(q);
        q->connect(m_navigator, SIGNAL(dateChanged(QDate)),
                q, SLOT(_q_slotChangeDate(QDate)));
        q->connect(m_navigator, SIGNAL(editingFinished()),
                q, SLOT(_q_editingFinished()));
        m_view->installEventFilter(m_navigator);
    } else {
        m_navigator->setWidget(0);
        q->disconnect(m_navigator, SIGNAL(dateChanged(QDate)),
                q, SLOT(_q_slotChangeDate(QDate)));
        q->disconnect(m_navigator, SIGNAL(editingFinished()),
                q, SLOT(_q_editingFinished()));
        m_view->removeEventFilter(m_navigator);
    }
}

void QCalendarWidgetPrivate::createNavigationBar(QWidget *widget)
{
    Q_Q(QCalendarWidget);
    navBarBackground = new QWidget(widget);
    navBarBackground->setObjectName(QLatin1String("qt_calendar_navigationbar"));
    navBarBackground->setAutoFillBackground(true);
    navBarBackground->setBackgroundRole(QPalette::Highlight);

    prevMonth = new QPrevNextCalButton(navBarBackground);
    nextMonth = new QPrevNextCalButton(navBarBackground);
    prevMonth->setAutoRaise(true);
    nextMonth->setAutoRaise(true);
    prevMonth->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);
    nextMonth->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);
    nextMonth->setAutoRaise(true);
    updateButtonIcons();
    prevMonth->setAutoRepeat(true);
    nextMonth->setAutoRepeat(true);

    monthButton = new QCalToolButton(navBarBackground);
    monthButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);
    monthButton->setAutoRaise(true);
    monthButton->setPopupMode(QToolButton::InstantPopup);
    monthMenu = new QMenu(monthButton);
    for (int i = 1; i <= 12; i++) {
        QString monthName(q->locale().standaloneMonthName(i, QLocale::LongFormat));
        QAction *act = monthMenu->addAction(monthName);
        act->setData(i);
        monthToAction[i] = act;
    }
    monthButton->setMenu(monthMenu);
    yearButton = new QCalToolButton(navBarBackground);
    yearButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);
    yearButton->setAutoRaise(true);
    yearEdit = new QSpinBox(navBarBackground);

    QFont font = q->font();
    font.setBold(true);
    monthButton->setFont(font);
    yearButton->setFont(font);
    yearEdit->setFrame(false);
    yearEdit->setMinimum(m_model->m_minimumDate.year());
    yearEdit->setMaximum(m_model->m_maximumDate.year());
    yearEdit->hide();
    spaceHolder = new QSpacerItem(0,0);

    QHBoxLayout *headerLayout = new QHBoxLayout;
    headerLayout->setMargin(0);
    headerLayout->setSpacing(0);
    headerLayout->addWidget(prevMonth);
    headerLayout->insertStretch(headerLayout->count());
    headerLayout->addWidget(monthButton);
    headerLayout->addItem(spaceHolder);
    headerLayout->addWidget(yearButton);
    headerLayout->insertStretch(headerLayout->count());
    headerLayout->addWidget(nextMonth);
    navBarBackground->setLayout(headerLayout);

    yearEdit->setFocusPolicy(Qt::StrongFocus);
    prevMonth->setFocusPolicy(Qt::NoFocus);
    nextMonth->setFocusPolicy(Qt::NoFocus);
    yearButton->setFocusPolicy(Qt::NoFocus);
    monthButton->setFocusPolicy(Qt::NoFocus);

    //set names for the header controls.
    prevMonth->setObjectName(QLatin1String("qt_calendar_prevmonth"));
    nextMonth->setObjectName(QLatin1String("qt_calendar_nextmonth"));
    monthButton->setObjectName(QLatin1String("qt_calendar_monthbutton"));
    yearButton->setObjectName(QLatin1String("qt_calendar_yearbutton"));
    yearEdit->setObjectName(QLatin1String("qt_calendar_yearedit"));

    updateMonthMenu();
    showMonth(m_model->m_date.year(), m_model->m_date.month());
}

void QCalendarWidgetPrivate::updateButtonIcons()
{
    Q_Q(QCalendarWidget);
    prevMonth->setIcon(q->style()->standardIcon(q->isRightToLeft() ? QStyle::SP_ArrowRight : QStyle::SP_ArrowLeft, 0, q));
    nextMonth->setIcon(q->style()->standardIcon(q->isRightToLeft() ? QStyle::SP_ArrowLeft : QStyle::SP_ArrowRight, 0, q));
}

void QCalendarWidgetPrivate::updateMonthMenu()
{
    int beg = 1, end = 12;
    bool prevEnabled = true;
    bool nextEnabled = true;
    if (m_model->m_shownYear == m_model->m_minimumDate.year()) {
        beg = m_model->m_minimumDate.month();
        if (m_model->m_shownMonth == m_model->m_minimumDate.month())
            prevEnabled = false;
    }
    if (m_model->m_shownYear == m_model->m_maximumDate.year()) {
        end = m_model->m_maximumDate.month();
        if (m_model->m_shownMonth == m_model->m_maximumDate.month())
            nextEnabled = false;
    }
    prevMonth->setEnabled(prevEnabled);
    nextMonth->setEnabled(nextEnabled);
    for (int i = 1; i <= 12; i++) {
        bool monthEnabled = true;
        if (i < beg || i > end)
            monthEnabled = false;
        monthToAction[i]->setEnabled(monthEnabled);
    }
}

void QCalendarWidgetPrivate::updateMonthMenuNames()
{
    Q_Q(QCalendarWidget);

    for (int i = 1; i <= 12; i++) {
        QString monthName(q->locale().standaloneMonthName(i, QLocale::LongFormat));
        monthToAction[i]->setText(monthName);
    }
}

void QCalendarWidgetPrivate::updateCurrentPage(const QDate &date)
{
    Q_Q(QCalendarWidget);

    QDate newDate = date;
    QDate minDate = q->minimumDate();
    QDate maxDate = q->maximumDate();
    if (minDate.isValid()&& minDate.daysTo(newDate) < 0)
        newDate = minDate;
    if (maxDate.isValid()&& maxDate.daysTo(newDate) > 0)
        newDate = maxDate;
    showMonth(newDate.year(), newDate.month());
    int row = -1, col = -1;
    m_model->cellForDate(newDate, &row, &col);
    if (row != -1 && col != -1)
    {
        m_view->selectionModel()->setCurrentIndex(m_model->index(row, col),
                                                  QItemSelectionModel::NoUpdate);
    }
}

void QCalendarWidgetPrivate::_q_monthChanged(QAction *act)
{
    monthButton->setText(act->text());
    QDate currentDate = getCurrentDate();
    QDate newDate = currentDate.addMonths(act->data().toInt()-currentDate.month());
    updateCurrentPage(newDate);
}

QDate QCalendarWidgetPrivate::getCurrentDate()
{
    QModelIndex index = m_view->currentIndex();
    return m_model->dateForCell(index.row(), index.column());
}

void QCalendarWidgetPrivate::_q_prevMonthClicked()
{
    QDate currentDate = getCurrentDate().addMonths(-1);
    updateCurrentPage(currentDate);
}

void QCalendarWidgetPrivate::_q_nextMonthClicked()
{
    QDate currentDate = getCurrentDate().addMonths(1);
    updateCurrentPage(currentDate);
}

void QCalendarWidgetPrivate::_q_yearEditingFinished()
{
    Q_Q(QCalendarWidget);
    yearButton->setText(yearEdit->text());
    yearEdit->hide();
    q->setFocusPolicy(oldFocusPolicy);
    qApp->removeEventFilter(q);
    spaceHolder->changeSize(0, 0);
    yearButton->show();
    QDate currentDate = getCurrentDate();
    currentDate = currentDate.addYears(yearEdit->text().toInt() - currentDate.year());
    updateCurrentPage(currentDate);
}

void QCalendarWidgetPrivate::_q_yearClicked()
{
    Q_Q(QCalendarWidget);
    //show the spinbox on top of the button
    yearEdit->setGeometry(yearButton->x(), yearButton->y(),
                          yearEdit->sizeHint().width(), yearButton->height());
    spaceHolder->changeSize(yearButton->width(), 0);
    yearButton->hide();
    oldFocusPolicy = q->focusPolicy();
    q->setFocusPolicy(Qt::NoFocus);
    yearEdit->show();
    qApp->installEventFilter(q);
    yearEdit->raise();
    yearEdit->selectAll();
    yearEdit->setFocus(Qt::MouseFocusReason);
}

void QCalendarWidgetPrivate::showMonth(int year, int month)
{
    if (m_model->m_shownYear == year && m_model->m_shownMonth == month)
        return;
    Q_Q(QCalendarWidget);
    m_model->showMonth(year, month);
    updateNavigationBar();
    emit q->currentPageChanged(year, month);
    m_view->internalUpdate();
    cachedSizeHint = QSize();
    update();
    updateMonthMenu();
}

void QCalendarWidgetPrivate::updateNavigationBar()
{
    Q_Q(QCalendarWidget);

    QString monthName = q->locale().standaloneMonthName(m_model->m_shownMonth, QLocale::LongFormat);

    monthButton->setText(monthName);
    yearButton->setText(QString::number(m_model->m_shownYear));
    yearEdit->setValue(m_model->m_shownYear);
}

void QCalendarWidgetPrivate::update()
{
    QDate currentDate = m_model->m_date;
    int row, column;
    m_model->cellForDate(currentDate, &row, &column);
    QModelIndex idx;
    m_selection->clear();
    if (row != -1 && column != -1) {
        idx = m_model->index(row, column);
        m_selection->setCurrentIndex(idx, QItemSelectionModel::SelectCurrent);
    }
}

void QCalendarWidgetPrivate::paintCell(QPainter *painter, const QRect &rect, const QDate &date) const
{
    Q_Q(const QCalendarWidget);
    q->paintCell(painter, rect, date);
}

void QCalendarWidgetPrivate::_q_slotShowDate(const QDate &date)
{
    updateCurrentPage(date);
}

void QCalendarWidgetPrivate::_q_slotChangeDate(const QDate &date)
{
    _q_slotChangeDate(date, true);
}

void QCalendarWidgetPrivate::_q_slotChangeDate(const QDate &date, bool changeMonth)
{
    QDate oldDate = m_model->m_date;
    m_model->setDate(date);
    QDate newDate = m_model->m_date;
    if (changeMonth)
        showMonth(newDate.year(), newDate.month());
    if (oldDate != newDate) {
        update();
        Q_Q(QCalendarWidget);
        m_navigator->setDate(newDate);
        emit q->selectionChanged();
    }
}

void QCalendarWidgetPrivate::_q_editingFinished()
{
    Q_Q(QCalendarWidget);
    emit q->activated(m_model->m_date);
}

/*!
    \class QCalendarWidget
    \brief The QCalendarWidget class provides a monthly based
    calendar widget allowing the user to select a date.
    \since 4.2

    \ingroup advanced

    \image cleanlooks-calendarwidget.png

    The widget is initialized with the current month and year, but
    QCalendarWidget provides several public slots to change the year
    and month that is shown.

    By default, today's date is selected, and the user can select a
    date using both mouse and keyboard. The currently selected date
    can be retrieved using the selectedDate() function. It is
    possible to constrain the user selection to a given date range by
    setting the minimumDate and maximumDate properties.
    Alternatively, both properties can be set in one go using the
    setDateRange() convenience slot. Set the \l selectionMode
    property to NoSelection to prohibit the user from selecting at
    all. Note that a date also can be selected programmatically using
    the setSelectedDate() slot.

    The currently displayed month and year can be retrieved using the
    monthShown() and yearShown() functions, respectively.

    A newly created calendar widget uses abbreviated day names, and
    both Saturdays and Sundays are marked in red. The calendar grid is
    not visible. The week numbers are displayed, and the first column
    day is Sunday.

    The notation of the days can be altered to a single letter
    abbreviations ("M" for "Monday") by setting the
    horizontalHeaderFormat property to
    QCalendarWidget::SingleLetterDayNames. Setting the same property
    to QCalendarWidget::LongDayNames makes the header display the
    complete day names. The week numbers can be removed by setting
    the verticalHeaderFormat property to
    QCalendarWidget::NoVerticalHeader.  The calendar grid can be
    turned on by setting the gridVisible property to true using the
    setGridVisible() function:

    \table
    \row \o
        \image qcalendarwidget-grid.png
    \row \o
        \snippet doc/src/snippets/code/src_gui_widgets_qcalendarwidget.cpp 0
    \endtable

    Finally, the day in the first column can be altered using the
    setFirstDayOfWeek() function.

    The QCalendarWidget class also provides three signals,
    selectionChanged(), activated() and currentPageChanged() making it
    possible to respond to user interaction.

    The rendering of the headers, weekdays or single days can be
    largely customized by setting QTextCharFormat's for some special
    weekday, a special date or for the rendering of the headers.

    Only a subset of the properties in QTextCharFormat are used by the
    calendar widget. Currently, the foreground, background and font
    properties are used to determine the rendering of individual cells
    in the widget.

    \sa QDate, QDateEdit, QTextCharFormat
*/

/*!
    \enum QCalendarWidget::SelectionMode

    This enum describes the types of selection offered to the user for
    selecting dates in the calendar.

    \value NoSelection      Dates cannot be selected.
    \value SingleSelection  Single dates can be selected.

    \sa selectionMode
*/

/*!
    Constructs a calendar widget with the given \a parent.

    The widget is initialized with the current month and year, and the
    currently selected date is today.

    \sa setCurrentPage()
*/
QCalendarWidget::QCalendarWidget(QWidget *parent)
    : QWidget(*new QCalendarWidgetPrivate, parent, 0)
{
    Q_D(QCalendarWidget);

    setAutoFillBackground(true);
    setBackgroundRole(QPalette::Window);

    QVBoxLayout *layoutV = new QVBoxLayout(this);
    layoutV->setMargin(0);
    d->m_model = new QCalendarModel(this);
    QTextCharFormat fmt;
    fmt.setForeground(QBrush(Qt::red));
    d->m_model->m_dayFormats.insert(Qt::Saturday, fmt);
    d->m_model->m_dayFormats.insert(Qt::Sunday, fmt);
    d->m_view = new QCalendarView(this);
    d->m_view->setObjectName(QLatin1String("qt_calendar_calendarview"));
    d->m_view->setModel(d->m_model);
    d->m_model->setView(d->m_view);
    d->m_view->setSelectionBehavior(QAbstractItemView::SelectItems);
    d->m_view->setSelectionMode(QAbstractItemView::SingleSelection);
    d->m_view->horizontalHeader()->setResizeMode(QHeaderView::Stretch);
    d->m_view->horizontalHeader()->setClickable(false);
    d->m_view->verticalHeader()->setResizeMode(QHeaderView::Stretch);
    d->m_view->verticalHeader()->setClickable(false);
    d->m_selection = d->m_view->selectionModel();
    d->createNavigationBar(this);
    d->m_view->setFrameStyle(QFrame::NoFrame);
    d->m_delegate = new QCalendarDelegate(d, this);
    d->m_view->setItemDelegate(d->m_delegate);
    d->update();
    d->updateNavigationBar();
    setFocusPolicy(Qt::StrongFocus);
    setFocusProxy(d->m_view);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    connect(d->m_view, SIGNAL(showDate(QDate)),
            this, SLOT(_q_slotShowDate(QDate)));
    connect(d->m_view, SIGNAL(changeDate(QDate,bool)),
            this, SLOT(_q_slotChangeDate(QDate,bool)));
    connect(d->m_view, SIGNAL(clicked(QDate)),
            this, SIGNAL(clicked(QDate)));
    connect(d->m_view, SIGNAL(editingFinished()),
            this, SLOT(_q_editingFinished()));

    connect(d->prevMonth, SIGNAL(clicked(bool)),
            this, SLOT(_q_prevMonthClicked()));
    connect(d->nextMonth, SIGNAL(clicked(bool)),
            this, SLOT(_q_nextMonthClicked()));
    connect(d->yearButton, SIGNAL(clicked(bool)),
            this, SLOT(_q_yearClicked()));
    connect(d->monthMenu, SIGNAL(triggered(QAction*)),
            this, SLOT(_q_monthChanged(QAction*)));
    connect(d->yearEdit, SIGNAL(editingFinished()),
            this, SLOT(_q_yearEditingFinished()));

    layoutV->setMargin(0);
    layoutV->setSpacing(0);
    layoutV->addWidget(d->navBarBackground);
    layoutV->addWidget(d->m_view);

    d->m_navigator = new QCalendarTextNavigator(this);
    setDateEditEnabled(true);
}

/*!
   Destroys the calendar widget.
*/
QCalendarWidget::~QCalendarWidget()
{
}

/*!
   \reimp
*/
QSize QCalendarWidget::sizeHint() const
{
    return minimumSizeHint();
}

/*!
   \reimp
*/
QSize QCalendarWidget::minimumSizeHint() const
{
    Q_D(const QCalendarWidget);
    if (d->cachedSizeHint.isValid())
        return d->cachedSizeHint;

    ensurePolished();

    int w = 0;
    int h = 0;

    int end = 53;
    int rows = 7;
    int cols = 8;

    const int marginH = (style()->pixelMetric(QStyle::PM_FocusFrameHMargin) + 1) * 2;

    if (horizontalHeaderFormat() == QCalendarWidget::NoHorizontalHeader) {
        rows = 6;
    } else {
        for (int i = 1; i <= 7; i++) {
            QFontMetrics fm(d->m_model->formatForCell(0, i).font());
            w = qMax(w, fm.width(d->m_model->dayName(d->m_model->dayOfWeekForColumn(i))) + marginH);
            h = qMax(h, fm.height());
        }
    }

    if (verticalHeaderFormat() == QCalendarWidget::NoVerticalHeader) {
        cols = 7;
    } else {
        for (int i = 1; i <= 6; i++) {
            QFontMetrics fm(d->m_model->formatForCell(i, 0).font());
            for (int j = 1; j < end; j++)
                w = qMax(w, fm.width(QString::number(j)) + marginH);
            h = qMax(h, fm.height());
        }
    }

    QFontMetrics fm(d->m_model->formatForCell(1, 1).font());
    for (int i = 1; i <= end; i++) {
        w = qMax(w, fm.width(QString::number(i)) + marginH);
        h = qMax(h, fm.height());
    }

    if (d->m_view->showGrid()) {
        // hardcoded in tableview
        w += 1;
        h += 1;
    }

    w += 1; // default column span

    h = qMax(h, d->m_view->verticalHeader()->minimumSectionSize());
    w = qMax(w, d->m_view->horizontalHeader()->minimumSectionSize());

    //add the size of the header.
    QSize headerSize(0, 0);
    if (d->navBarVisible) {
        int headerH = d->navBarBackground->sizeHint().height();
        int headerW = 0;

        headerW += d->prevMonth->sizeHint().width();
        headerW += d->nextMonth->sizeHint().width();

        QFontMetrics fm = d->monthButton->fontMetrics();
        int monthW = 0;
        for (int i = 1; i < 12; i++) {
            QString monthName = locale().standaloneMonthName(i, QLocale::LongFormat);
            monthW = qMax(monthW, fm.boundingRect(monthName).width());
        }
        const int buttonDecoMargin = d->monthButton->sizeHint().width() - fm.boundingRect(d->monthButton->text()).width();
        headerW += monthW + buttonDecoMargin;

        fm = d->yearButton->fontMetrics();
        headerW += fm.boundingRect(QLatin1String("5555")).width() + buttonDecoMargin;

        headerSize = QSize(headerW, headerH);
    }
    w *= cols;
    w = qMax(headerSize.width(), w);
    h = (h * rows) + headerSize.height();
    d->cachedSizeHint = QSize(w, h);
    return d->cachedSizeHint;
}

/*!
    Paints the cell specified by the given \a date, using the given \a painter and \a rect.
*/

void QCalendarWidget::paintCell(QPainter *painter, const QRect &rect, const QDate &date) const
{
    Q_D(const QCalendarWidget);
    d->m_delegate->paintCell(painter, rect, date);
}

/*!
    \property QCalendarWidget::selectedDate
    \brief the currently selected date.

    The selected date must be within the date range specified by the
    minimumDate and maximumDate properties. By default, the selected
    date is the current date.

    \sa setDateRange()
*/

QDate QCalendarWidget::selectedDate() const
{
    Q_D(const QCalendarWidget);
    return d->m_model->m_date;
}

void QCalendarWidget::setSelectedDate(const QDate &date)
{
    Q_D(QCalendarWidget);
    if (d->m_model->m_date == date && date == d->getCurrentDate())
        return;

    if (!date.isValid())
        return;

    d->m_model->setDate(date);
    d->update();
    QDate newDate = d->m_model->m_date;
    d->showMonth(newDate.year(), newDate.month());
    emit selectionChanged();
}

/*!
    Returns the year of the currently displayed month. Months are
    numbered from 1 to 12.

    \sa monthShown(), setCurrentPage()
*/

int QCalendarWidget::yearShown() const
{
    Q_D(const QCalendarWidget);
    return d->m_model->m_shownYear;
}

/*!
    Returns the currently displayed month. Months are numbered from 1 to
    12.

    \sa yearShown(), setCurrentPage()
*/

int QCalendarWidget::monthShown() const
{
    Q_D(const QCalendarWidget);
    return d->m_model->m_shownMonth;
}

/*!
    Displays the given \a month of the given \a year without changing
    the selected date. Use the setSelectedDate() function to alter the
    selected date.

    The currently displayed month and year can be retrieved using the
    monthShown() and yearShown() functions respectively.

    \sa yearShown(), monthShown(), showPreviousMonth(), showNextMonth(),
    showPreviousYear(), showNextYear()
*/

void QCalendarWidget::setCurrentPage(int year, int month)
{
    Q_D(QCalendarWidget);
    QDate currentDate = d->getCurrentDate();
    int day = currentDate.day();
    int daysInMonths = QDate(year, month, 1).daysInMonth();
    if (day > daysInMonths)
        day = daysInMonths;

    d->showMonth(year, month);

    QDate newDate(year, month, day);
    int row = -1, col = -1;
    d->m_model->cellForDate(newDate, &row, &col);
    if (row != -1 && col != -1) {
        d->m_view->selectionModel()->setCurrentIndex(d->m_model->index(row, col),
                                                  QItemSelectionModel::NoUpdate);
    }
}

/*!
    Shows the next month relative to the currently displayed
    month. Note that the selected date is not changed.

    \sa showPreviousMonth(), setCurrentPage(), setSelectedDate()
*/

void QCalendarWidget::showNextMonth()
{
    int year = yearShown();
    int month = monthShown();
    if (month == 12) {
        ++year;
        month = 1;
    } else {
        ++month;
    }
    setCurrentPage(year, month);
}

/*!
    Shows the previous month relative to the currently displayed
    month. Note that the selected date is not changed.

    \sa showNextMonth(), setCurrentPage(), setSelectedDate()
*/

void QCalendarWidget::showPreviousMonth()
{
    int year = yearShown();
    int month = monthShown();
    if (month == 1) {
        --year;
        month = 12;
    } else {
        --month;
    }
    setCurrentPage(year, month);
}

/*!
    Shows the currently displayed month in the \e next year relative
    to the currently displayed year. Note that the selected date is
    not changed.

    \sa showPreviousYear(), setCurrentPage(), setSelectedDate()
*/

void QCalendarWidget::showNextYear()
{
    int year = yearShown();
    int month = monthShown();
    ++year;
    setCurrentPage(year, month);
}

/*!
    Shows the currently displayed month in the \e previous year
    relative to the currently displayed year. Note that the selected
    date is not changed.

    \sa showNextYear(), setCurrentPage(), setSelectedDate()
*/

void QCalendarWidget::showPreviousYear()
{
    int year = yearShown();
    int month = monthShown();
    --year;
    setCurrentPage(year, month);
}

/*!
    Shows the month of the selected date.

    \sa selectedDate(), setCurrentPage()
*/
void QCalendarWidget::showSelectedDate()
{
    QDate currentDate = selectedDate();
    setCurrentPage(currentDate.year(), currentDate.month());
}

/*!
    Shows the month of the today's date.

    \sa selectedDate(), setCurrentPage()
*/
void QCalendarWidget::showToday()
{
    QDate currentDate = QDate::currentDate();
    setCurrentPage(currentDate.year(), currentDate.month());
}

/*!
    \property QCalendarWidget::minimumDate
    \brief the minimum date of the currently specified date range.

    The user will not be able to select a date that is before the
    currently set minimum date.

    \table
    \row
    \o \image qcalendarwidget-minimum.png
    \row
    \o
    \snippet doc/src/snippets/code/src_gui_widgets_qcalendarwidget.cpp 1
    \endtable

    By default, the minimum date is the earliest date that the QDate
    class can handle.

    When setting a minimum date, the maximumDate and selectedDate
    properties are adjusted if the selection range becomes invalid. If
    the provided date is not a valid QDate object, the
    setMinimumDate() function does nothing.

    \sa setDateRange()
*/

QDate QCalendarWidget::minimumDate() const
{
    Q_D(const QCalendarWidget);
    return d->m_model->m_minimumDate;
}

void QCalendarWidget::setMinimumDate(const QDate &date)
{
    Q_D(QCalendarWidget);
    if (!date.isValid() || d->m_model->m_minimumDate == date)
        return;

    QDate oldDate = d->m_model->m_date;
    d->m_model->setMinimumDate(date);
    d->yearEdit->setMinimum(d->m_model->m_minimumDate.year());
    d->updateMonthMenu();
    QDate newDate = d->m_model->m_date;
    if (oldDate != newDate) {
        d->update();
        d->showMonth(newDate.year(), newDate.month());
        d->m_navigator->setDate(newDate);
        emit selectionChanged();
    }
}

/*!
    \property QCalendarWidget::maximumDate
    \brief the maximum date of the currently specified date range.

    The user will not be able to select a date which is after the
    currently set maximum date.

    \table
    \row
    \o \image qcalendarwidget-maximum.png
    \row
    \o
    \snippet doc/src/snippets/code/src_gui_widgets_qcalendarwidget.cpp 2
    \endtable

    By default, the maximum date is the last day the QDate class can
    handle.

    When setting a maximum date, the minimumDate and selectedDate
    properties are adjusted if the selection range becomes invalid. If
    the provided date is not a valid QDate object, the
    setMaximumDate() function does nothing.

    \sa setDateRange()
*/

QDate QCalendarWidget::maximumDate() const
{
    Q_D(const QCalendarWidget);
    return d->m_model->m_maximumDate;
}

void QCalendarWidget::setMaximumDate(const QDate &date)
{
    Q_D(QCalendarWidget);
    if (!date.isValid() || d->m_model->m_maximumDate == date)
        return;

    QDate oldDate = d->m_model->m_date;
    d->m_model->setMaximumDate(date);
    d->yearEdit->setMaximum(d->m_model->m_maximumDate.year());
    d->updateMonthMenu();
    QDate newDate = d->m_model->m_date;
    if (oldDate != newDate) {
        d->update();
        d->showMonth(newDate.year(), newDate.month());
        d->m_navigator->setDate(newDate);
        emit selectionChanged();
    }
}

/*!
    Defines a date range by setting the minimumDate and maximumDate
    properties.

    The date range restricts the user selection, i.e. the user can
    only select dates within the specified date range. Note that

    \snippet doc/src/snippets/code/src_gui_widgets_qcalendarwidget.cpp 3

    is analogous to

    \snippet doc/src/snippets/code/src_gui_widgets_qcalendarwidget.cpp 4

    If either the \a min or \a max parameters are not valid QDate
    objects, this function does nothing.

    \sa setMinimumDate(), setMaximumDate()
*/

void QCalendarWidget::setDateRange(const QDate &min, const QDate &max)
{
    Q_D(QCalendarWidget);
    if (d->m_model->m_minimumDate == min && d->m_model->m_maximumDate == max)
        return;
    if (!min.isValid() || !max.isValid())
        return;

    QDate oldDate = d->m_model->m_date;
    d->m_model->setRange(min, max);
    d->yearEdit->setMinimum(d->m_model->m_minimumDate.year());
    d->yearEdit->setMaximum(d->m_model->m_maximumDate.year());
    d->updateMonthMenu();
    QDate newDate = d->m_model->m_date;
    if (oldDate != newDate) {
        d->update();
        d->showMonth(newDate.year(), newDate.month());
        d->m_navigator->setDate(newDate);
        emit selectionChanged();
    }
}


/*! \enum QCalendarWidget::HorizontalHeaderFormat

    This enum type defines the various formats the horizontal header can display.

    \value SingleLetterDayNames The header displays a single letter abbreviation for day names (e.g. M for Monday).
    \value ShortDayNames The header displays a short abbreviation for day names (e.g. Mon for Monday).
    \value LongDayNames The header displays complete day names (e.g. Monday).
    \value NoHorizontalHeader The header is hidden.

    \sa horizontalHeaderFormat(), VerticalHeaderFormat
*/

/*!
    \property QCalendarWidget::horizontalHeaderFormat
    \brief the format of the horizontal header.

    The default value is QCalendarWidget::ShortDayNames.
*/

void QCalendarWidget::setHorizontalHeaderFormat(QCalendarWidget::HorizontalHeaderFormat format)
{
    Q_D(QCalendarWidget);
    if (d->m_model->m_horizontalHeaderFormat == format)
        return;

    d->m_model->setHorizontalHeaderFormat(format);
    d->cachedSizeHint = QSize();
    d->m_view->viewport()->update();
    d->m_view->updateGeometry();
}

QCalendarWidget::HorizontalHeaderFormat QCalendarWidget::horizontalHeaderFormat() const
{
    Q_D(const QCalendarWidget);
    return d->m_model->m_horizontalHeaderFormat;
}


/*! 
    \enum QCalendarWidget::VerticalHeaderFormat

    This enum type defines the various formats the vertical header can display.

    \value ISOWeekNumbers The header displays ISO week numbers as described by \l QDate::weekNumber().
    \value NoVerticalHeader The header is hidden.

    \sa verticalHeaderFormat(), HorizontalHeaderFormat
*/

/*!
    \property QCalendarWidget::verticalHeaderFormat
    \brief the format of the vertical header.

    The default value is QCalendarWidget::ISOWeekNumber.
*/

QCalendarWidget::VerticalHeaderFormat QCalendarWidget::verticalHeaderFormat() const
{
    Q_D(const QCalendarWidget);
    bool shown = d->m_model->weekNumbersShown();
    if (shown)
        return QCalendarWidget::ISOWeekNumbers;
    return QCalendarWidget::NoVerticalHeader;
}

void QCalendarWidget::setVerticalHeaderFormat(QCalendarWidget::VerticalHeaderFormat format)
{
    Q_D(QCalendarWidget);
    bool show = false;
    if (format == QCalendarWidget::ISOWeekNumbers)
        show = true;
    if (d->m_model->weekNumbersShown() == show)
        return;
    d->m_model->setWeekNumbersShown(show);
    d->cachedSizeHint = QSize();
    d->m_view->viewport()->update();
    d->m_view->updateGeometry();
}

/*!
    \property QCalendarWidget::gridVisible
    \brief whether the table grid is displayed.

    \table
    \row
        \o \inlineimage qcalendarwidget-grid.png
    \row
        \o
        \snippet doc/src/snippets/code/src_gui_widgets_qcalendarwidget.cpp 5
    \endtable

    The default value is false.
*/

bool QCalendarWidget::isGridVisible() const
{
    Q_D(const QCalendarWidget);
    return d->m_view->showGrid();
}

void QCalendarWidget::setGridVisible(bool show)
{
    Q_D(QCalendarWidget);
    d->m_view->setShowGrid(show);
    d->cachedSizeHint = QSize();
    d->m_view->viewport()->update();
    d->m_view->updateGeometry();
}

/*!
    \property QCalendarWidget::selectionMode
    \brief the type of selection the user can make in the calendar

    When this property is set to SingleSelection, the user can select a date
    within the minimum and maximum allowed dates, using either the mouse or
    the keyboard.

    When the property is set to NoSelection, the user will be unable to select
    dates, but they can still be selected programmatically. Note that the date
    that is selected when the property is set to NoSelection will still be
    the selected date of the calendar.

    The default value is SingleSelection.
*/

QCalendarWidget::SelectionMode QCalendarWidget::selectionMode() const
{
    Q_D(const QCalendarWidget);
    return d->m_view->readOnly ? QCalendarWidget::NoSelection : QCalendarWidget::SingleSelection;
}

void QCalendarWidget::setSelectionMode(SelectionMode mode)
{
    Q_D(QCalendarWidget);
    d->m_view->readOnly = (mode == QCalendarWidget::NoSelection);
    d->setNavigatorEnabled(isDateEditEnabled() && (selectionMode() != QCalendarWidget::NoSelection));
    d->update();
}

/*!
    \property QCalendarWidget::firstDayOfWeek
    \brief a value identifying the day displayed in the first column.

    By default, the day displayed in the first column is Sunday
*/

void QCalendarWidget::setFirstDayOfWeek(Qt::DayOfWeek dayOfWeek)
{
    Q_D(QCalendarWidget);
    if ((Qt::DayOfWeek)d->m_model->firstColumnDay() == dayOfWeek)
        return;

    d->m_model->setFirstColumnDay(dayOfWeek);
    d->update();
}

Qt::DayOfWeek QCalendarWidget::firstDayOfWeek() const
{
    Q_D(const QCalendarWidget);
    return (Qt::DayOfWeek)d->m_model->firstColumnDay();
}

/*!
    Returns the text char format for rendering the header.
*/
QTextCharFormat QCalendarWidget::headerTextFormat() const
{
    Q_D(const QCalendarWidget);
    return d->m_model->m_headerFormat;
}

/*!
    Sets the text char format for rendering the header to \a format.
    If you also set a weekday text format, this format's foreground and
    background color will take precedence over the header's format.
    The other formatting information will still be decided by
    the header's format.
*/
void QCalendarWidget::setHeaderTextFormat(const QTextCharFormat &format)
{
    Q_D(QCalendarWidget);
    d->m_model->m_headerFormat = format;
    d->cachedSizeHint = QSize();
    d->m_view->viewport()->update();
    d->m_view->updateGeometry();
}

/*!
    Returns the text char format for rendering of day in the week \a dayOfWeek.

    \sa headerTextFormat()
*/
QTextCharFormat QCalendarWidget::weekdayTextFormat(Qt::DayOfWeek dayOfWeek) const
{
    Q_D(const QCalendarWidget);
    return d->m_model->m_dayFormats.value(dayOfWeek);
}

/*!
    Sets the text char format for rendering of day in the week \a dayOfWeek to \a format.
    The format will take precedence over the header format in case of foreground
    and background color. Other text formatting information is taken from the headers format.

    \sa setHeaderTextFormat()
*/
void QCalendarWidget::setWeekdayTextFormat(Qt::DayOfWeek dayOfWeek, const QTextCharFormat &format)
{
    Q_D(QCalendarWidget);
    d->m_model->m_dayFormats[dayOfWeek] = format;
    d->cachedSizeHint = QSize();
    d->m_view->viewport()->update();
    d->m_view->updateGeometry();
}

/*!
    Returns a QMap from QDate to QTextCharFormat showing all dates
    that use a special format that alters their rendering.
*/
QMap<QDate, QTextCharFormat> QCalendarWidget::dateTextFormat() const
{
    Q_D(const QCalendarWidget);
    return d->m_model->m_dateFormats;
}

/*!
    Returns a QTextCharFormat for \a date. The char format can be be
    empty if the date is not renderd specially.
*/
QTextCharFormat QCalendarWidget::dateTextFormat(const QDate &date) const
{
    Q_D(const QCalendarWidget);
    return d->m_model->m_dateFormats.value(date);
}

/*!
    Sets the format used to render the given \a date to that specified by \a format.

    If \a date is null, all date formats are cleared.
*/
void QCalendarWidget::setDateTextFormat(const QDate &date, const QTextCharFormat &format)
{
    Q_D(QCalendarWidget);
    if (date.isNull())
        d->m_model->m_dateFormats.clear();
    else
        d->m_model->m_dateFormats[date] = format;
    d->m_view->viewport()->update();
    d->m_view->updateGeometry();
}

/*!
    \property QCalendarWidget::dateEditEnabled
    \brief whether the date edit popup is enabled
    \since 4.3

    If this property is enabled, pressing a non-modifier key will cause a
    date edit to popup if the calendar widget has focus, allowing the user
    to specify a date in the form specified by the current locale.

    By default, this property is enabled.

    The date edit is simpler in appearance than QDateEdit, but allows the
    user to navigate between fields using the left and right cursor keys,
    increment and decrement individual fields using the up and down cursor
    keys, and enter values directly using the number keys.

    \sa QCalendarWidget::dateEditAcceptDelay
*/
bool QCalendarWidget::isDateEditEnabled() const
{
    Q_D(const QCalendarWidget);
    return d->m_dateEditEnabled;
}

void QCalendarWidget::setDateEditEnabled(bool enable)
{
    Q_D(QCalendarWidget);
    if (isDateEditEnabled() == enable)
        return;

    d->m_dateEditEnabled = enable;

    d->setNavigatorEnabled(enable && (selectionMode() != QCalendarWidget::NoSelection));
}

/*!
    \property QCalendarWidget::dateEditAcceptDelay
    \brief the time an inactive date edit is shown before its contents are accepted
    \since 4.3

    If the calendar widget's \l{dateEditEnabled}{date edit is enabled}, this
    property specifies the amount of time (in millseconds) that the date edit
    remains open after the most recent user input. Once this time has elapsed,
    the date specified in the date edit is accepted and the popup is closed.

    By default, the delay is defined to be 1500 milliseconds (1.5 seconds).
*/
int QCalendarWidget::dateEditAcceptDelay() const
{
    Q_D(const QCalendarWidget);
    return d->m_navigator->dateEditAcceptDelay();
}

void QCalendarWidget::setDateEditAcceptDelay(int delay)
{
    Q_D(QCalendarWidget);
    d->m_navigator->setDateEditAcceptDelay(delay);
}

/*!
    \since 4.4

    Updates the cell specified by the given \a date unless updates
    are disabled or the cell is hidden.

    \sa updateCells(), yearShown(), monthShown()
*/
void QCalendarWidget::updateCell(const QDate &date)
{
    if (!date.isValid()) {
        qWarning("QCalendarWidget::updateCell: Invalid date");
        return;
    }

    if (!isVisible())
        return;

    Q_D(QCalendarWidget);
    int row, column;
    d->m_model->cellForDate(date, &row, &column);
    if (row == -1 || column == -1)
        return;

    QModelIndex modelIndex = d->m_model->index(row, column);
    if (!modelIndex.isValid())
        return;

    d->m_view->viewport()->update(d->m_view->visualRect(modelIndex));
}

/*!
    \since 4.4

    Updates all visible cells unless updates are disabled.

    \sa updateCell()
*/
void QCalendarWidget::updateCells()
{
    Q_D(QCalendarWidget);
    if (isVisible())
        d->m_view->viewport()->update();
}

/*!
    \fn void QCalendarWidget::selectionChanged()

    This signal is emitted when the currently selected date is
    changed.

    The currently selected date can be changed by the user using the
    mouse or keyboard, or by the programmer using setSelectedDate().

    \sa selectedDate()
*/

/*!
    \fn void QCalendarWidget::currentPageChanged(int year, int month)

    This signal is emitted when the currently shown month is changed.
    The new \a year and \a month are passed as parameters.

    \sa setCurrentPage()
*/

/*!
    \fn void QCalendarWidget::activated(const QDate &date)

    This signal is emitted whenever the user presses the Return or
    Enter key or double-clicks a \a date in the calendar
    widget.
*/

/*!
    \fn void QCalendarWidget::clicked(const QDate &date)

    This signal is emitted when a mouse button is clicked. The date
    the mouse was clicked on is specified by \a date. The signal is
    only emitted when clicked on a valid date, e.g., dates are not
    outside the minimumDate() and maximumDate(). If the selection mode
    is NoSelection, this signal will not be emitted.

*/

/*!
    \property QCalendarWidget::headerVisible
    \brief whether the navigation bar is shown or not

    \obsolete

    Use navigationBarVisible() instead.

    By default, this property is true.
*/

/*!
    \obsolete

    Use setNavigationBarVisible() instead.
*/
bool QCalendarWidget::isHeaderVisible() const
{
    Q_D(const QCalendarWidget);
    return d->navBarVisible;
}

/*!
    \obsolete

    Use setNavigationBarVisible() instead.

*/
void QCalendarWidget::setHeaderVisible(bool visible)
{
    setNavigationBarVisible(visible);
}

/*!
    \property QCalendarWidget::navigationBarVisible
    \brief whether the navigation bar is shown or not

    \since 4.3

    When this property is true (the default), the next month,
    previous month, month selection, year selection controls are
    shown on top.

    When the property is set to false, these controls are hidden.
*/

void QCalendarWidget::setNavigationBarVisible(bool visible)
{
    Q_D(QCalendarWidget);
    d->navBarVisible = visible;
    d->cachedSizeHint = QSize();
    d->navBarBackground->setVisible(visible);
    updateGeometry();
}

/*!
  \reimp
*/
bool QCalendarWidget::event(QEvent *event)
{
    Q_D(QCalendarWidget);
    switch (event->type()) {
        case QEvent::LayoutDirectionChange:
            d->updateButtonIcons();
        case QEvent::LocaleChange:
            d->cachedSizeHint = QSize();
            d->updateMonthMenuNames();
            d->updateNavigationBar();
            d->m_view->updateGeometry();
            break;
        case QEvent::FontChange:
        case QEvent::ApplicationFontChange:
            d->cachedSizeHint = QSize();
            d->m_view->updateGeometry();
            break;
        case QEvent::StyleChange:
            d->cachedSizeHint = QSize();
            d->m_view->updateGeometry();
        default:
            break;
    }
    return QWidget::event(event);
}

/*!
  \reimp
*/
bool QCalendarWidget::eventFilter(QObject *watched, QEvent *event)
{
    Q_D(QCalendarWidget);
    if (event->type() == QEvent::MouseButtonPress && d->yearEdit->hasFocus()) {
        QWidget *tlw = window();
        QWidget *widget = static_cast<QWidget*>(watched);
        //as we have a event filter on the whole application we first make sure that the top level widget
        //of both this and the watched widget are the same to decide if we should finish the year edition.
        if (widget->window() == tlw) {
            QPoint mousePos = widget->mapTo(tlw, static_cast<QMouseEvent *>(event)->pos());
            QRect geom = QRect(d->yearEdit->mapTo(tlw, QPoint(0, 0)), d->yearEdit->size());
            if (!geom.contains(mousePos)) {
                event->accept();
                d->_q_yearEditingFinished();
                setFocus();
                return true;
            }
        }
    }
    return QWidget::eventFilter(watched, event);
}

/*!
  \reimp
*/
void QCalendarWidget::mousePressEvent(QMouseEvent *event)
{
    setAttribute(Qt::WA_NoMouseReplay);
    QWidget::mousePressEvent(event);
    setFocus();
}

/*!
  \reimp
*/
void QCalendarWidget::resizeEvent(QResizeEvent * event)
{
    Q_D(QCalendarWidget);

    // XXX Should really use a QWidgetStack for yearEdit and yearButton,
    // XXX here we hide the year edit when the layout is likely to break
    // XXX the manual positioning of the yearEdit over the yearButton.
    if(d->yearEdit->isVisible() && event->size().width() != event->oldSize().width())
        d->_q_yearEditingFinished();

    QWidget::resizeEvent(event);
}

/*!
  \reimp
*/
void QCalendarWidget::keyPressEvent(QKeyEvent * event)
{
    Q_D(QCalendarWidget);
    if(d->yearEdit->isVisible()&& event->key() == Qt::Key_Escape)
    {
        d->yearEdit->setValue(yearShown());
        d->_q_yearEditingFinished();
        return;
    }
    QWidget::keyPressEvent(event);
}

QT_END_NAMESPACE

#include "qcalendarwidget.moc"
#include "moc_qcalendarwidget.cpp"

#endif //QT_NO_CALENDARWIDGET
