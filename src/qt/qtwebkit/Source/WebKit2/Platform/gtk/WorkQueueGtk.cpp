/*
 * Copyright (C) 2011 Igalia S.L.
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

#include "config.h"
#include "WorkQueue.h"

#include <gio/gio.h>
#include <glib.h>
#include <wtf/gobject/GRefPtr.h>

// WorkQueue::EventSource
class WorkQueue::EventSource {
public:
    EventSource(const Function<void()>& function, WorkQueue* workQueue)
        : m_function(function)
        , m_workQueue(workQueue)
    {
        ASSERT(workQueue);
    }

    virtual ~EventSource() { }

    void performWork()
    {
        m_function();
    }

    static gboolean performWorkOnce(EventSource* eventSource)
    {
        ASSERT(eventSource);
        eventSource->performWork();
        return FALSE;
    }

    static gboolean performWorkOnTermination(GPid, gint, EventSource* eventSource)
    {
        ASSERT(eventSource);
        eventSource->performWork();
        return FALSE;
    }

    static void deleteEventSource(EventSource* eventSource)
    {
        ASSERT(eventSource);
        delete eventSource;
    }

private:
    Function<void()> m_function;
    RefPtr<WorkQueue> m_workQueue;
};

class WorkQueue::SocketEventSource : public WorkQueue::EventSource {
public:
    SocketEventSource(const Function<void()>& function, WorkQueue* workQueue, int condition, GCancellable* cancellable, const Function<void()>& closeFunction)
        : EventSource(function, workQueue)
        , m_condition(condition)
        , m_cancellable(cancellable)
        , m_closeFunction(closeFunction)
    {
        ASSERT(cancellable);
    }

    void cancel()
    {
        g_cancellable_cancel(m_cancellable);
    }

    void didClose()
    {
        m_closeFunction();
    }

    bool checkCondition(GIOCondition condition) const
    {
        return condition & m_condition;
    }

    static gboolean eventCallback(GSocket* socket, GIOCondition condition, SocketEventSource* eventSource)
    {
        ASSERT(eventSource);

        if (condition & G_IO_HUP || condition & G_IO_ERR) {
            eventSource->didClose();
            return FALSE;
        }

        if (eventSource->checkCondition(condition)) {
            eventSource->performWork();
            return TRUE;
        }

        // EventSource has been cancelled, return FALSE to destroy the source.
        return FALSE;
    }

private:
    int m_condition;
    GCancellable* m_cancellable;
    Function<void()> m_closeFunction;
};

// WorkQueue
static const size_t kVisualStudioThreadNameLimit = 31;

void WorkQueue::platformInitialize(const char* name)
{
    m_eventContext = adoptGRef(g_main_context_new());
    ASSERT(m_eventContext);
    m_eventLoop = adoptGRef(g_main_loop_new(m_eventContext.get(), FALSE));
    ASSERT(m_eventLoop);

    // This name can be com.apple.WebKit.ProcessLauncher or com.apple.CoreIPC.ReceiveQueue.
    // We are using those names for the thread name, but both are longer than 31 characters,
    // which is the limit of Visual Studio for thread names.
    // When log is enabled createThread() will assert instead of truncate the name, so we need
    // to make sure we don't use a name longer than 31 characters.
    const char* threadName = g_strrstr(name, ".");
    if (threadName)
        threadName++;
    else
        threadName = name;
    if (strlen(threadName) > kVisualStudioThreadNameLimit)
        threadName += strlen(threadName) - kVisualStudioThreadNameLimit;

    m_workQueueThread = createThread(reinterpret_cast<WTF::ThreadFunction>(&WorkQueue::startWorkQueueThread), this, threadName);
}

void WorkQueue::platformInvalidate()
{
    MutexLocker locker(m_eventLoopLock);

    if (m_eventLoop) {
        if (g_main_loop_is_running(m_eventLoop.get()))
            g_main_loop_quit(m_eventLoop.get());
        m_eventLoop.clear();
    }

    m_eventContext.clear();
}

void WorkQueue::startWorkQueueThread(WorkQueue* workQueue)
{
    workQueue->workQueueThreadBody();
}

void WorkQueue::workQueueThreadBody()
{
    g_main_loop_run(m_eventLoop.get());
}

void WorkQueue::registerSocketEventHandler(int fileDescriptor, int condition, const Function<void()>& function, const Function<void()>& closeFunction)
{
    GRefPtr<GSocket> socket = adoptGRef(g_socket_new_from_fd(fileDescriptor, 0));
    ASSERT(socket);
    GRefPtr<GCancellable> cancellable = adoptGRef(g_cancellable_new());
    GRefPtr<GSource> dispatchSource = adoptGRef(g_socket_create_source(socket.get(), static_cast<GIOCondition>(condition), cancellable.get()));
    ASSERT(dispatchSource);
    SocketEventSource* eventSource = new SocketEventSource(function, this, condition, cancellable.get(), closeFunction);
    ASSERT(eventSource);

    g_source_set_callback(dispatchSource.get(), reinterpret_cast<GSourceFunc>(&WorkQueue::SocketEventSource::eventCallback),
        eventSource, reinterpret_cast<GDestroyNotify>(&WorkQueue::EventSource::deleteEventSource));

    // Set up the event sources under the mutex since this is shared across multiple threads.
    {
        MutexLocker locker(m_eventSourcesLock);
        Vector<SocketEventSource*> sources;
        SocketEventSourceIterator it = m_eventSources.find(fileDescriptor);
        if (it != m_eventSources.end())
            sources = it->value;

        sources.append(eventSource);
        m_eventSources.set(fileDescriptor, sources);
    }

    g_source_attach(dispatchSource.get(), m_eventContext.get());
}

void WorkQueue::unregisterSocketEventHandler(int fileDescriptor)
{
    ASSERT(fileDescriptor);

    MutexLocker locker(m_eventSourcesLock);

    SocketEventSourceIterator it = m_eventSources.find(fileDescriptor);
    ASSERT(it != m_eventSources.end());
    ASSERT(m_eventSources.contains(fileDescriptor));

    if (it != m_eventSources.end()) {
        Vector<SocketEventSource*> sources = it->value;
        for (unsigned i = 0; i < sources.size(); i++)
            sources[i]->cancel();

        m_eventSources.remove(it);
    }
}

void WorkQueue::dispatchOnSource(GSource* dispatchSource, const Function<void()>& function, GSourceFunc sourceCallback)
{
    g_source_set_callback(dispatchSource, sourceCallback, new EventSource(function, this),
        reinterpret_cast<GDestroyNotify>(&WorkQueue::EventSource::deleteEventSource));

    g_source_attach(dispatchSource, m_eventContext.get());
}

void WorkQueue::dispatch(const Function<void()>& function)
{
    GRefPtr<GSource> dispatchSource = adoptGRef(g_idle_source_new());
    ASSERT(dispatchSource);
    g_source_set_priority(dispatchSource.get(), G_PRIORITY_DEFAULT);

    dispatchOnSource(dispatchSource.get(), function, reinterpret_cast<GSourceFunc>(&WorkQueue::EventSource::performWorkOnce));
}

void WorkQueue::dispatchAfterDelay(const Function<void()>& function, double delay)
{
    GRefPtr<GSource> dispatchSource = adoptGRef(g_timeout_source_new(static_cast<guint>(delay * 1000)));
    ASSERT(dispatchSource);

    dispatchOnSource(dispatchSource.get(), function, reinterpret_cast<GSourceFunc>(&WorkQueue::EventSource::performWorkOnce));
}

void WorkQueue::dispatchOnTermination(WebKit::PlatformProcessIdentifier process, const Function<void()>& function)
{
    GRefPtr<GSource> dispatchSource = adoptGRef(g_child_watch_source_new(process));
    ASSERT(dispatchSource);

    dispatchOnSource(dispatchSource.get(), function, reinterpret_cast<GSourceFunc>(&WorkQueue::EventSource::performWorkOnTermination));
}
