/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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
 *
 */

#include "config.h"

#if ENABLE(WORKERS)

#include "WorkerThread.h"

#include "DedicatedWorkerContext.h"
#include "KURL.h"
#include "PlatformString.h"
#include "ScriptSourceCode.h"
#include "ScriptValue.h"
#include "ThreadGlobalData.h"

#include <utility>
#include <wtf/Noncopyable.h>

#if ENABLE(DATABASE)
#include "DatabaseTask.h"
#include "DatabaseTracker.h"
#endif

namespace WebCore {

static Mutex& threadCountMutex()
{
    AtomicallyInitializedStatic(Mutex&, mutex = *new Mutex);
    return mutex;
}

unsigned WorkerThread::m_threadCount = 0;

unsigned WorkerThread::workerThreadCount()
{
    MutexLocker lock(threadCountMutex());
    return m_threadCount;
}

struct WorkerThreadStartupData {
    WTF_MAKE_NONCOPYABLE(WorkerThreadStartupData); WTF_MAKE_FAST_ALLOCATED;
public:
    static PassOwnPtr<WorkerThreadStartupData> create(const KURL& scriptURL, const String& userAgent, const String& sourceCode)
    {
        return adoptPtr(new WorkerThreadStartupData(scriptURL, userAgent, sourceCode));
    }

    KURL m_scriptURL;
    String m_userAgent;
    String m_sourceCode;
private:
    WorkerThreadStartupData(const KURL& scriptURL, const String& userAgent, const String& sourceCode);
};

WorkerThreadStartupData::WorkerThreadStartupData(const KURL& scriptURL, const String& userAgent, const String& sourceCode)
    : m_scriptURL(scriptURL.copy())
    , m_userAgent(userAgent.crossThreadString())
    , m_sourceCode(sourceCode.crossThreadString())
{
}

WorkerThread::WorkerThread(const KURL& scriptURL, const String& userAgent, const String& sourceCode, WorkerLoaderProxy& workerLoaderProxy, WorkerReportingProxy& workerReportingProxy)
    : m_threadID(0)
    , m_workerLoaderProxy(workerLoaderProxy)
    , m_workerReportingProxy(workerReportingProxy)
    , m_startupData(WorkerThreadStartupData::create(scriptURL, userAgent, sourceCode))
{
    MutexLocker lock(threadCountMutex());
    m_threadCount++;
}

WorkerThread::~WorkerThread()
{
    MutexLocker lock(threadCountMutex());
    ASSERT(m_threadCount > 0);
    m_threadCount--;
}

bool WorkerThread::start()
{
    // Mutex protection is necessary to ensure that m_threadID is initialized when the thread starts.
    MutexLocker lock(m_threadCreationMutex);

    if (m_threadID)
        return true;

    m_threadID = createThread(WorkerThread::workerThreadStart, this, "WebCore: Worker");

    return m_threadID;
}

void* WorkerThread::workerThreadStart(void* thread)
{
    return static_cast<WorkerThread*>(thread)->workerThread();
}

void* WorkerThread::workerThread()
{
    {
        MutexLocker lock(m_threadCreationMutex);
        m_workerContext = createWorkerContext(m_startupData->m_scriptURL, m_startupData->m_userAgent);

        if (m_runLoop.terminated()) {
            // The worker was terminated before the thread had a chance to run. Since the context didn't exist yet,
            // forbidExecution() couldn't be called from stop().
           m_workerContext->script()->forbidExecution();
        }
    }

    WorkerScriptController* script = m_workerContext->script();
    script->evaluate(ScriptSourceCode(m_startupData->m_sourceCode, m_startupData->m_scriptURL));
    // Free the startup data to cause its member variable deref's happen on the worker's thread (since
    // all ref/derefs of these objects are happening on the thread at this point). Note that
    // WorkerThread::~WorkerThread happens on a different thread where it was created.
    m_startupData.clear();

    runEventLoop();

    ThreadIdentifier threadID = m_threadID;

    ASSERT(m_workerContext->hasOneRef());

    // The below assignment will destroy the context, which will in turn notify messaging proxy.
    // We cannot let any objects survive past thread exit, because no other thread will run GC or otherwise destroy them.
    m_workerContext = 0;

    // Clean up WebCore::ThreadGlobalData before WTF::WTFThreadData goes away!
    threadGlobalData().destroy();

    // The thread object may be already destroyed from notification now, don't try to access "this".
    detachThread(threadID);

    return 0;
}

void WorkerThread::runEventLoop()
{
    // Does not return until terminated.
    m_runLoop.run(m_workerContext.get());
}

class WorkerThreadShutdownFinishTask : public ScriptExecutionContext::Task {
public:
    static PassOwnPtr<WorkerThreadShutdownFinishTask> create()
    {
        return adoptPtr(new WorkerThreadShutdownFinishTask());
    }

    virtual void performTask(ScriptExecutionContext *context)
    {
        ASSERT(context->isWorkerContext());
        WorkerContext* workerContext = static_cast<WorkerContext*>(context);
        // It's not safe to call clearScript until all the cleanup tasks posted by functions initiated by WorkerThreadShutdownStartTask have completed.
        workerContext->clearScript();
        workerContext->thread()->runLoop().terminate();
    }

    virtual bool isCleanupTask() const { return true; }
};

class WorkerThreadShutdownStartTask : public ScriptExecutionContext::Task {
public:
    static PassOwnPtr<WorkerThreadShutdownStartTask> create()
    {
        return adoptPtr(new WorkerThreadShutdownStartTask());
    }

    virtual void performTask(ScriptExecutionContext *context)
    {
        ASSERT(context->isWorkerContext());
        WorkerContext* workerContext = static_cast<WorkerContext*>(context);

#if ENABLE(DATABASE)
        DatabaseTaskSynchronizer cleanupSync;
        workerContext->stopDatabases(&cleanupSync);
#endif

        workerContext->stopActiveDOMObjects();

        workerContext->notifyObserversOfStop();

        // Event listeners would keep DOMWrapperWorld objects alive for too long. Also, they have references to JS objects,
        // which become dangling once Heap is destroyed.
        workerContext->removeAllEventListeners();

#if ENABLE(DATABASE)
        // We wait for the database thread to clean up all its stuff so that we
        // can do more stringent leak checks as we exit.
        cleanupSync.waitForTaskCompletion();
#endif

        // Stick a shutdown command at the end of the queue, so that we deal
        // with all the cleanup tasks the databases post first.
        workerContext->postTask(WorkerThreadShutdownFinishTask::create());
    }

    virtual bool isCleanupTask() const { return true; }
};

void WorkerThread::stop()
{
    // Mutex protection is necessary because stop() can be called before the context is fully created.
    MutexLocker lock(m_threadCreationMutex);

    // Ensure that tasks are being handled by thread event loop. If script execution weren't forbidden, a while(1) loop in JS could keep the thread alive forever.
    if (m_workerContext) {
        m_workerContext->script()->scheduleExecutionTermination();

#if ENABLE(DATABASE)
        DatabaseTracker::tracker().interruptAllDatabasesForContext(m_workerContext.get());
#endif

    // FIXME: Rudely killing the thread won't work when we allow nested workers, because they will try to post notifications of their destruction.
    // This can likely use the same mechanism as used for databases above.

        m_runLoop.postTask(WorkerThreadShutdownStartTask::create());
    } else
        m_runLoop.terminate();
}

} // namespace WebCore

#endif // ENABLE(WORKERS)
