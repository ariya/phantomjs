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

#ifndef IDBSQLiteBackingStore_h
#define IDBSQLiteBackingStore_h

#if ENABLE(INDEXED_DATABASE)

#include "IDBBackingStore.h"

namespace WebCore {

class IDBSQLiteBackingStore : public IDBBackingStore {
public:
    static PassRefPtr<IDBBackingStore> open(SecurityOrigin*, const String& pathBase, int64_t maximumSize, const String& fileIdentifier, IDBFactoryBackendImpl*);
    virtual ~IDBSQLiteBackingStore();

    virtual bool extractIDBDatabaseMetaData(const String& name, String& foundVersion, int64_t& foundId);
    virtual bool setIDBDatabaseMetaData(const String& name, const String& version, int64_t& rowId, bool invalidRowId);

    virtual void getObjectStores(int64_t databaseId, Vector<int64_t>& foundIds, Vector<String>& foundNames, Vector<String>& foundKeyPaths, Vector<bool>& foundAutoIncrementFlags);
    virtual bool createObjectStore(int64_t databaseId, const String& name, const String& keyPath, bool autoIncrement, int64_t& assignedObjectStoreId);
    virtual void deleteObjectStore(int64_t databaseId, int64_t objectStoreId);
    virtual PassRefPtr<ObjectStoreRecordIdentifier> createInvalidRecordIdentifier();
    virtual String getObjectStoreRecord(int64_t databaseId, int64_t objectStoreId, const IDBKey&);
    virtual bool putObjectStoreRecord(int64_t databaseId, int64_t objectStoreId, const IDBKey&, const String& value, ObjectStoreRecordIdentifier*);
    virtual void clearObjectStore(int64_t databaseId, int64_t objectStoreId);
    virtual void deleteObjectStoreRecord(int64_t databaseId, int64_t objectStoreId, const ObjectStoreRecordIdentifier*);
    virtual double nextAutoIncrementNumber(int64_t databaseId, int64_t objectStoreId);
    virtual bool keyExistsInObjectStore(int64_t databaseId, int64_t objectStoreId, const IDBKey&, ObjectStoreRecordIdentifier* foundRecordIdentifier);

    virtual bool forEachObjectStoreRecord(int64_t databaseId, int64_t objectStoreId, ObjectStoreRecordCallback&);

    virtual void getIndexes(int64_t databaseId, int64_t objectStoreId, Vector<int64_t>& foundIds, Vector<String>& foundNames, Vector<String>& foundKeyPaths, Vector<bool>& foundUniqueFlags);
    virtual bool createIndex(int64_t databaseId, int64_t objectStoreId, const String& name, const String& keyPath, bool isUnique, int64_t& indexId);
    virtual void deleteIndex(int64_t databaseId, int64_t objectStoreId, int64_t indexId);
    virtual bool putIndexDataForRecord(int64_t databaseId, int64_t objectStoreId, int64_t indexId, const IDBKey&, const ObjectStoreRecordIdentifier*);
    virtual bool deleteIndexDataForRecord(int64_t databaseId, int64_t objectStoreId, int64_t indexId, const ObjectStoreRecordIdentifier*);
    virtual String getObjectViaIndex(int64_t databaseId, int64_t objectStoreId, int64_t indexId, const IDBKey&);
    virtual PassRefPtr<IDBKey> getPrimaryKeyViaIndex(int64_t databaseId, int64_t objectStoreId, int64_t indexId, const IDBKey&);
    virtual bool keyExistsInIndex(int64_t databaseId, int64_t objectStoreId, int64_t indexId, const IDBKey&);

    virtual PassRefPtr<Cursor> openObjectStoreCursor(int64_t databaseId, int64_t objectStoreId, const IDBKeyRange*, IDBCursor::Direction);
    virtual PassRefPtr<Cursor> openIndexKeyCursor(int64_t databaseId, int64_t objectStoreId, int64_t indexId, const IDBKeyRange*, IDBCursor::Direction);
    virtual PassRefPtr<Cursor> openIndexCursor(int64_t databaseId, int64_t objectStoreId, int64_t indexId, const IDBKeyRange*, IDBCursor::Direction);

    virtual PassRefPtr<Transaction> createTransaction();

private:
    IDBSQLiteBackingStore(String identifier, IDBFactoryBackendImpl*);

    SQLiteDatabase m_db;
    String m_identifier;
    RefPtr<IDBFactoryBackendImpl> m_factory;
};

} // namespace WebCore

#endif

#endif // IDBSQLiteBackingStore_h
