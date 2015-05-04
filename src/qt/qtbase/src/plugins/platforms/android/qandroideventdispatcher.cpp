/****************************************************************************
**
** Copyright (C) 2014 BogDan Vatra <bogdan@kde.org>
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "qandroideventdispatcher.h"
#include "androidjnimain.h"

QAndroidEventDispatcher::QAndroidEventDispatcher(QObject *parent) :
    QUnixEventDispatcherQPA(parent)
{
    if (QtAndroid::blockEventLoopsWhenSuspended())
        QAndroidEventDispatcherStopper::instance()->addEventDispatcher(this);
}

QAndroidEventDispatcher::~QAndroidEventDispatcher()
{
    if (QtAndroid::blockEventLoopsWhenSuspended())
        QAndroidEventDispatcherStopper::instance()->removeEventDispatcher(this);
}

enum States {Running = 0, StopRequest = 1, Stopping = 2};

void QAndroidEventDispatcher::start()
{
    int prevState = m_stopRequest.fetchAndStoreAcquire(Running);
    if (prevState == Stopping) {
        m_semaphore.release();
        wakeUp();
    } else if (prevState == Running) {
        qWarning("Error: start without corresponding stop");
    }
    //else if prevState == StopRequest, no action needed
}

void QAndroidEventDispatcher::stop()
{
    if (m_stopRequest.testAndSetAcquire(Running, StopRequest))
        wakeUp();
    else
        qWarning("Error: start/stop out of sync");
}

void QAndroidEventDispatcher::goingToStop(bool stop)
{
    m_goingToStop.store(stop ? 1 : 0);
    if (!stop)
        wakeUp();
}

int QAndroidEventDispatcher::select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, timespec *timeout)
{
    if (m_stopRequest.testAndSetAcquire(StopRequest, Stopping)) {
        m_semaphore.acquire();
        wakeUp();
    }

    return QUnixEventDispatcherQPA::select(nfds, readfds, writefds, exceptfds, timeout);
}

bool QAndroidEventDispatcher::processEvents(QEventLoop::ProcessEventsFlags flags)
{
    if (m_goingToStop.load()) {
        return QUnixEventDispatcherQPA::processEvents(flags /*| QEventLoop::ExcludeUserInputEvents*/
                                                      | QEventLoop::ExcludeSocketNotifiers
                                                      | QEventLoop::X11ExcludeTimers);
    } else {
        return QUnixEventDispatcherQPA::processEvents(flags);
    }
}


QAndroidEventDispatcherStopper *QAndroidEventDispatcherStopper::instance()
{
    static QAndroidEventDispatcherStopper androidEventDispatcherStopper;
    return &androidEventDispatcherStopper;
}

void QAndroidEventDispatcherStopper::startAll()
{
    QMutexLocker lock(&m_mutex);
    if (started)
        return;

    started = true;
    foreach (QAndroidEventDispatcher *d, m_dispatchers)
        d->start();
}

void QAndroidEventDispatcherStopper::stopAll()
{
    QMutexLocker lock(&m_mutex);
    if (!started)
        return;

    started = false;
    foreach (QAndroidEventDispatcher *d, m_dispatchers)
        d->stop();
}

void QAndroidEventDispatcherStopper::addEventDispatcher(QAndroidEventDispatcher *dispatcher)
{
    QMutexLocker lock(&m_mutex);
    m_dispatchers.push_back(dispatcher);
}

void QAndroidEventDispatcherStopper::removeEventDispatcher(QAndroidEventDispatcher *dispatcher)
{
    QMutexLocker lock(&m_mutex);
    m_dispatchers.erase(std::find(m_dispatchers.begin(), m_dispatchers.end(), dispatcher));
}

void QAndroidEventDispatcherStopper::goingToStop(bool stop)
{
    QMutexLocker lock(&m_mutex);
    foreach (QAndroidEventDispatcher *d, m_dispatchers)
        d->goingToStop(stop);
}
