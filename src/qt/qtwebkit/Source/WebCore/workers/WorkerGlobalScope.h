/*
 * Copyright (C) 2008, 2009 Apple Inc. All rights reserved.
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

#ifndef WorkerGlobalScope_h
#define WorkerGlobalScope_h

#if ENABLE(WORKERS)

#include "ContentSecurityPolicy.h"
#include "EventListener.h"
#include "EventNames.h"
#include "EventTarget.h"
#include "GroupSettings.h"
#include "ScriptExecutionContext.h"
#include "WorkerEventQueue.h"
#include "WorkerScriptController.h"
#include <wtf/Assertions.h>
#include <wtf/HashMap.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>
#include <wtf/text/AtomicStringHash.h>

namespace WebCore {

    class Blob;
    class ScheduledAction;
    class WorkerInspectorController;
    class WorkerLocation;
    class WorkerNavigator;
    class WorkerThread;

    class WorkerGlobalScope : public RefCounted<WorkerGlobalScope>, public ScriptExecutionContext, public EventTarget {
    public:
        virtual ~WorkerGlobalScope();

        virtual bool isWorkerGlobalScope() const OVERRIDE { return true; }

        virtual ScriptExecutionContext* scriptExecutionContext() const OVERRIDE;

        virtual bool isSharedWorkerGlobalScope() const { return false; }
        virtual bool isDedicatedWorkerGlobalScope() const { return false; }

        const KURL& url() const { return m_url; }
        KURL completeURL(const String&) const;

        const GroupSettings* groupSettings() { return m_groupSettings.get(); }
        virtual String userAgent(const KURL&) const;

        virtual void disableEval(const String& errorMessage) OVERRIDE;

        WorkerScriptController* script() { return m_script.get(); }
        void clearScript() { m_script.clear(); }
#if ENABLE(INSPECTOR)
        void clearInspector();
#endif

        WorkerThread* thread() const { return m_thread; }

        bool hasPendingActivity() const;

        virtual void postTask(PassOwnPtr<Task>) OVERRIDE; // Executes the task on context's thread asynchronously.

        // WorkerGlobalScope
        WorkerGlobalScope* self() { return this; }
        WorkerLocation* location() const;
        void close();

        DEFINE_ATTRIBUTE_EVENT_LISTENER(error);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(offline);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(online);

        // WorkerUtils
        virtual void importScripts(const Vector<String>& urls, ExceptionCode&);
        WorkerNavigator* navigator() const;

        // Timers
        int setTimeout(PassOwnPtr<ScheduledAction>, int timeout);
        void clearTimeout(int timeoutId);
        int setInterval(PassOwnPtr<ScheduledAction>, int timeout);
        void clearInterval(int timeoutId);

        // ScriptExecutionContext
        virtual WorkerEventQueue* eventQueue() const OVERRIDE;

        virtual bool isContextThread() const OVERRIDE;
        virtual bool isJSExecutionForbidden() const OVERRIDE;

#if ENABLE(INSPECTOR)
        WorkerInspectorController* workerInspectorController() { return m_workerInspectorController.get(); }
#endif
        // These methods are used for GC marking. See JSWorkerGlobalScope::visitChildrenVirtual(SlotVisitor&) in
        // JSWorkerGlobalScopeCustom.cpp.
        WorkerNavigator* optionalNavigator() const { return m_navigator.get(); }
        WorkerLocation* optionalLocation() const { return m_location.get(); }

        using RefCounted<WorkerGlobalScope>::ref;
        using RefCounted<WorkerGlobalScope>::deref;

        bool isClosing() { return m_closing; }

        // An observer interface to be notified when the worker thread is getting stopped.
        class Observer {
            WTF_MAKE_NONCOPYABLE(Observer);
        public:
            Observer(WorkerGlobalScope*);
            virtual ~Observer();
            virtual void notifyStop() = 0;
            void stopObserving();
        private:
            WorkerGlobalScope* m_context;
        };
        friend class Observer;
        void registerObserver(Observer*);
        void unregisterObserver(Observer*);
        void notifyObserversOfStop();

        virtual SecurityOrigin* topOrigin() const OVERRIDE { return m_topOrigin.get(); }

    protected:
        WorkerGlobalScope(const KURL&, const String& userAgent, PassOwnPtr<GroupSettings>, WorkerThread*, PassRefPtr<SecurityOrigin> topOrigin);
        void applyContentSecurityPolicyFromString(const String& contentSecurityPolicy, ContentSecurityPolicy::HeaderType);

        virtual void logExceptionToConsole(const String& errorMessage, const String& sourceURL, int lineNumber, int columnNumber, PassRefPtr<ScriptCallStack>) OVERRIDE;
        void addMessageToWorkerConsole(MessageSource, MessageLevel, const String& message, const String& sourceURL, unsigned lineNumber, unsigned columnNumber, PassRefPtr<ScriptCallStack>, ScriptState* = 0, unsigned long requestIdentifier = 0);

    private:
        virtual void refScriptExecutionContext() OVERRIDE { ref(); }
        virtual void derefScriptExecutionContext() OVERRIDE { deref(); }

        virtual void refEventTarget() OVERRIDE { ref(); }
        virtual void derefEventTarget() OVERRIDE { deref(); }
        virtual EventTargetData* eventTargetData() OVERRIDE;
        virtual EventTargetData* ensureEventTargetData() OVERRIDE;

        virtual const KURL& virtualURL() const OVERRIDE;
        virtual KURL virtualCompleteURL(const String&) const;

        virtual void addMessage(MessageSource, MessageLevel, const String& message, const String& sourceURL, unsigned lineNumber, unsigned columnNumber, PassRefPtr<ScriptCallStack>, ScriptState* = 0, unsigned long requestIdentifier = 0) OVERRIDE;
        virtual void addConsoleMessage(MessageSource, MessageLevel, const String& message, unsigned long requestIdentifier = 0) OVERRIDE;

        virtual EventTarget* errorEventTarget() OVERRIDE;

        KURL m_url;
        String m_userAgent;
        OwnPtr<GroupSettings> m_groupSettings;

        mutable RefPtr<WorkerLocation> m_location;
        mutable RefPtr<WorkerNavigator> m_navigator;

        OwnPtr<WorkerScriptController> m_script;
        WorkerThread* m_thread;

#if ENABLE(INSPECTOR)
        OwnPtr<WorkerInspectorController> m_workerInspectorController;
#endif
        bool m_closing;
        EventTargetData m_eventTargetData;

        HashSet<Observer*> m_workerObservers;

        OwnPtr<WorkerEventQueue> m_eventQueue;

        RefPtr<SecurityOrigin> m_topOrigin;
    };

} // namespace WebCore

#endif // ENABLE(WORKERS)

#endif // WorkerGlobalScope_h
