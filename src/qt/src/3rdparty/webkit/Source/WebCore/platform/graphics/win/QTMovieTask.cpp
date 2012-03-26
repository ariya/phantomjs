/*
 * Copyright (C) 2007, 2008, 2009, 2010 Apple, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */
#include "config.h"

#include "QTMovieTask.h"

// Put Movies.h first so build failures here point clearly to QuickTime
#include <Movies.h>

#include <wtf/HashSet.h>
#include <wtf/StdLibExtras.h>
#include <wtf/Vector.h>

QTMovieTask::QTMovieTask() 
    : m_setTaskTimerDelay(0)
    , m_stopTaskTimer(0)
{
}

QTMovieTask::~QTMovieTask()
{
}

QTMovieTask* QTMovieTask::sharedTask()
{
    static QTMovieTask* s_sharedTask = new QTMovieTask;
    return s_sharedTask;
}

void QTMovieTask::updateTaskTimer(double maxInterval, double minInterval)
{
    ASSERT(m_setTaskTimerDelay);
    if (!m_setTaskTimerDelay)
        return;

    ASSERT(m_stopTaskTimer);
    if (!m_taskList.size() && m_stopTaskTimer) {
        m_stopTaskTimer();
        return;
    }
    
    long intervalInMS;
    OSStatus status = QTGetTimeUntilNextTask(&intervalInMS, 1000);
    double interval = intervalInMS / 1000.0;
    if (interval < minInterval)
        interval = minInterval;
    if (interval > maxInterval)
        interval = maxInterval;
    m_setTaskTimerDelay(interval);
}

void QTMovieTask::fireTaskClients()
{
    Vector<QTMovieTaskClient*> clients;
    copyToVector(m_taskList, clients);
    for (Vector<QTMovieTaskClient*>::iterator i = clients.begin(); i != clients.end(); ++i)
        (*i)->task();
}

void QTMovieTask::addTaskClient(QTMovieTaskClient* client) 
{
    ASSERT(client);
    if (!client)
        return;

    m_taskList.add(client);
}

void QTMovieTask::removeTaskClient(QTMovieTaskClient* client)
{
    ASSERT(client);
    if (!client)
        return;

    m_taskList.remove(client);
}

void QTMovieTask::setTaskTimerFuncs(SetTaskTimerDelayFunc setTaskTimerDelay, StopTaskTimerFunc stopTaskTimer)
{
    m_setTaskTimerDelay = setTaskTimerDelay;
    m_stopTaskTimer = stopTaskTimer;
}

