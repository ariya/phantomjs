/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
 * Copyright (C) 2012 Google Inc. All Rights Reserved.
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

#ifndef ScriptExecutionContext_h
#define ScriptExecutionContext_h

#include "ActiveDOMObject.h"
#include "ConsoleTypes.h"
#include "KURL.h"
#include "SecurityContext.h"
#include "Supplementable.h"
#include <wtf/HashSet.h>

namespace JSC {
class ExecState;
class VM;
}

namespace WebCore {

class CachedScript;
class DatabaseContext;
class DOMTimer;
class EventListener;
class EventQueue;
class EventTarget;
class MessagePort;
class ScriptCallStack;

typedef JSC::ExecState ScriptState;

#if ENABLE(BLOB)
class PublicURLManager;
#endif

class ScriptExecutionContext : public SecurityContext, public Supplementable<ScriptExecutionContext> {
public:
    ScriptExecutionContext();
    virtual ~ScriptExecutionContext();

    virtual bool isDocument() const { return false; }
    virtual bool isWorkerGlobalScope() const { return false; }

    virtual bool isContextThread() const { return true; }
    virtual bool isJSExecutionForbidden() const = 0;

    const KURL& url() const { return virtualURL(); }
    KURL completeURL(const String& url) const { return virtualCompleteURL(url); }

    virtual String userAgent(const KURL&) const = 0;

    virtual void disableEval(const String& errorMessage) = 0;

    bool sanitizeScriptError(String& errorMessage, int& lineNumber, int& columnNumber, String& sourceURL, CachedScript* = 0);
    // FIXME: <http://webkit.org/b/114315> ScriptExecutionContext log exception should include a column number
    void reportException(const String& errorMessage, int lineNumber, int columnNumber, const String& sourceURL, PassRefPtr<ScriptCallStack>, CachedScript* = 0);

    void addConsoleMessage(MessageSource, MessageLevel, const String& message, const String& sourceURL, unsigned lineNumber, unsigned columnNumber, ScriptState* = 0, unsigned long requestIdentifier = 0);
    virtual void addConsoleMessage(MessageSource, MessageLevel, const String& message, unsigned long requestIdentifier = 0) = 0;

    virtual SecurityOrigin* topOrigin() const = 0;

#if ENABLE(BLOB)
    PublicURLManager& publicURLManager();
#endif
    // Active objects are not garbage collected even if inaccessible, e.g. because their activity may result in callbacks being invoked.
    bool canSuspendActiveDOMObjects();
    // Active objects can be asked to suspend even if canSuspendActiveDOMObjects() returns 'false' -
    // step-by-step JS debugging is one example.
    virtual void suspendActiveDOMObjects(ActiveDOMObject::ReasonForSuspension);
    virtual void resumeActiveDOMObjects(ActiveDOMObject::ReasonForSuspension);
    virtual void stopActiveDOMObjects();

    bool activeDOMObjectsAreSuspended() const { return m_activeDOMObjectsAreSuspended; }
    bool activeDOMObjectsAreStopped() const { return m_activeDOMObjectsAreStopped; }

    // Called from the constructor and destructors of ActiveDOMObject.
    void didCreateActiveDOMObject(ActiveDOMObject*);
    void willDestroyActiveDOMObject(ActiveDOMObject*);

    // Called after the construction of an ActiveDOMObject to synchronize suspend state.
    void suspendActiveDOMObjectIfNeeded(ActiveDOMObject*);

    typedef HashSet<ActiveDOMObject*> ActiveDOMObjectsSet;
    const ActiveDOMObjectsSet& activeDOMObjects() const { return m_activeDOMObjects; }

    void didCreateDestructionObserver(ContextDestructionObserver*);
    void willDestroyDestructionObserver(ContextDestructionObserver*);

    // MessagePort is conceptually a kind of ActiveDOMObject, but it needs to be tracked separately for message dispatch.
    void processMessagePortMessagesSoon();
    void dispatchMessagePortEvents();
    void createdMessagePort(MessagePort*);
    void destroyedMessagePort(MessagePort*);
    const HashSet<MessagePort*>& messagePorts() const { return m_messagePorts; }

    void ref() { refScriptExecutionContext(); }
    void deref() { derefScriptExecutionContext(); }

    class Task {
        WTF_MAKE_NONCOPYABLE(Task);
        WTF_MAKE_FAST_ALLOCATED;
    public:
        Task() { }
        virtual ~Task();
        virtual void performTask(ScriptExecutionContext*) = 0;
        // Certain tasks get marked specially so that they aren't discarded, and are executed, when the context is shutting down its message queue.
        virtual bool isCleanupTask() const { return false; }
    };

    virtual void postTask(PassOwnPtr<Task>) = 0; // Executes the task on context's thread asynchronously.

    // Gets the next id in a circular sequence from 1 to 2^31-1.
    int circularSequentialID();

    bool addTimeout(int timeoutId, DOMTimer* timer) { return m_timeouts.add(timeoutId, timer).isNewEntry; }
    void removeTimeout(int timeoutId) { m_timeouts.remove(timeoutId); }
    DOMTimer* findTimeout(int timeoutId) { return m_timeouts.get(timeoutId); }

    JSC::VM* vm();

    // Interval is in seconds.
    void adjustMinimumTimerInterval(double oldMinimumTimerInterval);
    virtual double minimumTimerInterval() const;

    void didChangeTimerAlignmentInterval();
    virtual double timerAlignmentInterval() const;

    virtual EventQueue* eventQueue() const = 0;

#if ENABLE(SQL_DATABASE)
    void setDatabaseContext(DatabaseContext*);
#endif

protected:
    class AddConsoleMessageTask : public Task {
    public:
        static PassOwnPtr<AddConsoleMessageTask> create(MessageSource source, MessageLevel level, const String& message)
        {
            return adoptPtr(new AddConsoleMessageTask(source, level, message));
        }
        virtual void performTask(ScriptExecutionContext*);
    private:
        AddConsoleMessageTask(MessageSource source, MessageLevel level, const String& message)
            : m_source(source)
            , m_level(level)
            , m_message(message.isolatedCopy())
        {
        }
        MessageSource m_source;
        MessageLevel m_level;
        String m_message;
    };

    ActiveDOMObject::ReasonForSuspension reasonForSuspendingActiveDOMObjects() const { return m_reasonForSuspendingActiveDOMObjects; }

private:
    virtual const KURL& virtualURL() const = 0;
    virtual KURL virtualCompleteURL(const String&) const = 0;

    virtual void addMessage(MessageSource, MessageLevel, const String& message, const String& sourceURL, unsigned lineNumber, unsigned columnNumber, PassRefPtr<ScriptCallStack>, ScriptState* = 0, unsigned long requestIdentifier = 0) = 0;
    virtual EventTarget* errorEventTarget() = 0;
    virtual void logExceptionToConsole(const String& errorMessage, const String& sourceURL, int lineNumber, int columnNumber, PassRefPtr<ScriptCallStack>) = 0;
    bool dispatchErrorEvent(const String& errorMessage, int lineNumber, int columnNumber, const String& sourceURL, CachedScript*);

    void closeMessagePorts();

    virtual void refScriptExecutionContext() = 0;
    virtual void derefScriptExecutionContext() = 0;

    HashSet<MessagePort*> m_messagePorts;
    HashSet<ContextDestructionObserver*> m_destructionObservers;
    ActiveDOMObjectsSet m_activeDOMObjects;
    bool m_iteratingActiveDOMObjects;
    bool m_inDestructor;

    int m_circularSequentialID;
    typedef HashMap<int, DOMTimer*> TimeoutMap;
    TimeoutMap m_timeouts;

    bool m_inDispatchErrorEvent;
    class PendingException;
    OwnPtr<Vector<OwnPtr<PendingException> > > m_pendingExceptions;

    bool m_activeDOMObjectsAreSuspended;
    ActiveDOMObject::ReasonForSuspension m_reasonForSuspendingActiveDOMObjects;
    bool m_activeDOMObjectsAreStopped;

#if ENABLE(BLOB)
    OwnPtr<PublicURLManager> m_publicURLManager;
#endif

#if ENABLE(SQL_DATABASE)
    RefPtr<DatabaseContext> m_databaseContext;
#endif
};

} // namespace WebCore

#endif // ScriptExecutionContext_h
