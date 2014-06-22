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
#include "DOMError.h"
#include "Event.h"
#include "EventListener.h"
#include "EventNames.h"
#include "EventTarget.h"
#include "IDBMetadata.h"
#include "IndexedDB.h"
#include "ScriptWrappable.h"
#include <wtf/HashSet.h>
#include <wtf/RefCounted.h>

namespace WebCore {

class IDBCursor;
class IDBDatabase;
class IDBDatabaseBackendInterface;
class IDBDatabaseError;
class IDBObjectStore;
class IDBOpenDBRequest;
struct IDBObjectStoreMetadata;

class IDBTransaction : public ScriptWrappable, public RefCounted<IDBTransaction>, public EventTarget, public ActiveDOMObject {
public:
    static PassRefPtr<IDBTransaction> create(ScriptExecutionContext*, int64_t, const Vector<String>& objectStoreNames, IndexedDB::TransactionMode, IDBDatabase*);
    static PassRefPtr<IDBTransaction> create(ScriptExecutionContext*, int64_t, IDBDatabase*, IDBOpenDBRequest*, const IDBDatabaseMetadata& previousMetadata);
    virtual ~IDBTransaction();

    static const AtomicString& modeReadOnly();
    static const AtomicString& modeReadWrite();
    static const AtomicString& modeVersionChange();
    static const AtomicString& modeReadOnlyLegacy();
    static const AtomicString& modeReadWriteLegacy();

    static IndexedDB::TransactionMode stringToMode(const String&, ExceptionCode&);
    static const AtomicString& modeToString(IndexedDB::TransactionMode);

    IDBDatabaseBackendInterface* backendDB() const;

    int64_t id() const { return m_id; }
    bool isActive() const { return m_state == Active; }
    bool isFinished() const { return m_state == Finished; }
    bool isReadOnly() const { return m_mode == IndexedDB::TransactionReadOnly; }
    bool isVersionChange() const { return m_mode == IndexedDB::TransactionVersionChange; }

    // Implement the IDBTransaction IDL
    const String& mode() const;
    IDBDatabase* db() const { return m_database.get(); }
    PassRefPtr<DOMError> error() const { return m_error; }
    PassRefPtr<IDBObjectStore> objectStore(const String& name, ExceptionCode&);
    void abort(ExceptionCode&);

    class OpenCursorNotifier {
    public:
        OpenCursorNotifier(PassRefPtr<IDBTransaction>, IDBCursor*);
        ~OpenCursorNotifier();
        void cursorFinished();
    private:
        RefPtr<IDBTransaction> m_transaction;
        IDBCursor* m_cursor;
    };

    void registerRequest(IDBRequest*);
    void unregisterRequest(IDBRequest*);
    void objectStoreCreated(const String&, PassRefPtr<IDBObjectStore>);
    void objectStoreDeleted(const String&);
    void setActive(bool);
    void setError(PassRefPtr<DOMError>, const String& errorMessage);
    String webkitErrorMessage() const;

    DEFINE_ATTRIBUTE_EVENT_LISTENER(abort);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(complete);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(error);

    virtual void onAbort(PassRefPtr<IDBDatabaseError>);
    virtual void onComplete();

    // EventTarget
    virtual const AtomicString& interfaceName() const;
    virtual ScriptExecutionContext* scriptExecutionContext() const;

    using EventTarget::dispatchEvent;
    virtual bool dispatchEvent(PassRefPtr<Event>) OVERRIDE;

    // ActiveDOMObject
    virtual bool hasPendingActivity() const OVERRIDE;
    virtual bool canSuspend() const OVERRIDE;
    virtual void stop() OVERRIDE;

    using RefCounted<IDBTransaction>::ref;
    using RefCounted<IDBTransaction>::deref;

private:
    IDBTransaction(ScriptExecutionContext*, int64_t, const Vector<String>&, IndexedDB::TransactionMode, IDBDatabase*, IDBOpenDBRequest*, const IDBDatabaseMetadata&);

    void enqueueEvent(PassRefPtr<Event>);
    void closeOpenCursors();

    void registerOpenCursor(IDBCursor*);
    void unregisterOpenCursor(IDBCursor*);

    // EventTarget
    virtual void refEventTarget() { ref(); }
    virtual void derefEventTarget() { deref(); }
    virtual EventTargetData* eventTargetData();
    virtual EventTargetData* ensureEventTargetData();

    enum State {
        Inactive, // Created or started, but not in an event callback
        Active, // Created or started, in creation scope or an event callback
        Finishing, // In the process of aborting or completing.
        Finished, // No more events will fire and no new requests may be filed.
    };

    int64_t m_id;
    RefPtr<IDBDatabase> m_database;
    const Vector<String> m_objectStoreNames;
    IDBOpenDBRequest* m_openDBRequest;
    const IndexedDB::TransactionMode m_mode;
    State m_state;
    bool m_hasPendingActivity;
    bool m_contextStopped;
    RefPtr<DOMError> m_error;
    String m_errorMessage;

    ListHashSet<RefPtr<IDBRequest> > m_requestList;

    typedef HashMap<String, RefPtr<IDBObjectStore> > IDBObjectStoreMap;
    IDBObjectStoreMap m_objectStoreMap;

    typedef HashSet<RefPtr<IDBObjectStore> > IDBObjectStoreSet;
    IDBObjectStoreSet m_deletedObjectStores;

    typedef HashMap<RefPtr<IDBObjectStore>, IDBObjectStoreMetadata> IDBObjectStoreMetadataMap;
    IDBObjectStoreMetadataMap m_objectStoreCleanupMap;
    IDBDatabaseMetadata m_previousMetadata;

    HashSet<IDBCursor*> m_openCursors;

    EventTargetData m_eventTargetData;
};

} // namespace WebCore

#endif // ENABLE(INDEXED_DATABASE)

#endif // IDBTransaction_h
