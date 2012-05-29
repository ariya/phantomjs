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

#ifndef WorkerContext_h
#define WorkerContext_h

#if ENABLE(WORKERS)

#include "EventListener.h"
#include "EventNames.h"
#include "EventTarget.h"
#include "ScriptExecutionContext.h"
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
    class DOMFileSystemSync;
    class DOMURL;
    class Database;
    class DatabaseCallback;
    class DatabaseSync;
    class EntryCallback;
    class EntrySync;
    class ErrorCallback;
    class FileSystemCallback;
    class NotificationCenter;
    class ScheduledAction;
    class WorkerInspectorController;
    class WorkerLocation;
    class WorkerNavigator;
    class WorkerThread;

    class WorkerContext : public RefCounted<WorkerContext>, public ScriptExecutionContext, public EventTarget {
    public:
        virtual ~WorkerContext();

        virtual bool isWorkerContext() const { return true; }

        virtual ScriptExecutionContext* scriptExecutionContext() const;

        virtual bool isSharedWorkerContext() const { return false; }
        virtual bool isDedicatedWorkerContext() const { return false; }

        const KURL& url() const { return m_url; }
        KURL completeURL(const String&) const;

        virtual String userAgent(const KURL&) const;

        WorkerScriptController* script() { return m_script.get(); }
        void clearScript() { m_script.clear(); }

        WorkerThread* thread() const { return m_thread; }

        bool hasPendingActivity() const;

        virtual void postTask(PassOwnPtr<Task>); // Executes the task on context's thread asynchronously.

        // WorkerGlobalScope
        WorkerContext* self() { return this; }
        WorkerLocation* location() const;
        void close();

        DEFINE_ATTRIBUTE_EVENT_LISTENER(error);

        // WorkerUtils
        virtual void importScripts(const Vector<String>& urls, ExceptionCode&);
        WorkerNavigator* navigator() const;

        // Timers
        int setTimeout(PassOwnPtr<ScheduledAction>, int timeout);
        void clearTimeout(int timeoutId);
        int setInterval(PassOwnPtr<ScheduledAction>, int timeout);
        void clearInterval(int timeoutId);

        // ScriptExecutionContext
        virtual void addMessage(MessageSource, MessageType, MessageLevel, const String& message, unsigned lineNumber, const String& sourceURL, PassRefPtr<ScriptCallStack>);

#if ENABLE(NOTIFICATIONS)
        NotificationCenter* webkitNotifications() const;
#endif

#if ENABLE(DATABASE)
        // HTML 5 client-side database
        PassRefPtr<Database> openDatabase(const String& name, const String& version, const String& displayName, unsigned long estimatedSize, PassRefPtr<DatabaseCallback> creationCallback, ExceptionCode&);
        PassRefPtr<DatabaseSync> openDatabaseSync(const String& name, const String& version, const String& displayName, unsigned long estimatedSize, PassRefPtr<DatabaseCallback> creationCallback, ExceptionCode&);

        // Not implemented yet.
        virtual bool allowDatabaseAccess() const { return true; }
        // Not implemented for real yet.
        virtual void databaseExceededQuota(const String&);
#endif
        virtual bool isContextThread() const;
        virtual bool isJSExecutionForbidden() const;

#if ENABLE(BLOB)
        DOMURL* webkitURL() const;
#endif

#if ENABLE(FILE_SYSTEM)
        enum FileSystemType {
            TEMPORARY,
            PERSISTENT,
            EXTERNAL,
        };
        void webkitRequestFileSystem(int type, long long size, PassRefPtr<FileSystemCallback> successCallback, PassRefPtr<ErrorCallback>);
        PassRefPtr<DOMFileSystemSync> webkitRequestFileSystemSync(int type, long long size, ExceptionCode&);
        void webkitResolveLocalFileSystemURL(const String& url, PassRefPtr<EntryCallback> successCallback, PassRefPtr<ErrorCallback>);
        PassRefPtr<EntrySync> webkitResolveLocalFileSystemSyncURL(const String& url, ExceptionCode&);
#endif
#if ENABLE(INSPECTOR)
        WorkerInspectorController* workerInspectorController() { return m_workerInspectorController.get(); }
#endif
        // These methods are used for GC marking. See JSWorkerContext::visitChildren(SlotVisitor&) in
        // JSWorkerContextCustom.cpp.
        WorkerNavigator* optionalNavigator() const { return m_navigator.get(); }
        WorkerLocation* optionalLocation() const { return m_location.get(); }

        using RefCounted<WorkerContext>::ref;
        using RefCounted<WorkerContext>::deref;

        bool isClosing() { return m_closing; }

        // An observer interface to be notified when the worker thread is getting stopped.
        class Observer {
            WTF_MAKE_NONCOPYABLE(Observer);
        public:
            Observer(WorkerContext*);
            virtual ~Observer();
            virtual void notifyStop() = 0;
            void stopObserving();
        private:
            WorkerContext* m_context;
        };
        friend class Observer;
        void registerObserver(Observer*);
        void unregisterObserver(Observer*);
        void notifyObserversOfStop();

    protected:
        WorkerContext(const KURL&, const String&, WorkerThread*);

    private:
        virtual void refScriptExecutionContext() { ref(); }
        virtual void derefScriptExecutionContext() { deref(); }

        virtual void refEventTarget() { ref(); }
        virtual void derefEventTarget() { deref(); }
        virtual EventTargetData* eventTargetData();
        virtual EventTargetData* ensureEventTargetData();

        virtual const KURL& virtualURL() const;
        virtual KURL virtualCompleteURL(const String&) const;

        virtual EventTarget* errorEventTarget();
        virtual void logExceptionToConsole(const String& errorMessage, int lineNumber, const String& sourceURL, PassRefPtr<ScriptCallStack>);

        KURL m_url;
        String m_userAgent;

        mutable RefPtr<WorkerLocation> m_location;
        mutable RefPtr<WorkerNavigator> m_navigator;

        OwnPtr<WorkerScriptController> m_script;
        WorkerThread* m_thread;

#if ENABLE_NOTIFICATIONS
        mutable RefPtr<NotificationCenter> m_notifications;
#endif
#if ENABLE(BLOB)
        mutable RefPtr<DOMURL> m_domURL;
#endif
#if ENABLE(INSPECTOR)
        OwnPtr<WorkerInspectorController> m_workerInspectorController;
#endif
        bool m_closing;
        EventTargetData m_eventTargetData;

        HashSet<Observer*> m_workerObservers;
    };

} // namespace WebCore

#endif // ENABLE(WORKERS)

#endif // WorkerContext_h
