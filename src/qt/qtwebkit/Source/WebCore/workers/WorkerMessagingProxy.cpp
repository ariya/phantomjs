/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
 * Copyright (C) 2009 Google Inc. All Rights Reserved.
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

#include "WorkerMessagingProxy.h"

#include "ContentSecurityPolicy.h"
#include "CrossThreadTask.h"
#include "DedicatedWorkerGlobalScope.h"
#include "DedicatedWorkerThread.h"
#include "DOMWindow.h"
#include "Document.h"
#include "ErrorEvent.h"
#include "Event.h"
#include "EventNames.h"
#include "ExceptionCode.h"
#include "InspectorInstrumentation.h"
#include "MessageEvent.h"
#include "NotImplemented.h"
#include "PageGroup.h"
#include "ScriptCallStack.h"
#include "ScriptExecutionContext.h"
#include "Worker.h"
#include "WorkerDebuggerAgent.h"
#include "WorkerInspectorController.h"
#include <wtf/MainThread.h>

namespace WebCore {

class MessageWorkerGlobalScopeTask : public ScriptExecutionContext::Task {
public:
    static PassOwnPtr<MessageWorkerGlobalScopeTask> create(PassRefPtr<SerializedScriptValue> message, PassOwnPtr<MessagePortChannelArray> channels)
    {
        return adoptPtr(new MessageWorkerGlobalScopeTask(message, channels));
    }

private:
    MessageWorkerGlobalScopeTask(PassRefPtr<SerializedScriptValue> message, PassOwnPtr<MessagePortChannelArray> channels)
        : m_message(message)
        , m_channels(channels)
    {
    }

    virtual void performTask(ScriptExecutionContext* scriptContext)
    {
        ASSERT_WITH_SECURITY_IMPLICATION(scriptContext->isWorkerGlobalScope());
        DedicatedWorkerGlobalScope* context = static_cast<DedicatedWorkerGlobalScope*>(scriptContext);
        OwnPtr<MessagePortArray> ports = MessagePort::entanglePorts(*scriptContext, m_channels.release());
        context->dispatchEvent(MessageEvent::create(ports.release(), m_message));
        context->thread()->workerObjectProxy().confirmMessageFromWorkerObject(context->hasPendingActivity());
    }

private:
    RefPtr<SerializedScriptValue> m_message;
    OwnPtr<MessagePortChannelArray> m_channels;
};

class MessageWorkerTask : public ScriptExecutionContext::Task {
public:
    static PassOwnPtr<MessageWorkerTask> create(PassRefPtr<SerializedScriptValue> message, PassOwnPtr<MessagePortChannelArray> channels, WorkerMessagingProxy* messagingProxy)
    {
        return adoptPtr(new MessageWorkerTask(message, channels, messagingProxy));
    }

private:
    MessageWorkerTask(PassRefPtr<SerializedScriptValue> message, PassOwnPtr<MessagePortChannelArray> channels, WorkerMessagingProxy* messagingProxy)
        : m_message(message)
        , m_channels(channels)
        , m_messagingProxy(messagingProxy)
    {
    }

    virtual void performTask(ScriptExecutionContext* scriptContext)
    {
        Worker* workerObject = m_messagingProxy->workerObject();
        if (!workerObject || m_messagingProxy->askedToTerminate())
            return;

        OwnPtr<MessagePortArray> ports = MessagePort::entanglePorts(*scriptContext, m_channels.release());
        workerObject->dispatchEvent(MessageEvent::create(ports.release(), m_message));
    }

private:
    RefPtr<SerializedScriptValue> m_message;
    OwnPtr<MessagePortChannelArray> m_channels;
    WorkerMessagingProxy* m_messagingProxy;
};

class WorkerExceptionTask : public ScriptExecutionContext::Task {
public:
    static PassOwnPtr<WorkerExceptionTask> create(const String& errorMessage, int lineNumber, int columnNumber, const String& sourceURL, WorkerMessagingProxy* messagingProxy)
    {
        return adoptPtr(new WorkerExceptionTask(errorMessage, lineNumber, columnNumber, sourceURL, messagingProxy));
    }

private:
    WorkerExceptionTask(const String& errorMessage, int lineNumber, int columnNumber, const String& sourceURL, WorkerMessagingProxy* messagingProxy)
        : m_errorMessage(errorMessage.isolatedCopy())
        , m_lineNumber(lineNumber)
        , m_columnNumber(columnNumber)
        , m_sourceURL(sourceURL.isolatedCopy())
        , m_messagingProxy(messagingProxy)
    {
    }

    virtual void performTask(ScriptExecutionContext* context)
    {
        Worker* workerObject = m_messagingProxy->workerObject();
        if (!workerObject)
            return;

        // We don't bother checking the askedToTerminate() flag here, because exceptions should *always* be reported even if the thread is terminated.
        // This is intentionally different than the behavior in MessageWorkerTask, because terminated workers no longer deliver messages (section 4.6 of the WebWorker spec), but they do report exceptions.

        bool errorHandled = !workerObject->dispatchEvent(ErrorEvent::create(m_errorMessage, m_sourceURL, m_lineNumber, m_columnNumber));
        if (!errorHandled)
            context->reportException(m_errorMessage, m_lineNumber, m_columnNumber, m_sourceURL, 0);
    }

    String m_errorMessage;
    int m_lineNumber;
    int m_columnNumber;
    String m_sourceURL;
    WorkerMessagingProxy* m_messagingProxy;
};

class WorkerGlobalScopeDestroyedTask : public ScriptExecutionContext::Task {
public:
    static PassOwnPtr<WorkerGlobalScopeDestroyedTask> create(WorkerMessagingProxy* messagingProxy)
    {
        return adoptPtr(new WorkerGlobalScopeDestroyedTask(messagingProxy));
    }

private:
    WorkerGlobalScopeDestroyedTask(WorkerMessagingProxy* messagingProxy)
        : m_messagingProxy(messagingProxy)
    {
    }

    virtual void performTask(ScriptExecutionContext*)
    {
        m_messagingProxy->workerGlobalScopeDestroyedInternal();
    }

    WorkerMessagingProxy* m_messagingProxy;
};

class WorkerTerminateTask : public ScriptExecutionContext::Task {
public:
    static PassOwnPtr<WorkerTerminateTask> create(WorkerMessagingProxy* messagingProxy)
    {
        return adoptPtr(new WorkerTerminateTask(messagingProxy));
    }

private:
    WorkerTerminateTask(WorkerMessagingProxy* messagingProxy)
        : m_messagingProxy(messagingProxy)
    {
    }

    virtual void performTask(ScriptExecutionContext*)
    {
        m_messagingProxy->terminateWorkerGlobalScope();
    }

    WorkerMessagingProxy* m_messagingProxy;
};

class WorkerThreadActivityReportTask : public ScriptExecutionContext::Task {
public:
    static PassOwnPtr<WorkerThreadActivityReportTask> create(WorkerMessagingProxy* messagingProxy, bool confirmingMessage, bool hasPendingActivity)
    {
        return adoptPtr(new WorkerThreadActivityReportTask(messagingProxy, confirmingMessage, hasPendingActivity));
    }

private:
    WorkerThreadActivityReportTask(WorkerMessagingProxy* messagingProxy, bool confirmingMessage, bool hasPendingActivity)
        : m_messagingProxy(messagingProxy)
        , m_confirmingMessage(confirmingMessage)
        , m_hasPendingActivity(hasPendingActivity)
    {
    }

    virtual void performTask(ScriptExecutionContext*)
    {
        m_messagingProxy->reportPendingActivityInternal(m_confirmingMessage, m_hasPendingActivity);
    }

    WorkerMessagingProxy* m_messagingProxy;
    bool m_confirmingMessage;
    bool m_hasPendingActivity;
};

class PostMessageToPageInspectorTask : public ScriptExecutionContext::Task {
public:
    static PassOwnPtr<PostMessageToPageInspectorTask> create(WorkerMessagingProxy* messagingProxy, const String& message)
    {
        return adoptPtr(new PostMessageToPageInspectorTask(messagingProxy, message));
    }

private:
    PostMessageToPageInspectorTask(WorkerMessagingProxy* messagingProxy, const String& message)
        : m_messagingProxy(messagingProxy)
        , m_message(message.isolatedCopy())
    {
    }

    virtual void performTask(ScriptExecutionContext*)
    {
#if ENABLE(INSPECTOR)
        if (WorkerGlobalScopeProxy::PageInspector* pageInspector = m_messagingProxy->m_pageInspector)
            pageInspector->dispatchMessageFromWorker(m_message);
#endif
    }

    WorkerMessagingProxy* m_messagingProxy;
    String m_message;
};

class NotifyNetworkStateChangeTask : public ScriptExecutionContext::Task {
public:
    static PassOwnPtr<NotifyNetworkStateChangeTask> create(bool isOnLine)
    {
        return adoptPtr(new NotifyNetworkStateChangeTask(isOnLine));
    }

private:
    NotifyNetworkStateChangeTask(bool isOnLine)
        : m_isOnLine(isOnLine)
    {
    }

    virtual void performTask(ScriptExecutionContext *context)
    {
        AtomicString eventName = m_isOnLine ? eventNames().onlineEvent : eventNames().offlineEvent;
        WorkerGlobalScope* workerGlobalScope = static_cast<WorkerGlobalScope*>(context);
        workerGlobalScope->dispatchEvent(Event::create(eventName, false, false));
    }

    bool m_isOnLine;
};


WorkerGlobalScopeProxy* WorkerGlobalScopeProxy::create(Worker* worker)
{
    return new WorkerMessagingProxy(worker);
}

WorkerMessagingProxy::WorkerMessagingProxy(Worker* workerObject)
    : m_scriptExecutionContext(workerObject->scriptExecutionContext())
    , m_workerObject(workerObject)
    , m_mayBeDestroyed(false)
    , m_unconfirmedMessageCount(0)
    , m_workerThreadHadPendingActivity(false)
    , m_askedToTerminate(false)
#if ENABLE(INSPECTOR)
    , m_pageInspector(0)
#endif
{
    ASSERT(m_workerObject);
    ASSERT((m_scriptExecutionContext->isDocument() && isMainThread())
           || (m_scriptExecutionContext->isWorkerGlobalScope() && currentThread() == static_cast<WorkerGlobalScope*>(m_scriptExecutionContext.get())->thread()->threadID()));
}

WorkerMessagingProxy::~WorkerMessagingProxy()
{
    ASSERT(!m_workerObject);
    ASSERT((m_scriptExecutionContext->isDocument() && isMainThread())
           || (m_scriptExecutionContext->isWorkerGlobalScope() && currentThread() == static_cast<WorkerGlobalScope*>(m_scriptExecutionContext.get())->thread()->threadID()));
}

void WorkerMessagingProxy::startWorkerGlobalScope(const KURL& scriptURL, const String& userAgent, const String& sourceCode, WorkerThreadStartMode startMode)
{
    // FIXME: This need to be revisited when we support nested worker one day
    ASSERT(m_scriptExecutionContext->isDocument());
    Document* document = static_cast<Document*>(m_scriptExecutionContext.get());
    GroupSettings* settings = 0;
    if (document->page())
        settings = document->page()->group().groupSettings();
    RefPtr<DedicatedWorkerThread> thread = DedicatedWorkerThread::create(scriptURL, userAgent, settings, sourceCode, *this, *this, startMode, document->contentSecurityPolicy()->deprecatedHeader(), document->contentSecurityPolicy()->deprecatedHeaderType(), document->topOrigin());
    workerThreadCreated(thread);
    thread->start();
    InspectorInstrumentation::didStartWorkerGlobalScope(m_scriptExecutionContext.get(), this, scriptURL);
}

void WorkerMessagingProxy::postMessageToWorkerObject(PassRefPtr<SerializedScriptValue> message, PassOwnPtr<MessagePortChannelArray> channels)
{
    m_scriptExecutionContext->postTask(MessageWorkerTask::create(message, channels, this));
}

void WorkerMessagingProxy::postMessageToWorkerGlobalScope(PassRefPtr<SerializedScriptValue> message, PassOwnPtr<MessagePortChannelArray> channels)
{
    if (m_askedToTerminate)
        return;

    if (m_workerThread) {
        ++m_unconfirmedMessageCount;
        m_workerThread->runLoop().postTask(MessageWorkerGlobalScopeTask::create(message, channels));
    } else
        m_queuedEarlyTasks.append(MessageWorkerGlobalScopeTask::create(message, channels));
}

bool WorkerMessagingProxy::postTaskForModeToWorkerGlobalScope(PassOwnPtr<ScriptExecutionContext::Task> task, const String& mode)
{
    if (m_askedToTerminate)
        return false;

    ASSERT(m_workerThread);
    m_workerThread->runLoop().postTaskForMode(task, mode);
    return true;
}

void WorkerMessagingProxy::postTaskToLoader(PassOwnPtr<ScriptExecutionContext::Task> task)
{
    // FIXME: In case of nested workers, this should go directly to the root Document context.
    ASSERT(m_scriptExecutionContext->isDocument());
    m_scriptExecutionContext->postTask(task);
}

void WorkerMessagingProxy::postExceptionToWorkerObject(const String& errorMessage, int lineNumber, int columnNumber, const String& sourceURL)
{
    m_scriptExecutionContext->postTask(WorkerExceptionTask::create(errorMessage, lineNumber, columnNumber, sourceURL, this));
}

static void postConsoleMessageTask(ScriptExecutionContext* context, WorkerMessagingProxy* messagingProxy, MessageSource source, MessageLevel level, const String& message, unsigned lineNumber, unsigned columnNumber, const String& sourceURL)
{
    if (messagingProxy->askedToTerminate())
        return;
    context->addConsoleMessage(source, level, message, sourceURL, lineNumber, columnNumber);
}

void WorkerMessagingProxy::postConsoleMessageToWorkerObject(MessageSource source, MessageLevel level, const String& message, int lineNumber, int columnNumber, const String& sourceURL)
{
    m_scriptExecutionContext->postTask(createCallbackTask(&postConsoleMessageTask, AllowCrossThreadAccess(this), source, level, message, lineNumber, columnNumber, sourceURL));
}

void WorkerMessagingProxy::workerThreadCreated(PassRefPtr<DedicatedWorkerThread> workerThread)
{
    m_workerThread = workerThread;

    if (m_askedToTerminate) {
        // Worker.terminate() could be called from JS before the thread was created.
        m_workerThread->stop();
    } else {
        unsigned taskCount = m_queuedEarlyTasks.size();
        ASSERT(!m_unconfirmedMessageCount);
        m_unconfirmedMessageCount = taskCount;
        m_workerThreadHadPendingActivity = true; // Worker initialization means a pending activity.

        for (unsigned i = 0; i < taskCount; ++i)
            m_workerThread->runLoop().postTask(m_queuedEarlyTasks[i].release());
        m_queuedEarlyTasks.clear();
    }
}

void WorkerMessagingProxy::workerObjectDestroyed()
{
    m_workerObject = 0;
    m_scriptExecutionContext->postTask(createCallbackTask(&workerObjectDestroyedInternal, AllowCrossThreadAccess(this)));
}

void WorkerMessagingProxy::notifyNetworkStateChange(bool isOnline)
{
    if (m_askedToTerminate)
        return;

    if (!m_workerThread)
        return;

    m_workerThread->runLoop().postTask(NotifyNetworkStateChangeTask::create(isOnline));
}

void WorkerMessagingProxy::workerObjectDestroyedInternal(ScriptExecutionContext*, WorkerMessagingProxy* proxy)
{
    proxy->m_mayBeDestroyed = true;
    if (proxy->m_workerThread)
        proxy->terminateWorkerGlobalScope();
    else
        proxy->workerGlobalScopeDestroyedInternal();
}

#if ENABLE(INSPECTOR)
#if ENABLE(JAVASCRIPT_DEBUGGER)
static void connectToWorkerGlobalScopeInspectorTask(ScriptExecutionContext* context, bool)
{
    ASSERT_WITH_SECURITY_IMPLICATION(context->isWorkerGlobalScope());
    static_cast<WorkerGlobalScope*>(context)->workerInspectorController()->connectFrontend();
}
#endif

void WorkerMessagingProxy::connectToInspector(WorkerGlobalScopeProxy::PageInspector* pageInspector)
{
    if (m_askedToTerminate)
        return;
    ASSERT(!m_pageInspector);
    m_pageInspector = pageInspector;
#if ENABLE(JAVASCRIPT_DEBUGGER)
    m_workerThread->runLoop().postTaskForMode(createCallbackTask(connectToWorkerGlobalScopeInspectorTask, true), WorkerDebuggerAgent::debuggerTaskMode);
#endif
}

#if ENABLE(JAVASCRIPT_DEBUGGER)
static void disconnectFromWorkerGlobalScopeInspectorTask(ScriptExecutionContext* context, bool)
{
    ASSERT_WITH_SECURITY_IMPLICATION(context->isWorkerGlobalScope());
    static_cast<WorkerGlobalScope*>(context)->workerInspectorController()->disconnectFrontend();
}
#endif

void WorkerMessagingProxy::disconnectFromInspector()
{
    m_pageInspector = 0;
    if (m_askedToTerminate)
        return;
#if ENABLE(JAVASCRIPT_DEBUGGER)
    m_workerThread->runLoop().postTaskForMode(createCallbackTask(disconnectFromWorkerGlobalScopeInspectorTask, true), WorkerDebuggerAgent::debuggerTaskMode);
#endif
}

#if ENABLE(JAVASCRIPT_DEBUGGER)
static void dispatchOnInspectorBackendTask(ScriptExecutionContext* context, const String& message)
{
    ASSERT_WITH_SECURITY_IMPLICATION(context->isWorkerGlobalScope());
    static_cast<WorkerGlobalScope*>(context)->workerInspectorController()->dispatchMessageFromFrontend(message);
}
#endif

void WorkerMessagingProxy::sendMessageToInspector(const String& message)
{
    if (m_askedToTerminate)
        return;
#if ENABLE(JAVASCRIPT_DEBUGGER)
    m_workerThread->runLoop().postTaskForMode(createCallbackTask(dispatchOnInspectorBackendTask, String(message)), WorkerDebuggerAgent::debuggerTaskMode);
    WorkerDebuggerAgent::interruptAndDispatchInspectorCommands(m_workerThread.get());
#endif
}
#endif

void WorkerMessagingProxy::workerGlobalScopeDestroyed()
{
    m_scriptExecutionContext->postTask(WorkerGlobalScopeDestroyedTask::create(this));
    // Will execute workerGlobalScopeDestroyedInternal() on context's thread.
}

void WorkerMessagingProxy::workerGlobalScopeClosed()
{
    // Executes terminateWorkerGlobalScope() on parent context's thread.
    m_scriptExecutionContext->postTask(WorkerTerminateTask::create(this));
}

void WorkerMessagingProxy::workerGlobalScopeDestroyedInternal()
{
    // WorkerGlobalScopeDestroyedTask is always the last to be performed, so the proxy is not needed for communication
    // in either side any more. However, the Worker object may still exist, and it assumes that the proxy exists, too.
    m_askedToTerminate = true;
    m_workerThread = 0;

    InspectorInstrumentation::workerGlobalScopeTerminated(m_scriptExecutionContext.get(), this);

    if (m_mayBeDestroyed)
        delete this;
}

void WorkerMessagingProxy::terminateWorkerGlobalScope()
{
    if (m_askedToTerminate)
        return;
    m_askedToTerminate = true;

    if (m_workerThread)
        m_workerThread->stop();

    InspectorInstrumentation::workerGlobalScopeTerminated(m_scriptExecutionContext.get(), this);
}

#if ENABLE(INSPECTOR)
void WorkerMessagingProxy::postMessageToPageInspector(const String& message)
{
    m_scriptExecutionContext->postTask(PostMessageToPageInspectorTask::create(this, message));
}

void WorkerMessagingProxy::updateInspectorStateCookie(const String&)
{
    notImplemented();
}
#endif

void WorkerMessagingProxy::confirmMessageFromWorkerObject(bool hasPendingActivity)
{
    m_scriptExecutionContext->postTask(WorkerThreadActivityReportTask::create(this, true, hasPendingActivity));
    // Will execute reportPendingActivityInternal() on context's thread.
}

void WorkerMessagingProxy::reportPendingActivity(bool hasPendingActivity)
{
    m_scriptExecutionContext->postTask(WorkerThreadActivityReportTask::create(this, false, hasPendingActivity));
    // Will execute reportPendingActivityInternal() on context's thread.
}

void WorkerMessagingProxy::reportPendingActivityInternal(bool confirmingMessage, bool hasPendingActivity)
{
    if (confirmingMessage && !m_askedToTerminate) {
        ASSERT(m_unconfirmedMessageCount);
        --m_unconfirmedMessageCount;
    }

    m_workerThreadHadPendingActivity = hasPendingActivity;
}

bool WorkerMessagingProxy::hasPendingActivity() const
{
    return (m_unconfirmedMessageCount || m_workerThreadHadPendingActivity) && !m_askedToTerminate;
}

} // namespace WebCore

#endif // ENABLE(WORKERS)
