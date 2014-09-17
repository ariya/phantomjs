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

#ifndef IDBDatabaseBackendImpl_h
#define IDBDatabaseBackendImpl_h

#include "IDBCallbacks.h"
#include "IDBDatabase.h"
#include <wtf/Deque.h>
#include <wtf/HashMap.h>
#include <wtf/ListHashSet.h>

#if ENABLE(INDEXED_DATABASE)

namespace WebCore {

class IDBBackingStore;
class IDBDatabase;
class IDBFactoryBackendImpl;
class IDBObjectStoreBackendImpl;
class IDBTransactionCoordinator;

class IDBDatabaseBackendImpl : public IDBDatabaseBackendInterface {
public:
    static PassRefPtr<IDBDatabaseBackendImpl> create(const String& name, IDBBackingStore* database, IDBTransactionCoordinator* coordinator, IDBFactoryBackendImpl* factory, const String& uniqueIdentifier)
    {
        return adoptRef(new IDBDatabaseBackendImpl(name, database, coordinator, factory, uniqueIdentifier));
    }
    virtual ~IDBDatabaseBackendImpl();

    PassRefPtr<IDBBackingStore> backingStore() const;

    static const int64_t InvalidId = 0;
    int64_t id() const { return m_id; }
    void open(PassRefPtr<IDBDatabaseCallbacks>);

    virtual String name() const { return m_name; }
    virtual String version() const { return m_version; }
    virtual PassRefPtr<DOMStringList> objectStoreNames() const;

    virtual PassRefPtr<IDBObjectStoreBackendInterface> createObjectStore(const String& name, const String& keyPath, bool autoIncrement, IDBTransactionBackendInterface*, ExceptionCode&);
    virtual void deleteObjectStore(const String& name, IDBTransactionBackendInterface*, ExceptionCode&);
    virtual void setVersion(const String& version, PassRefPtr<IDBCallbacks>, PassRefPtr<IDBDatabaseCallbacks>, ExceptionCode&);
    virtual PassRefPtr<IDBTransactionBackendInterface> transaction(DOMStringList* objectStoreNames, unsigned short mode, ExceptionCode&);
    virtual void close(PassRefPtr<IDBDatabaseCallbacks>);

    PassRefPtr<IDBObjectStoreBackendInterface> objectStore(const String& name);
    IDBTransactionCoordinator* transactionCoordinator() const { return m_transactionCoordinator.get(); }

private:
    IDBDatabaseBackendImpl(const String& name, IDBBackingStore* database, IDBTransactionCoordinator*, IDBFactoryBackendImpl*, const String& uniqueIdentifier);

    void loadObjectStores();

    static void createObjectStoreInternal(ScriptExecutionContext*, PassRefPtr<IDBDatabaseBackendImpl>, PassRefPtr<IDBObjectStoreBackendImpl>, PassRefPtr<IDBTransactionBackendInterface>);
    static void deleteObjectStoreInternal(ScriptExecutionContext*, PassRefPtr<IDBDatabaseBackendImpl>, PassRefPtr<IDBObjectStoreBackendImpl>, PassRefPtr<IDBTransactionBackendInterface>);
    static void setVersionInternal(ScriptExecutionContext*, PassRefPtr<IDBDatabaseBackendImpl>, const String& version, PassRefPtr<IDBCallbacks>, PassRefPtr<IDBTransactionBackendInterface>);

    // These are used as setVersion transaction abort tasks.
    static void removeObjectStoreFromMap(ScriptExecutionContext*, PassRefPtr<IDBDatabaseBackendImpl>, PassRefPtr<IDBObjectStoreBackendImpl>);
    static void addObjectStoreToMap(ScriptExecutionContext*, PassRefPtr<IDBDatabaseBackendImpl>, PassRefPtr<IDBObjectStoreBackendImpl>);
    static void resetVersion(ScriptExecutionContext*, PassRefPtr<IDBDatabaseBackendImpl>, const String& version);

    RefPtr<IDBBackingStore> m_backingStore;
    int64 m_id;
    String m_name;
    String m_version;

    String m_identifier;
    // This might not need to be a RefPtr since the factory's lifetime is that of the page group, but it's better to be conservitive than sorry.
    RefPtr<IDBFactoryBackendImpl> m_factory;

    typedef HashMap<String, RefPtr<IDBObjectStoreBackendImpl> > ObjectStoreMap;
    ObjectStoreMap m_objectStores;

    RefPtr<IDBTransactionCoordinator> m_transactionCoordinator;

    class PendingSetVersionCall;
    Deque<RefPtr<PendingSetVersionCall> > m_pendingSetVersionCalls;

    typedef ListHashSet<RefPtr<IDBDatabaseCallbacks> > DatabaseCallbacksSet;
    DatabaseCallbacksSet m_databaseCallbacksSet;
};

} // namespace WebCore

#endif

#endif // IDBDatabaseBackendImpl_h
