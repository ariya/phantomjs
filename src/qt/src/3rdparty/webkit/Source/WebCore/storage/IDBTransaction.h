/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef IDBTransaction_h
#define IDBTransaction_h

#if ENABLE(INDEXED_DATABASE)

#include "ActiveDOMObject.h"
#include "DOMStringList.h"
#include "Event.h"
#include "EventListener.h"
#include "EventNames.h"
#include "EventTarget.h"
#include "IDBTransactionBackendInterface.h"
#include "IDBTransactionCallbacks.h"
#include <wtf/RefCounted.h>

namespace WebCore {

class IDBDatabase;
class IDBObjectStore;

class IDBTransaction : public IDBTransactionCallbacks, public EventTarget, public ActiveDOMObject {
public:
    static PassRefPtr<IDBTransaction> create(ScriptExecutionContext*, PassRefPtr<IDBTransactionBackendInterface>, IDBDatabase*);
    virtual ~IDBTransaction();

    enum Mode {
        READ_WRITE = 0,
        READ_ONLY = 1,
        VERSION_CHANGE = 2
    };

    IDBTransactionBackendInterface* backend() const;
    bool finished() const;

    unsigned short mode() const;
    IDBDatabase* db() const;
    PassRefPtr<IDBObjectStore> objectStore(const String& name, ExceptionCode&);
    void abort();

    void registerRequest(IDBRequest*);
    void unregisterRequest(IDBRequest*);

    DEFINE_ATTRIBUTE_EVENT_LISTENER(abort);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(complete);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(error);

    // IDBTransactionCallbacks
    virtual void onAbort();
    virtual void onComplete();

    // EventTarget
    virtual IDBTransaction* toIDBTransaction() { return this; }
    virtual ScriptExecutionContext* scriptExecutionContext() const;
    virtual bool dispatchEvent(PassRefPtr<Event>);
    bool dispatchEvent(PassRefPtr<Event> event, ExceptionCode& ec) { return EventTarget::dispatchEvent(event, ec); }

    // ActiveDOMObject
    virtual bool hasPendingActivity() const;
    virtual bool canSuspend() const;
    virtual void contextDestroyed();

    using RefCounted<IDBTransactionCallbacks>::ref;
    using RefCounted<IDBTransactionCallbacks>::deref;

private:
    IDBTransaction(ScriptExecutionContext*, PassRefPtr<IDBTransactionBackendInterface>, IDBDatabase*);

    void enqueueEvent(PassRefPtr<Event>);

    // EventTarget
    virtual void refEventTarget() { ref(); }
    virtual void derefEventTarget() { deref(); }
    virtual EventTargetData* eventTargetData();
    virtual EventTargetData* ensureEventTargetData();

    RefPtr<IDBTransactionBackendInterface> m_backend;
    RefPtr<IDBDatabase> m_database;
    unsigned short m_mode;
    bool m_finished; // Is it possible that we'll fire any more events or allow any new transactions? If not, we're finished.

    ListHashSet<IDBRequest*> m_childRequests;

    EventTargetData m_eventTargetData;
};

} // namespace WebCore

#endif // ENABLE(INDEXED_DATABASE)

#endif // IDBTransaction_h
