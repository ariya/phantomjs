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
#include "IDBObjectStoreBackendImpl.h"

#if ENABLE(INDEXED_DATABASE)

#include "IDBBackingStore.h"
#include "IDBBindingUtilities.h"
#include "IDBCallbacks.h"
#include "IDBCursorBackendImpl.h"
#include "IDBDatabaseBackendImpl.h"
#include "IDBDatabaseException.h"
#include "IDBKey.h"
#include "IDBKeyPath.h"
#include "IDBKeyRange.h"
#include "IDBTracing.h"
#include "IDBTransactionBackendImpl.h"
#include <wtf/MathExtras.h>

namespace WebCore {

bool IDBObjectStoreBackendImpl::IndexWriter::verifyIndexKeys(IDBBackingStore& backingStore, IDBBackingStore::Transaction* transaction, int64_t databaseId, int64_t objectStoreId, int64_t indexId, bool& canAddKeys, const IDBKey* primaryKey, String* errorMessage) const
{
    canAddKeys = false;
    for (size_t i = 0; i < m_indexKeys.size(); ++i) {
        bool ok = addingKeyAllowed(backingStore, transaction, databaseId, objectStoreId, indexId, (m_indexKeys)[i].get(), primaryKey, canAddKeys);
        if (!ok)
            return false;
        if (!canAddKeys) {
            if (errorMessage)
                *errorMessage = String::format("Unable to add key to index '%s': at least one key does not satisfy the uniqueness requirements.", m_indexMetadata.name.utf8().data());
            return true;
        }
    }
    canAddKeys = true;
    return true;
}

void IDBObjectStoreBackendImpl::IndexWriter::writeIndexKeys(const IDBBackingStore::RecordIdentifier& recordIdentifier, IDBBackingStore& backingStore, IDBBackingStore::Transaction* transaction, int64_t databaseId, int64_t objectStoreId) const
{
    int64_t indexId = m_indexMetadata.id;
    for (size_t i = 0; i < m_indexKeys.size(); ++i) {
        bool ok = backingStore.putIndexDataForRecord(transaction, databaseId, objectStoreId, indexId, *(m_indexKeys)[i].get(), recordIdentifier);
        // This should have already been verified as a valid write during verifyIndexKeys.
        ASSERT_UNUSED(ok, ok);
    }
}

bool IDBObjectStoreBackendImpl::IndexWriter::addingKeyAllowed(IDBBackingStore& backingStore, IDBBackingStore::Transaction* transaction, int64_t databaseId, int64_t objectStoreId, int64_t indexId, const IDBKey* indexKey, const IDBKey* primaryKey, bool& allowed) const
{
    allowed = false;
    if (!m_indexMetadata.unique) {
        allowed = true;
        return true;
    }

    RefPtr<IDBKey> foundPrimaryKey;
    bool found = false;
    bool ok = backingStore.keyExistsInIndex(transaction, databaseId, objectStoreId, indexId, *indexKey, foundPrimaryKey, found);
    if (!ok)
        return false;
    if (!found || (primaryKey && foundPrimaryKey->isEqual(primaryKey)))
        allowed = true;
    return true;
}

bool IDBObjectStoreBackendImpl::makeIndexWriters(PassRefPtr<IDBTransactionBackendImpl> transaction, IDBBackingStore* backingStore, int64_t databaseId, const IDBObjectStoreMetadata& objectStore, PassRefPtr<IDBKey> primaryKey, bool keyWasGenerated, const Vector<int64_t>& indexIds, const Vector<IDBDatabaseBackendInterface::IndexKeys>& indexKeys, Vector<OwnPtr<IndexWriter> >* indexWriters, String* errorMessage, bool& completed)
{
    ASSERT(indexIds.size() == indexKeys.size());
    completed = false;

    HashMap<int64_t, IDBDatabaseBackendInterface::IndexKeys> indexKeyMap;
    for (size_t i = 0; i < indexIds.size(); ++i)
        indexKeyMap.add(indexIds[i], indexKeys[i]);

    for (IDBObjectStoreMetadata::IndexMap::const_iterator it = objectStore.indexes.begin(); it != objectStore.indexes.end(); ++it) {

        const IDBIndexMetadata& index = it->value;

        IDBDatabaseBackendInterface::IndexKeys keys = indexKeyMap.get(it->key);
        // If the objectStore is using autoIncrement, then any indexes with an identical keyPath need to also use the primary (generated) key as a key.
        if (keyWasGenerated && (index.keyPath == objectStore.keyPath))
            keys.append(primaryKey);

        OwnPtr<IndexWriter> indexWriter(adoptPtr(new IndexWriter(index, keys)));
        bool canAddKeys = false;
        bool backingStoreSuccess = indexWriter->verifyIndexKeys(*backingStore, transaction->backingStoreTransaction(), databaseId, objectStore.id, index.id, canAddKeys, primaryKey.get(), errorMessage);
        if (!backingStoreSuccess)
            return false;
        if (!canAddKeys)
            return true;

        indexWriters->append(indexWriter.release());
    }

    completed = true;
    return true;
}

PassRefPtr<IDBKey> IDBObjectStoreBackendImpl::generateKey(PassRefPtr<IDBBackingStore> backingStore, PassRefPtr<IDBTransactionBackendImpl> transaction, int64_t databaseId, int64_t objectStoreId)
{
    const int64_t maxGeneratorValue = 9007199254740992LL; // Maximum integer storable as ECMAScript number.
    int64_t currentNumber;
    bool ok = backingStore->getKeyGeneratorCurrentNumber(transaction->backingStoreTransaction(), databaseId, objectStoreId, currentNumber);
    if (!ok) {
        LOG_ERROR("Failed to getKeyGeneratorCurrentNumber");
        return IDBKey::createInvalid();
    }
    if (currentNumber < 0 || currentNumber > maxGeneratorValue)
        return IDBKey::createInvalid();

    return IDBKey::createNumber(currentNumber);
}

bool IDBObjectStoreBackendImpl::updateKeyGenerator(PassRefPtr<IDBBackingStore> backingStore, PassRefPtr<IDBTransactionBackendImpl> transaction, int64_t databaseId, int64_t objectStoreId, const IDBKey* key, bool checkCurrent)
{
    ASSERT(key && key->type() == IDBKey::NumberType);
    return backingStore->maybeUpdateKeyGeneratorCurrentNumber(transaction->backingStoreTransaction(), databaseId, objectStoreId, static_cast<int64_t>(floor(key->number())) + 1, checkCurrent);
}

} // namespace WebCore

#endif
