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

#ifndef IDBDatabase_h
#define IDBDatabase_h

#include "ActiveDOMObject.h"
#include "DOMStringList.h"
#include "Dictionary.h"
#include "Event.h"
#include "EventTarget.h"
#include "IDBDatabaseCallbacks.h"
#include "IDBMetadata.h"
#include "IDBObjectStore.h"
#include "IDBTransaction.h"
#include "IndexedDB.h"
#include "ScriptWrappable.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>

#if ENABLE(INDEXED_DATABASE)

namespace WebCore {

class ScriptExecutionContext;

typedef int ExceptionCode;

class IDBDatabase : public RefCounted<IDBDatabase>, public ScriptWrappable, public EventTarget, public ActiveDOMObject {
public:
    static PassRefPtr<IDBDatabase> create(ScriptExecutionContext*, PassRefPtr<IDBDatabaseBackendInterface>, PassRefPtr<IDBDatabaseCallbacks>);
    ~IDBDatabase();

    void setMetadata(const IDBDatabaseMetadata& metadata) { m_metadata = metadata; }
    void transactionCreated(IDBTransaction*);
    void transactionFinished(IDBTransaction*);

    // Implement the IDL
    const String name() const { return m_metadata.name; }
    PassRefPtr<IDBAny> version() const;
    PassRefPtr<DOMStringList> objectStoreNames() const;

    PassRefPtr<IDBObjectStore> createObjectStore(const String& name, const Dictionary&, ExceptionCode&);
    PassRefPtr<IDBObjectStore> createObjectStore(const String& name, const IDBKeyPath&, bool autoIncrement, ExceptionCode&);
    PassRefPtr<IDBTransaction> transaction(ScriptExecutionContext* context, PassRefPtr<DOMStringList> scope, const String& mode, ExceptionCode& ec) { return transaction(context, *scope, mode, ec); }
    PassRefPtr<IDBTransaction> transaction(ScriptExecutionContext*, const Vector<String>&, const String& mode, ExceptionCode&);
    PassRefPtr<IDBTransaction> transaction(ScriptExecutionContext*, const String&, const String& mode, ExceptionCode&);
    void deleteObjectStore(const String& name, ExceptionCode&);
    void close();

    DEFINE_ATTRIBUTE_EVENT_LISTENER(abort);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(error);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(versionchange);

    // IDBDatabaseCallbacks
    virtual void onVersionChange(int64_t oldVersion, int64_t newVersion);
    virtual void onAbort(int64_t, PassRefPtr<IDBDatabaseError>);
    virtual void onComplete(int64_t);

    // ActiveDOMObject
    virtual bool hasPendingActivity() const OVERRIDE;
    virtual void stop() OVERRIDE;

    // EventTarget
    virtual const AtomicString& interfaceName() const;
    virtual ScriptExecutionContext* scriptExecutionContext() const;

    bool isClosePending() const { return m_closePending; }
    void forceClose();
    const IDBDatabaseMetadata metadata() const { return m_metadata; }
    void enqueueEvent(PassRefPtr<Event>);

    using EventTarget::dispatchEvent;
    virtual bool dispatchEvent(PassRefPtr<Event>) OVERRIDE;

    int64_t findObjectStoreId(const String& name) const;
    bool containsObjectStore(const String& name) const
    {
        return findObjectStoreId(name) != IDBObjectStoreMetadata::InvalidId;
    }

    IDBDatabaseBackendInterface* backend() const { return m_backend.get(); }

    static int64_t nextTransactionId();

    using RefCounted<IDBDatabase>::ref;
    using RefCounted<IDBDatabase>::deref;

private:
    IDBDatabase(ScriptExecutionContext*, PassRefPtr<IDBDatabaseBackendInterface>, PassRefPtr<IDBDatabaseCallbacks>);

    // EventTarget
    virtual void refEventTarget() { ref(); }
    virtual void derefEventTarget() { deref(); }
    virtual EventTargetData* eventTargetData();
    virtual EventTargetData* ensureEventTargetData();

    void closeConnection();

    IDBDatabaseMetadata m_metadata;
    RefPtr<IDBDatabaseBackendInterface> m_backend;
    RefPtr<IDBTransaction> m_versionChangeTransaction;
    typedef HashMap<int64_t, IDBTransaction*> TransactionMap;
    TransactionMap m_transactions;

    bool m_closePending;
    bool m_contextStopped;

    EventTargetData m_eventTargetData;

    // Keep track of the versionchange events waiting to be fired on this
    // database so that we can cancel them if the database closes.
    Vector<RefPtr<Event> > m_enqueuedEvents;

    RefPtr<IDBDatabaseCallbacks> m_databaseCallbacks;
};

} // namespace WebCore

#endif

#endif // IDBDatabase_h
