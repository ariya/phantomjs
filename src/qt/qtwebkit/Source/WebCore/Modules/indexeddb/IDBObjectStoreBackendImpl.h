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

#ifndef IDBObjectStoreBackendImpl_h
#define IDBObjectStoreBackendImpl_h

#include "IDBBackingStore.h"
#include "IDBDatabaseBackendImpl.h"
#include "IDBKeyPath.h"
#include "IDBMetadata.h"
#include <wtf/HashMap.h>
#include <wtf/text/StringHash.h>

#if ENABLE(INDEXED_DATABASE)

namespace WebCore {

class IDBDatabaseBackendImpl;
class IDBTransactionBackendImpl;
struct IDBObjectStoreMetadata;

// FIXME: this namespace is temporary until we move its contents out to their own home.
namespace IDBObjectStoreBackendImpl {

    class IndexWriter {
    public:
        explicit IndexWriter(const IDBIndexMetadata& indexMetadata)
            : m_indexMetadata(indexMetadata)
        { }

        IndexWriter(const IDBIndexMetadata& indexMetadata, const IDBDatabaseBackendInterface::IndexKeys& indexKeys)
            : m_indexMetadata(indexMetadata)
            , m_indexKeys(indexKeys)
        { }

        bool verifyIndexKeys(IDBBackingStore&, IDBBackingStore::Transaction*, int64_t databaseId, int64_t objectStoreId, int64_t indexId, bool& canAddKeys, const IDBKey* primaryKey = 0, String* errorMessage = 0) const WARN_UNUSED_RETURN;

        void writeIndexKeys(const IDBBackingStore::RecordIdentifier&, IDBBackingStore&, IDBBackingStore::Transaction*, int64_t databaseId, int64_t objectStoreId) const;

    private:
        bool addingKeyAllowed(IDBBackingStore&, IDBBackingStore::Transaction*, int64_t databaseId, int64_t objectStoreId, int64_t indexId, const IDBKey* indexKey, const IDBKey* primaryKey, bool& allowed) const WARN_UNUSED_RETURN;

        const IDBIndexMetadata m_indexMetadata;
        IDBDatabaseBackendInterface::IndexKeys m_indexKeys;
    };

    bool makeIndexWriters(PassRefPtr<IDBTransactionBackendImpl>, IDBBackingStore*, int64_t databaseId, const IDBObjectStoreMetadata&, PassRefPtr<IDBKey> primaryKey, bool keyWasGenerated, const Vector<int64_t>& indexIds, const Vector<IDBDatabaseBackendInterface::IndexKeys>&, Vector<OwnPtr<IndexWriter> >* indexWriters, String* errorMessage, bool& completed) WARN_UNUSED_RETURN;

    PassRefPtr<IDBKey> generateKey(PassRefPtr<IDBBackingStore>, PassRefPtr<IDBTransactionBackendImpl>, int64_t databaseId, int64_t objectStoreId);
    bool updateKeyGenerator(PassRefPtr<IDBBackingStore>, PassRefPtr<IDBTransactionBackendImpl>, int64_t databaseId, int64_t objectStoreId, const IDBKey*, bool checkCurrent);
};

} // namespace WebCore

#endif

#endif // IDBObjectStoreBackendImpl_h
