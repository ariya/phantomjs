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

#include "qwineventnotifier_p.h"

#include "qeventdispatcher_win_p.h"
#include "qcoreapplication.h"

#include <private/qthread_p.h>

QT_BEGIN_NAMESPACE

/*
    \class QWinEventNotifier
    \brief The QWinEventNotifier class provides support for the Windows Wait functions.

    The QWinEventNotifier class makes it possible to use the wait
    functions on windows in a asynchronous manner. With this class
    you can register a HANDLE to an event and get notification when
    that event becomes signalled. The state of the event is not modified
    in the process so if it is a manual reset event you will need to
    reset it after the notification.
*/


QWinEventNotifier::QWinEventNotifier(QObject *parent)
  : QObject(parent), handleToEvent(0), enabled(false)
{}

QWinEventNotifier::QWinEventNotifier(HANDLE hEvent, QObject *parent)
 : QObject(parent), handleToEvent(hEvent), enabled(false)
{
    Q_D(QObject);
    QEventDispatcherWin32 *eventDispatcher = qobject_cast<QEventDispatcherWin32 *>(d->threadData->eventDispatcher);
    Q_ASSERT_X(eventDispatcher, "QWinEventNotifier::QWinEventNotifier()",
               "Cannot create a win event notifier without a QEventDispatcherWin32");
    eventDispatcher->registerEventNotifier(this);
    enabled = true;
}

QWinEventNotifier::~QWinEventNotifier()
{
    setEnabled(false);
}

void QWinEventNotifier::setHandle(HANDLE hEvent)
{
    setEnabled(false);
    handleToEvent = hEvent;
}

HANDLE  QWinEventNotifier::handle() const
{
    return handleToEvent;
}

bool QWinEventNotifier::isEnabled() const
{
    return enabled;
}

void QWinEventNotifier::setEnabled(bool enable)
{
    if (enabled == enable)                        // no change
        return;
    enabled = enable;

    Q_D(QObject);
    QEventDispatcherWin32 *eventDispatcher = qobject_cast<QEventDispatcherWin32 *>(d->threadData->eventDispatcher);
    if (!eventDispatcher) // perhaps application is shutting down
        return;

    if (enabled)
        eventDispatcher->registerEventNotifier(this);
    else
        eventDispatcher->unregisterEventNotifier(this);
}

bool QWinEventNotifier::event(QEvent * e)
{
    if (e->type() == QEvent::ThreadChange) {
        if (enabled) {
            QMetaObject::invokeMethod(this, "setEnabled", Qt::QueuedConnection,
                                      Q_ARG(bool, enabled));
            setEnabled(false);
        }
    }
    QObject::event(e);                        // will activate filters
    if (e->type() == QEvent::WinEventAct) {
        emit activated(handleToEvent);
        return true;
    }
    return false;
}

QT_END_NAMESPACE
