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

#include "qplatformdefs.h"
#include "qapplication.h"
#include "private/qwscommand_qws_p.h"
#include "qwsdisplay_qws.h"
#include "qwsevent_qws.h"
#include "qwindowsystem_qws.h"
#include "qeventdispatcher_qws_p.h"
#include "private/qeventdispatcher_unix_p.h"
#ifndef QT_NO_THREAD
#  include "qmutex.h"
#endif

#include <errno.h>

QT_BEGIN_NAMESPACE

QT_USE_NAMESPACE

class QEventDispatcherQWSPrivate : public QEventDispatcherUNIXPrivate
{
    Q_DECLARE_PUBLIC(QEventDispatcherQWS)
public:
    inline QEventDispatcherQWSPrivate()
    { }
    QList<QWSEvent*> queuedUserInputEvents;
};


QEventDispatcherQWS::QEventDispatcherQWS(QObject *parent)
    : QEventDispatcherUNIX(*new QEventDispatcherQWSPrivate, parent)
{ }

QEventDispatcherQWS::~QEventDispatcherQWS()
{ }



// from qapplication_qws.cpp
extern QWSDisplay* qt_fbdpy; // QWS `display'

//#define ZERO_FOR_THE_MOMENT

bool QEventDispatcherQWS::processEvents(QEventLoop::ProcessEventsFlags flags)
{
    Q_D(QEventDispatcherQWS);
    // process events from the QWS server
    int           nevents = 0;

    // handle gui and posted events
    d->interrupt = false;
    QApplication::sendPostedEvents();

    while (!d->interrupt) {        // also flushes output buffer ###can be optimized
        QWSEvent *event;
        if (!(flags & QEventLoop::ExcludeUserInputEvents)
            && !d->queuedUserInputEvents.isEmpty()) {
            // process a pending user input event
            event = d->queuedUserInputEvents.takeFirst();
        } else if  (qt_fbdpy->eventPending()) {
            event = qt_fbdpy->getEvent();        // get next event
             if (flags & QEventLoop::ExcludeUserInputEvents) {
                 // queue user input events
                 if (event->type == QWSEvent::Mouse || event->type == QWSEvent::Key) {
                      d->queuedUserInputEvents.append(event);
                        continue;
                 }
             }
        } else {
            break;
        }

        if (filterEvent(event)) {
            delete event;
            continue;
        }
        nevents++;

        bool ret = qApp->qwsProcessEvent(event) == 1;
        delete event;
        if (ret) {
            return true;
        }
    }

    if (!d->interrupt) {
        extern QList<QWSCommand*> *qt_get_server_queue();
        if (!qt_get_server_queue()->isEmpty()) {
            QWSServer::processEventQueue();
        }

        if (QEventDispatcherUNIX::processEvents(flags))
            return true;
    }
    return (nevents > 0);
}

bool QEventDispatcherQWS::hasPendingEvents()
{
    extern uint qGlobalPostedEventsCount(); // from qapplication.cpp
    return qGlobalPostedEventsCount() || qt_fbdpy->eventPending();
}

void QEventDispatcherQWS::startingUp()
{

}

void QEventDispatcherQWS::closingDown()
{

}

void QEventDispatcherQWS::flush()
{
    if(qApp)
        qApp->sendPostedEvents();
    (void)qt_fbdpy->eventPending(); // flush
}


int QEventDispatcherQWS::select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
                                timeval *timeout)
{
    return QEventDispatcherUNIX::select(nfds, readfds, writefds, exceptfds, timeout);
}

QT_END_NAMESPACE
