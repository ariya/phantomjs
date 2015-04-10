/***************************************************************************
**
** Copyright (C) 2011 - 2012 Research In Motion
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "qqnxglobal.h"

#include "qqnxscreeneventthread.h"
#include "qqnxscreeneventhandler.h"

#include <QtCore/QDebug>

#include <errno.h>
#include <unistd.h>

#include <cctype>

#if defined(QQNXSCREENEVENTTHREAD_DEBUG)
#define qScreenEventThreadDebug qDebug
#else
#define qScreenEventThreadDebug QT_NO_QDEBUG_MACRO
#endif

QQnxScreenEventThread::QQnxScreenEventThread(screen_context_t context, QQnxScreenEventHandler *screenEventHandler)
    : QThread(),
      m_screenContext(context),
      m_screenEventHandler(screenEventHandler),
      m_quit(false)
{
    screenEventHandler->setScreenEventThread(this);
    connect(this, SIGNAL(eventPending()), screenEventHandler, SLOT(processEventsFromScreenThread()), Qt::QueuedConnection);
    connect(this, SIGNAL(finished()), screenEventHandler, SLOT(processEventsFromScreenThread()), Qt::QueuedConnection);
}

QQnxScreenEventThread::~QQnxScreenEventThread()
{
    // block until thread terminates
    shutdown();
}

void QQnxScreenEventThread::injectKeyboardEvent(int flags, int sym, int mod, int scan, int cap)
{
    QQnxScreenEventHandler::injectKeyboardEvent(flags, sym, mod, scan, cap);
}

QQnxScreenEventArray *QQnxScreenEventThread::lock()
{
    m_mutex.lock();
    return &m_events;
}

void QQnxScreenEventThread::unlock()
{
    m_mutex.unlock();
}

void QQnxScreenEventThread::run()
{
    qScreenEventThreadDebug() << Q_FUNC_INFO << "screen event thread started";

    int errorCounter = 0;
    // loop indefinitely
    while (!m_quit) {
        screen_event_t event;

        // create screen event
        Q_SCREEN_CHECKERROR(screen_create_event(&event), "Failed to create screen event");

        // block until screen event is available
        const int error = screen_get_event(m_screenContext, event, -1);
        Q_SCREEN_CRITICALERROR(error, "Failed to get screen event");
        // Only allow 50 consecutive errors before we exit the thread
        if (error) {
            errorCounter++;
            if (errorCounter > 50)
                m_quit = true;

            screen_destroy_event(event);
            continue;
        } else {
            errorCounter = 0;
        }

        // process received event
        // get the event type
        int qnxType;
        Q_SCREEN_CHECKERROR(screen_get_event_property_iv(event, SCREEN_PROPERTY_TYPE, &qnxType),
                            "Failed to query screen event type");

        if (qnxType == SCREEN_EVENT_USER) {
            // treat all user events as shutdown requests
            qScreenEventThreadDebug() << Q_FUNC_INFO << "QNX user screen event";
            m_quit = true;
        } else {
            m_mutex.lock();
            m_events << event;
            m_mutex.unlock();
            emit eventPending();
        }
    }

    qScreenEventThreadDebug() << Q_FUNC_INFO << "screen event thread stopped";

    // cleanup
    m_mutex.lock();
    Q_FOREACH (screen_event_t event, m_events) {
        screen_destroy_event(event);
    }
    m_events.clear();
    m_mutex.unlock();
}

void QQnxScreenEventThread::shutdown()
{
    screen_event_t event;

    // create screen event
    Q_SCREEN_CHECKERROR(screen_create_event(&event),
                        "Failed to create screen event");

    // set the event type as user
    int type = SCREEN_EVENT_USER;
    Q_SCREEN_CHECKERROR(screen_set_event_property_iv(event, SCREEN_PROPERTY_TYPE, &type),
                        "Failed to set screen type");

    // NOTE: ignore SCREEN_PROPERTY_USER_DATA; treat all user events as shutdown events

    // post event to event loop so it will wake up and die
    Q_SCREEN_CHECKERROR(screen_send_event(m_screenContext, event, getpid()),
                        "Failed to set screen event type");

    // cleanup
    screen_destroy_event(event);

    qScreenEventThreadDebug() << Q_FUNC_INFO << "screen event thread shutdown begin";

    // block until thread terminates
    wait();

    qScreenEventThreadDebug() << Q_FUNC_INFO << "screen event thread shutdown end";
}
