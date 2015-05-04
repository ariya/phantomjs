/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QABSTRACTEVENTDISPATCHER_H
#define QABSTRACTEVENTDISPATCHER_H

#include <QtCore/qobject.h>
#include <QtCore/qeventloop.h>

QT_BEGIN_NAMESPACE

class QAbstractNativeEventFilter;
class QAbstractEventDispatcherPrivate;
class QSocketNotifier;

#ifdef Q_OS_WIN
class QWinEventNotifier;
#endif

class Q_CORE_EXPORT QAbstractEventDispatcher : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QAbstractEventDispatcher)

public:
    struct TimerInfo
    {
        int timerId;
        int interval;
        Qt::TimerType timerType;

        inline TimerInfo(int id, int i, Qt::TimerType t)
            : timerId(id), interval(i), timerType(t)
        { }
    };

    explicit QAbstractEventDispatcher(QObject *parent = 0);
    ~QAbstractEventDispatcher();

    static QAbstractEventDispatcher *instance(QThread *thread = 0);

    virtual bool processEvents(QEventLoop::ProcessEventsFlags flags) = 0;
    virtual bool hasPendingEvents() = 0; // ### Qt6: remove, mark final or make protected

    virtual void registerSocketNotifier(QSocketNotifier *notifier) = 0;
    virtual void unregisterSocketNotifier(QSocketNotifier *notifier) = 0;

#if QT_DEPRECATED_SINCE(5,0)
    QT_DEPRECATED inline int registerTimer(int interval, QObject *object)
    { return registerTimer(interval, Qt::CoarseTimer, object); }
    QT_DEPRECATED inline void registerTimer(int timerId, int interval, QObject *object)
    { registerTimer(timerId, interval, Qt::CoarseTimer, object); }
#endif
    int registerTimer(int interval, Qt::TimerType timerType, QObject *object);
    virtual void registerTimer(int timerId, int interval, Qt::TimerType timerType, QObject *object) = 0;
    virtual bool unregisterTimer(int timerId) = 0;
    virtual bool unregisterTimers(QObject *object) = 0;
    virtual QList<TimerInfo> registeredTimers(QObject *object) const = 0;

    virtual int remainingTime(int timerId) = 0;

#ifdef Q_OS_WIN
    virtual bool registerEventNotifier(QWinEventNotifier *notifier) = 0;
    virtual void unregisterEventNotifier(QWinEventNotifier *notifier) = 0;
#endif

    virtual void wakeUp() = 0;
    virtual void interrupt() = 0;
    virtual void flush() = 0;

    virtual void startingUp();
    virtual void closingDown();

    void installNativeEventFilter(QAbstractNativeEventFilter *filterObj);
    void removeNativeEventFilter(QAbstractNativeEventFilter *filterObj);
    bool filterNativeEvent(const QByteArray &eventType, void *message, long *result);
#if QT_DEPRECATED_SINCE(5, 0)
    QT_DEPRECATED bool filterEvent(void *message) { return filterNativeEvent("", message, 0); }
#endif

Q_SIGNALS:
    void aboutToBlock();
    void awake();

protected:
    QAbstractEventDispatcher(QAbstractEventDispatcherPrivate &,
                             QObject *parent);
};

QT_END_NAMESPACE

#endif // QABSTRACTEVENTDISPATCHER_H
