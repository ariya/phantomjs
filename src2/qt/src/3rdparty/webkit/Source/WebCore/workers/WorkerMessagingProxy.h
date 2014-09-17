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

#ifndef WorkerMessagingProxy_h
#define WorkerMessagingProxy_h

#if ENABLE(WORKERS)

#include "ScriptExecutionContext.h"
#include "WorkerContextProxy.h"
#include "WorkerLoaderProxy.h"
#include "WorkerObjectProxy.h"
#include <wtf/Forward.h>
#include <wtf/Noncopyable.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>
#include <wtf/Vector.h>

namespace WebCore {

    class DedicatedWorkerThread;
    class ScriptExecutionContext;
    class Worker;

    class WorkerMessagingProxy : public WorkerContextProxy, public WorkerObjectProxy, public WorkerLoaderProxy {
        WTF_MAKE_NONCOPYABLE(WorkerMessagingProxy); WTF_MAKE_FAST_ALLOCATED;
    public:
        WorkerMessagingProxy(Worker*);

        // Implementations of WorkerContextProxy.
        // (Only use these methods in the worker object thread.)
        virtual void startWorkerContext(const KURL& scriptURL, const String& userAgent, const String& sourceCode);
        virtual void terminateWorkerContext();
        virtual void postMessageToWorkerContext(PassRefPtr<SerializedScriptValue>, PassOwnPtr<MessagePortChannelArray>);
        virtual bool hasPendingActivity() const;
        virtual void workerObjectDestroyed();

        // Implementations of WorkerObjectProxy.
        // (Only use these methods in the worker context thread.)
        virtual void postMessageToWorkerObject(PassRefPtr<SerializedScriptValue>, PassOwnPtr<MessagePortChannelArray>);
        virtual void postExceptionToWorkerObject(const String& errorMessage, int lineNumber, const String& sourceURL);
        virtual void postConsoleMessageToWorkerObject(MessageSource, MessageType, MessageLevel, const String& message, int lineNumber, const String& sourceURL);
        virtual void confirmMessageFromWorkerObject(bool hasPendingActivity);
        virtual void reportPendingActivity(bool hasPendingActivity);
        virtual void workerContextClosed();
        virtual void workerContextDestroyed();

        // Implementation of WorkerLoaderProxy.
        // These methods are called on different threads to schedule loading
        // requests and to send callbacks back to WorkerContext.
        virtual void postTaskToLoader(PassOwnPtr<ScriptExecutionContext::Task>);
        virtual void postTaskForModeToWorkerContext(PassOwnPtr<ScriptExecutionContext::Task>, const String& mode);

        void workerThreadCreated(PassRefPtr<DedicatedWorkerThread>);

        // Only use this method on the worker object thread.
        bool askedToTerminate() const { return m_askedToTerminate; }

    private:
        friend class MessageWorkerTask;
        friend class WorkerContextDestroyedTask;
        friend class WorkerExceptionTask;
        friend class WorkerThreadActivityReportTask;

        virtual ~WorkerMessagingProxy();

        void workerContextDestroyedInternal();
        void reportPendingActivityInternal(bool confirmingMessage, bool hasPendingActivity);
        Worker* workerObject() const { return m_workerObject; }

        RefPtr<ScriptExecutionContext> m_scriptExecutionContext;
        Worker* m_workerObject;
        RefPtr<DedicatedWorkerThread> m_workerThread;

        unsigned m_unconfirmedMessageCount; // Unconfirmed messages from worker object to worker thread.
        bool m_workerThreadHadPendingActivity; // The latest confirmation from worker thread reported that it was still active.

        bool m_askedToTerminate;

        Vector<OwnPtr<ScriptExecutionContext::Task> > m_queuedEarlyTasks; // Tasks are queued here until there's a thread object created.
    };

} // namespace WebCore

#endif // ENABLE(WORKERS)

#endif // WorkerMessagingProxy_h
