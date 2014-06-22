/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

/****************************************************************************
**
** Copyright (c) 2007-2008, Apple, Inc.
**
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**
**   * Redistributions of source code must retain the above copyright notice,
**     this list of conditions and the following disclaimer.
**
**   * Redistributions in binary form must reproduce the above copyright notice,
**     this list of conditions and the following disclaimer in the documentation
**     and/or other materials provided with the distribution.
**
**   * Neither the name of Apple, Inc. nor the names of its contributors
**     may be used to endorse or promote products derived from this software
**     without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
** CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
** EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
** PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
** PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
** LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
** NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
** SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**
****************************************************************************/

#ifndef QEVENTDISPATCHER_CF_P_H
#define QEVENTDISPATCHER_CF_P_H

#define DEBUG_EVENT_DISPATCHER 0

#include <QtCore/qabstracteventdispatcher.h>
#include <QtCore/private/qtimerinfo_unix_p.h>
#include <QtCore/qdebug.h>
#include <QtPlatformSupport/private/qcfsocketnotifier_p.h>
#include <CoreFoundation/CoreFoundation.h>

#ifdef __OBJC__
@class RunLoopModeTracker;
#else
typedef struct objc_object RunLoopModeTracker;
#endif

QT_BEGIN_NAMESPACE

class QEventDispatcherCoreFoundation;

template <class T = QEventDispatcherCoreFoundation>
class RunLoopSource
{
public:
    typedef void (T::*CallbackFunction) ();

    enum { kHighestPriority = 0 } RunLoopSourcePriority;

    RunLoopSource(T *delegate, CallbackFunction callback)
        : m_delegate(delegate), m_callback(callback)
    {
        CFRunLoopSourceContext context = {};
        context.info = this;
        context.perform = RunLoopSource::process;

        m_source = CFRunLoopSourceCreate(kCFAllocatorDefault, kHighestPriority, &context);
        Q_ASSERT(m_source);
    }

    ~RunLoopSource()
    {
        CFRunLoopSourceInvalidate(m_source);
        CFRelease(m_source);
    }

    void addToMode(CFStringRef mode, CFRunLoopRef runLoop = 0)
    {
        if (!runLoop)
            runLoop = CFRunLoopGetCurrent();

        CFRunLoopAddSource(runLoop, m_source, mode);
    }

    void signal() { CFRunLoopSourceSignal(m_source); }

private:
    static void process(void *info)
    {
        RunLoopSource *self = static_cast<RunLoopSource *>(info);
        ((self->m_delegate)->*(self->m_callback))();
    }

    T *m_delegate;
    CallbackFunction m_callback;
    CFRunLoopSourceRef m_source;
};

template <class T = QEventDispatcherCoreFoundation>
class RunLoopObserver
{
public:
    typedef void (T::*CallbackFunction) (CFRunLoopActivity activity);

    RunLoopObserver(T *delegate, CallbackFunction callback, CFOptionFlags activities)
        : m_delegate(delegate), m_callback(callback)
    {
        CFRunLoopObserverContext context = {};
        context.info = this;

        m_observer = CFRunLoopObserverCreate(kCFAllocatorDefault, activities, true, 0, process, &context);
        Q_ASSERT(m_observer);
    }

    ~RunLoopObserver()
    {
        CFRunLoopObserverInvalidate(m_observer);
        CFRelease(m_observer);
    }

    void addToMode(CFStringRef mode, CFRunLoopRef runLoop = 0)
    {
        if (!runLoop)
            runLoop = CFRunLoopGetCurrent();

        if (!CFRunLoopContainsObserver(runLoop, m_observer, mode))
            CFRunLoopAddObserver(runLoop, m_observer, mode);
    }

    void removeFromMode(CFStringRef mode, CFRunLoopRef runLoop = 0)
    {
        if (!runLoop)
            runLoop = CFRunLoopGetCurrent();

        if (CFRunLoopContainsObserver(runLoop, m_observer, mode))
            CFRunLoopRemoveObserver(runLoop, m_observer, mode);
    }

private:
    static void process(CFRunLoopObserverRef, CFRunLoopActivity activity, void *info)
    {
        RunLoopObserver *self = static_cast<RunLoopObserver *>(info);
        ((self->m_delegate)->*(self->m_callback))(activity);
    }

    T *m_delegate;
    CallbackFunction m_callback;
    CFRunLoopObserverRef m_observer;
};

class QEventDispatcherCoreFoundation : public QAbstractEventDispatcher
{
    Q_OBJECT

public:
    explicit QEventDispatcherCoreFoundation(QObject *parent = 0);
    ~QEventDispatcherCoreFoundation();

    bool processEvents(QEventLoop::ProcessEventsFlags flags);
    bool hasPendingEvents();

    void registerSocketNotifier(QSocketNotifier *notifier);
    void unregisterSocketNotifier(QSocketNotifier *notifier);

    void registerTimer(int timerId, int interval, Qt::TimerType timerType, QObject *object);
    bool unregisterTimer(int timerId);
    bool unregisterTimers(QObject *object);
    QList<QAbstractEventDispatcher::TimerInfo> registeredTimers(QObject *object) const;

    int remainingTime(int timerId);

    void wakeUp();
    void interrupt();
    void flush();

private:
    RunLoopSource<> m_postedEventsRunLoopSource;
    RunLoopObserver<> m_runLoopActivityObserver;

    RunLoopModeTracker *m_runLoopModeTracker;

    QTimerInfoList m_timerInfoList;
    CFRunLoopTimerRef m_runLoopTimer;
    CFRunLoopTimerRef m_blockedRunLoopTimer;
    bool m_overdueTimerScheduled;

    QCFSocketNotifier m_cfSocketNotifier;

    struct ProcessEventsState
    {
        ProcessEventsState(QEventLoop::ProcessEventsFlags f)
         : flags(f), wasInterrupted(false)
         , processedPostedEvents(false), processedTimers(false)
         , deferredWakeUp(false), deferredUpdateTimers(false) {}

        QEventLoop::ProcessEventsFlags flags;
        bool wasInterrupted;
        bool processedPostedEvents;
        bool processedTimers;
        bool deferredWakeUp;
        bool deferredUpdateTimers;
    };

    ProcessEventsState m_processEvents;

    void processPostedEvents();
    void processTimers(CFRunLoopTimerRef);

    void handleRunLoopActivity(CFRunLoopActivity activity);

    void updateTimers();
    void invalidateTimer();
};

QT_END_NAMESPACE

#if DEBUG_EVENT_DISPATCHER
extern uint g_eventDispatcherIndentationLevel;
#define qEventDispatcherDebug() qDebug().nospace() \
            << qPrintable(QString(QLatin1String("| ")).repeated(g_eventDispatcherIndentationLevel)) \
            << __FUNCTION__ << "(): "
#define qIndent() ++g_eventDispatcherIndentationLevel
#define qUnIndent() --g_eventDispatcherIndentationLevel
#else
#define qEventDispatcherDebug() QT_NO_QDEBUG_MACRO()
#define qIndent()
#define qUnIndent()
#endif

#endif // QEVENTDISPATCHER_CF_P_H
