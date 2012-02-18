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
#include "IDBDatabaseBackendImpl.h"
#include "IDBDatabaseException.h"
#include "IDBLevelDBBackingStore.h"
#include "IDBSQLiteBackingStore.h"
#include "IDBTransactionCoordinator.h"
#include "SecurityOrigin.h"
#include <wtf/Threading.h>
#include <wtf/UnusedParam.h>

#if ENABLE(INDEXED_DATABASE)

namespace WebCore {

IDBFactoryBackendImpl::IDBFactoryBackendImpl()
    : m_transactionCoordinator(IDBTransactionCoordinator::create())
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

void IDBFactoryBackendImpl::addIDBBackingStore(const String& uniqueIdentifier, IDBBackingStore* backingStore)
{
    ASSERT(!m_backingStoreMap.contains(uniqueIdentifier));
    m_backingStoreMap.set(uniqueIdentifier, backingStore);
}

void IDBFactoryBackendImpl::removeIDBBackingStore(const String& uniqueIdentifier)
{
    ASSERT(m_backingStoreMap.contains(uniqueIdentifier));
    m_backingStoreMap.remove(uniqueIdentifier);
}

void IDBFactoryBackendImpl::open(const String& name, PassRefPtr<IDBCallbacks> callbacks, PassRefPtr<SecurityOrigin> securityOrigin, Frame*, const String& dataDir, int64_t maximumSize, BackingStoreType backingStoreType)
{
    String fileIdentifier = securityOrigin->databaseIdentifier();
    String uniqueIdentifier = fileIdentifier + "@" + name;
    IDBDatabaseBackendMap::iterator it = m_databaseBackendMap.find(uniqueIdentifier);
    if (it != m_databaseBackendMap.end()) {
        callbacks->onSuccess(it->second);
        return;
    }

    // FIXME: Everything from now on should be done on another thread.

    RefPtr<IDBBackingStore> backingStore;
    IDBBackingStoreMap::iterator it2 = m_backingStoreMap.find(fileIdentifier);
    if (it2 != m_backingStoreMap.end())
        backingStore = it2->second;
    else {
        if (backingStoreType == DefaultBackingStore)
            backingStore = IDBSQLiteBackingStore::open(securityOrigin.get(), dataDir, maximumSize, fileIdentifier, this);
#if ENABLE(LEVELDB)
        else if (backingStoreType == LevelDBBackingStore)
            backingStore = IDBLevelDBBackingStore::open(securityOrigin.get(), dataDir, maximumSize, fileIdentifier, this);
#endif
        if (!backingStore) {
            callbacks->onError(IDBDatabaseError::create(IDBDatabaseException::UNKNOWN_ERR, "Internal error."));
            return;
        }
    }

    RefPtr<IDBDatabaseBackendImpl> databaseBackend = IDBDatabaseBackendImpl::create(name, backingStore.get(), m_transactionCoordinator.get(), this, uniqueIdentifier);
    callbacks->onSuccess(databaseBackend.get());
    m_databaseBackendMap.set(uniqueIdentifier, databaseBackend.get());
}

} // namespace WebCore

#endif // ENABLE(INDEXED_DATABASE)
