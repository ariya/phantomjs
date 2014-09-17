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

#ifndef QTMovieTask_h
#define QTMovieTask_h

#include <WTF/HashSet.h>

#ifdef QTMOVIEWIN_EXPORTS
#define QTMOVIEWIN_API __declspec(dllexport)
#else
#define QTMOVIEWIN_API __declspec(dllimport)
#endif

class QTMovieTaskClient {
public:
    virtual void task() = 0;
};

typedef void (*SetTaskTimerDelayFunc)(double);
typedef void (*StopTaskTimerFunc)();

class QTMOVIEWIN_API QTMovieTask {
public:
    static QTMovieTask* sharedTask();

    void addTaskClient(QTMovieTaskClient* client);
    void removeTaskClient(QTMovieTaskClient*);
    void fireTaskClients();

    void updateTaskTimer(double maxInterval = 1.0, double minInterval = 1.0 / 30);
    void setTaskTimerFuncs(SetTaskTimerDelayFunc setTaskTimerDelay, StopTaskTimerFunc stopTaskTimer);

protected:
    QTMovieTask();
    ~QTMovieTask();

    SetTaskTimerDelayFunc m_setTaskTimerDelay;
    StopTaskTimerFunc m_stopTaskTimer;
    HashSet<QTMovieTaskClient*> m_taskList;

private:
    QTMovieTask(const QTMovieTask&);
    QTMovieTask& operator=(const QTMovieTask&);
};

#endif
