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

#ifndef QABSTRACTEVENTDISPATCHER_H
#define QABSTRACTEVENTDISPATCHER_H

#include <QtCore/qobject.h>
#include <QtCore/qeventloop.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Core)

class QAbstractEventDispatcherPrivate;
class QSocketNotifier;
template <typename T1, typename T2> struct QPair;

class Q_CORE_EXPORT QAbstractEventDispatcher : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QAbstractEventDispatcher)

public:
    typedef QPair<int, int> TimerInfo;

    explicit QAbstractEventDispatcher(QObject *parent = 0);
    ~QAbstractEventDispatcher();

    static QAbstractEventDispatcher *instance(QThread *thread = 0);

    virtual bool processEvents(QEventLoop::ProcessEventsFlags flags) = 0;
    virtual bool hasPendingEvents() = 0;

    virtual void registerSocketNotifier(QSocketNotifier *notifier) = 0;
    virtual void unregisterSocketNotifier(QSocketNotifier *notifier) = 0;

    int registerTimer(int interval, QObject *object);
    virtual void registerTimer(int timerId, int interval, QObject *object) = 0;
    virtual bool unregisterTimer(int timerId) = 0;
    virtual bool unregisterTimers(QObject *object) = 0;
    virtual QList<TimerInfo> registeredTimers(QObject *object) const = 0;

    virtual void wakeUp() = 0;
    virtual void interrupt() = 0;
    virtual void flush() = 0;

    virtual void startingUp();
    virtual void closingDown();

    typedef bool(*EventFilter)(void *message);
    EventFilter setEventFilter(EventFilter filter);
    bool filterEvent(void *message);

Q_SIGNALS:
    void aboutToBlock();
    void awake();

protected:
    QAbstractEventDispatcher(QAbstractEventDispatcherPrivate &,
                             QObject *parent);
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QABSTRACTEVENTDISPATCHER_H
