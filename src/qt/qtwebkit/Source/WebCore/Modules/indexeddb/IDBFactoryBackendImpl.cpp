/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
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

#include "config.h"
#include "IDBFactoryBackendImpl.h"

#include "DOMStringList.h"
#include "IDBBackingStore.h"
#include "IDBDatabaseBackendImpl.h"
#include "IDBDatabaseException.h"
#include "IDBTracing.h"
#include "IDBTransactionCoordinator.h"
#include "SecurityOrigin.h"

#if ENABLE(INDEXED_DATABASE)

namespace WebCore {

template<typename K, typename M>
static void cleanWeakMap(HashMap<K, WeakPtr<M> >& map)
{
    HashMap<K, WeakPtr<M> > other;
    other.swap(map);

    typename HashMap<K, WeakPtr<M> >::const_iterator iter = other.begin();
    while (iter != other.end()) {
        if (iter->value.get())
            map.set(iter->key, iter->value);
        ++iter;
    }
}

static String computeFileIdentifier(SecurityOrigin* securityOrigin)
{
    static const char levelDBFileSuffix[] = "@1";
    return securityOrigin->databaseIdentifier() + levelDBFileSuffix;
}

static String computeUniqueIdentifier(const String& name, SecurityOrigin* securityOrigin)
{
    return computeFileIdentifier(securityOrigin) + name;
}

IDBFactoryBackendImpl::IDBFactoryBackendImpl()
{
}

IDBFactoryBackendImpl::~IDBFactoryBackendImpl()
{
}

void IDBFactoryBackendImpl::removeIDBDatabaseBackend(const String& uniqueIdentifier)
{
    ASSERT(m_databaseBackendMap.contains(uniqueIdentifier));
    m_databaseBackendMap.remove(uniqueIdentifier);
}

void IDBFactoryBackendImpl::getDatabaseNames(PassRefPtr<IDBCallbacks> callbacks, PassRefPtr<SecurityOrigin> securityOrigin, ScriptExecutionContext*, const String& dataDirectory)
{
    IDB_TRACE("IDBFactoryBackendImpl::getDatabaseNames");
    RefPtr<IDBBackingStore> backingStore = openBackingStore(securityOrigin, dataDirectory);
    if (!backingStore) {
        callbacks->onError(IDBDatabaseError::create(IDBDatabaseException::UnknownError, "Internal error opening backing store for indexedDB.webkitGetDatabaseNames."));
        return;
    }

    RefPtr<DOMStringList> databaseNames = DOMStringList::create();

    Vector<String> foundNames = backingStore->getDatabaseNames();
    for (Vector<String>::const_iterator it = foundNames.begin(); it != foundNames.end(); ++it)
        databaseNames->append(*it);

    callbacks->onSuccess(databaseNames.release());
}

void IDBFactoryBackendImpl::deleteDatabase(const String& name, PassRefPtr<IDBCallbacks> callbacks, PassRefPtr<SecurityOrigin> securityOrigin, ScriptExecutionContext*, const String& dataDirectory)
{
    IDB_TRACE("IDBFactoryBackendImpl::deleteDatabase");
    const String uniqueIdentifier = computeUniqueIdentifier(name, securityOrigin.get());

    IDBDatabaseBackendMap::iterator it = m_databaseBackendMap.find(uniqueIdentifier);
    if (it != m_databaseBackendMap.end()) {
        // If there are any connections to the database, directly delete the
        // database.
        it->value->deleteDatabase(callbacks);
        return;
    }

    // FIXME: Everything from now on should be done on another thread.
    RefPtr<IDBBackingStore> backingStore = openBackingStore(securityOrigin, dataDirectory);
    if (!backingStore) {
        callbacks->onError(IDBDatabaseError::create(IDBDatabaseException::UnknownError, "Internal error opening backing store for indexedDB.deleteDatabase."));
        return;
    }

    RefPtr<IDBDatabaseBackendImpl> databaseBackend = IDBDatabaseBackendImpl::create(name, backingStore.get(), this, uniqueIdentifier);
    if (databaseBackend) {
        m_databaseBackendMap.set(uniqueIdentifier, databaseBackend.get());
        databaseBackend->deleteDatabase(callbacks);
        m_databaseBackendMap.remove(uniqueIdentifier);
    } else
        callbacks->onError(IDBDatabaseError::create(IDBDatabaseException::UnknownError, "Internal error creating database backend for indexedDB.deleteDatabase."));
}

PassRefPtr<IDBBackingStore> IDBFactoryBackendImpl::openBackingStore(PassRefPtr<SecurityOrigin> securityOrigin, const String& dataDirectory)
{
    const String fileIdentifier = computeFileIdentifier(securityOrigin.get());
    const bool openInMemory = dataDirectory.isEmpty();

    IDBBackingStoreMap::iterator it2 = m_backingStoreMap.find(fileIdentifier);
    if (it2 != m_backingStoreMap.end() && it2->value.get())
        return it2->value.get();

    RefPtr<IDBBackingStore> backingStore;
    if (openInMemory)
        backingStore = IDBBackingStore::openInMemory(securityOrigin.get(), fileIdentifier);
    else
        backingStore = IDBBackingStore::open(securityOrigin.get(), dataDirectory, fileIdentifier);

    if (backingStore) {
        cleanWeakMap(m_backingStoreMap);
        m_backingStoreMap.set(fileIdentifier, backingStore->createWeakPtr());
        // If an in-memory database, bind lifetime to this factory instance.
        if (openInMemory)
            m_sessionOnlyBackingStores.add(backingStore);

        // All backing stores associated with this factory should be of the same type.
        ASSERT(m_sessionOnlyBackingStores.isEmpty() || openInMemory);

        return backingStore.release();
    }

    return 0;
}

void IDBFactoryBackendImpl::open(const String& name, int64_t version, int64_t transactionId, PassRefPtr<IDBCallbacks> callbacks, PassRefPtr<IDBDatabaseCallbacks> databaseCallbacks, PassRefPtr<SecurityOrigin> prpSecurityOrigin, ScriptExecutionContext*, const String& dataDirectory)
{
    IDB_TRACE("IDBFactoryBackendImpl::open");
    RefPtr<SecurityOrigin> securityOrigin = prpSecurityOrigin;
    const String uniqueIdentifier = computeUniqueIdentifier(name, securityOrigin.get());

    RefPtr<IDBDatabaseBackendImpl> databaseBackend;
    IDBDatabaseBackendMap::iterator it = m_databaseBackendMap.find(uniqueIdentifier);
    if (it == m_databaseBackendMap.end()) {
        RefPtr<IDBBackingStore> backingStore = openBackingStore(securityOrigin, dataDirectory);
        if (!backingStore) {
            callbacks->onError(IDBDatabaseError::create(IDBDatabaseException::UnknownError, "Internal error opening backing store for indexedDB.open."));
            return;
        }

        databaseBackend = IDBDatabaseBackendImpl::create(name, backingStore.get(), this, uniqueIdentifier);
        if (databaseBackend)
            m_databaseBackendMap.set(uniqueIdentifier, databaseBackend.get());
        else {
            callbacks->onError(IDBDatabaseError::create(IDBDatabaseException::UnknownError, "Internal error creating database backend for indexeddb.open."));
            return;
        }
    } else
        databaseBackend = it->value;

    databaseBackend->openConnection(callbacks, databaseCallbacks, transactionId, version);
}

} // namespace WebCore

#endif // ENABLE(INDEXED_DATABASE)
