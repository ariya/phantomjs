/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Portions Copyright (c) 2010 Motorola Mobility, Inc.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WorkQueue_h
#define WorkQueue_h

#if OS(DARWIN)
#if HAVE(DISPATCH_H)
#include <dispatch/dispatch.h>
#endif
#endif

#include <wtf/Forward.h>
#include <wtf/Functional.h>
#include <wtf/HashMap.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/Threading.h>
#include <wtf/Vector.h>

#if (PLATFORM(QT) && !OS(DARWIN)) || PLATFORM(GTK) || PLATFORM(EFL)
#include "PlatformProcessIdentifier.h"
#endif

#if PLATFORM(QT) && !OS(DARWIN)
#include <QSocketNotifier>
QT_BEGIN_NAMESPACE
class QObject;
class QThread;
QT_END_NAMESPACE
#elif PLATFORM(GTK)
#include <wtf/gobject/GRefPtr.h>
typedef gboolean (*GSourceFunc) (gpointer data);
#elif PLATFORM(EFL)
#include <Ecore.h>
#endif

class WorkQueue : public ThreadSafeRefCounted<WorkQueue> {
public:
    static PassRefPtr<WorkQueue> create(const char* name);
    ~WorkQueue();

    // Will dispatch the given function to run as soon as possible.
    void dispatch(const Function<void()>&);

    // Will dispatch the given function after the given delay (in seconds).
    void dispatchAfterDelay(const Function<void()>&, double delay);

#if OS(DARWIN)
    dispatch_queue_t dispatchQueue() const { return m_dispatchQueue; }

#elif OS(WINDOWS)
    void registerHandle(HANDLE, const Function<void()>&);
    void unregisterAndCloseHandle(HANDLE);
#elif PLATFORM(QT)
    QSocketNotifier* registerSocketEventHandler(int, QSocketNotifier::Type, const Function<void()>&);
    void dispatchOnTermination(WebKit::PlatformProcessIdentifier, const Function<void()>&);
#elif PLATFORM(GTK)
    void registerSocketEventHandler(int, int, const Function<void()>& function, const Function<void()>& closeFunction);
    void unregisterSocketEventHandler(int);
    void dispatchOnTermination(WebKit::PlatformProcessIdentifier, const Function<void()>&);
#elif PLATFORM(EFL)
    void registerSocketEventHandler(int, const Function<void()>&);
    void unregisterSocketEventHandler(int);
#endif

private:
    explicit WorkQueue(const char* name);

    void platformInitialize(const char* name);
    void platformInvalidate();

#if OS(DARWIN)
    static void executeFunction(void*);
    dispatch_queue_t m_dispatchQueue;
#elif OS(WINDOWS)
    class WorkItemWin : public ThreadSafeRefCounted<WorkItemWin> {
    public:
        static PassRefPtr<WorkItemWin> create(const Function<void()>&, WorkQueue*);
        virtual ~WorkItemWin();

        Function<void()>& function() { return m_function; }
        WorkQueue* queue() const { return m_queue.get(); }

    protected:
        WorkItemWin(const Function<void()>&, WorkQueue*);

    private:
        Function<void()> m_function;
        RefPtr<WorkQueue> m_queue;
    };

    class HandleWorkItem : public WorkItemWin {
    public:
        static PassRefPtr<HandleWorkItem> createByAdoptingHandle(HANDLE, const Function<void()>&, WorkQueue*);
        virtual ~HandleWorkItem();

        void setWaitHandle(HANDLE waitHandle) { m_waitHandle = waitHandle; }
        HANDLE waitHandle() const { return m_waitHandle; }

    private:
        HandleWorkItem(HANDLE, const Function<void()>&, WorkQueue*);

        HANDLE m_handle;
        HANDLE m_waitHandle;
    };

    static void CALLBACK handleCallback(void* context, BOOLEAN timerOrWaitFired);
    static void CALLBACK timerCallback(void* context, BOOLEAN timerOrWaitFired);
    static DWORD WINAPI workThreadCallback(void* context);

    bool tryRegisterAsWorkThread();
    void unregisterAsWorkThread();
    void performWorkOnRegisteredWorkThread();

    static void unregisterWaitAndDestroyItemSoon(PassRefPtr<HandleWorkItem>);
    static DWORD WINAPI unregisterWaitAndDestroyItemCallback(void* context);

    volatile LONG m_isWorkThreadRegistered;

    Mutex m_workItemQueueLock;
    Vector<RefPtr<WorkItemWin>> m_workItemQueue;

    Mutex m_handlesLock;
    HashMap<HANDLE, RefPtr<HandleWorkItem>> m_handles;

    HANDLE m_timerQueue;
#elif PLATFORM(QT)
    class WorkItemQt;
    QThread* m_workThread;
    friend class WorkItemQt;
#elif PLATFORM(GTK)
    static void startWorkQueueThread(WorkQueue*);
    void workQueueThreadBody();
    void dispatchOnSource(GSource*, const Function<void()>&, GSourceFunc);

    ThreadIdentifier m_workQueueThread;
    GRefPtr<GMainContext> m_eventContext;
    Mutex m_eventLoopLock;
    GRefPtr<GMainLoop> m_eventLoop;
    Mutex m_eventSourcesLock;
    class EventSource;
    class SocketEventSource;
    HashMap<int, Vector<SocketEventSource*>> m_eventSources;
    typedef HashMap<int, Vector<SocketEventSource*>>::iterator SocketEventSourceIterator;
#elif PLATFORM(EFL)
    class TimerWorkItem {
    public:
        static PassOwnPtr<TimerWorkItem> create(Function<void()>, double expireTime);
        void dispatch() { m_function(); }
        double expireTime() const { return m_expireTime; }
        bool expired(double currentTime) const { return currentTime >= m_expireTime; }

    protected:
        TimerWorkItem(Function<void()>, double expireTime);

    private:
        Function<void()> m_function;
        double m_expireTime;
    };

    fd_set m_fileDescriptorSet;
    int m_maxFileDescriptor;
    int m_readFromPipeDescriptor;
    int m_writeToPipeDescriptor;
    Mutex m_writeToPipeDescriptorLock;

    bool m_threadLoop;

    Vector<Function<void()>> m_workItemQueue;
    Mutex m_workItemQueueLock;

    int m_socketDescriptor;
    Function<void()> m_socketEventHandler;

    Vector<OwnPtr<TimerWorkItem>> m_timerWorkItems;
    Mutex m_timerWorkItemsLock;

    void sendMessageToThread(const char*);
    static void* workQueueThread(WorkQueue*);
    void performWork();
    void performFileDescriptorWork();
    static double getCurrentTime();
    struct timeval* getNextTimeOut();
    void performTimerWork();
    void insertTimerWorkItem(PassOwnPtr<TimerWorkItem>);
#endif
};

#endif // WorkQueue_h
