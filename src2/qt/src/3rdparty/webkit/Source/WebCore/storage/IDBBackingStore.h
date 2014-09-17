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

#ifndef IDBBackingStore_h
#define IDBBackingStore_h

#if ENABLE(INDEXED_DATABASE)

#include "IDBCursor.h"
#include "SQLiteDatabase.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>
#include <wtf/Vector.h>

namespace WebCore {

class IDBFactoryBackendImpl;
class IDBKey;
class IDBKeyRange;
class SecurityOrigin;

class IDBBackingStore : public RefCounted<IDBBackingStore> {
public:
    virtual ~IDBBackingStore() {};

    virtual bool extractIDBDatabaseMetaData(const String& name, String& foundVersion, int64_t& foundId) = 0;
    virtual bool setIDBDatabaseMetaData(const String& name, const String& version, int64_t& rowId, bool invalidRowId) = 0;

    virtual void getObjectStores(int64_t databaseId, Vector<int64_t>& foundIds, Vector<String>& foundNames, Vector<String>& foundKeyPaths, Vector<bool>& foundAutoIncrementFlags) = 0;
    virtual bool createObjectStore(int64_t databaseId, const String& name, const String& keyPath, bool autoIncrement, int64_t& assignedObjectStoreId) = 0;
    virtual void deleteObjectStore(int64_t databaseId, int64_t objectStoreId) = 0;

    class ObjectStoreRecordIdentifier : public RefCounted<ObjectStoreRecordIdentifier> {
    public:
        virtual bool isValid() const = 0;
        virtual ~ObjectStoreRecordIdentifier() {}
    };
    virtual PassRefPtr<ObjectStoreRecordIdentifier> createInvalidRecordIdentifier() = 0;

    virtual String getObjectStoreRecord(int64_t databaseId, int64_t objectStoreId, const IDBKey&) = 0;
    virtual bool putObjectStoreRecord(int64_t databaseId, int64_t objectStoreId, const IDBKey&, const String& value, ObjectStoreRecordIdentifier*) = 0;
    virtual void clearObjectStore(int64_t databaseId, int64_t objectStoreId) = 0;
    virtual void deleteObjectStoreRecord(int64_t databaseId, int64_t objectStoreId, const ObjectStoreRecordIdentifier*) = 0;
    virtual double nextAutoIncrementNumber(int64_t databaseId, int64_t objectStoreId) = 0;
    virtual bool keyExistsInObjectStore(int64_t databaseId, int64_t objectStoreId, const IDBKey&, ObjectStoreRecordIdentifier* foundRecordIdentifier) = 0;

    class ObjectStoreRecordCallback {
    public:
         virtual bool callback(const ObjectStoreRecordIdentifier*, const String& value) = 0;
         virtual ~ObjectStoreRecordCallback() {};
    };
    virtual bool forEachObjectStoreRecord(int64_t databaseId, int64_t objectStoreId, ObjectStoreRecordCallback&) = 0;

    virtual void getIndexes(int64_t databaseId, int64_t objectStoreId, Vector<int64_t>& foundIds, Vector<String>& foundNames, Vector<String>& foundKeyPaths, Vector<bool>& foundUniqueFlags) = 0;
    virtual bool createIndex(int64_t databaseId, int64_t objectStoreId, const String& name, const String& keyPath, bool isUnique, int64_t& indexId) = 0;
    virtual void deleteIndex(int64_t databaseId, int64_t objectStoreId, int64_t indexId) = 0;
    virtual bool putIndexDataForRecord(int64_t databaseId, int64_t objectStoreId, int64_t indexId, const IDBKey&, const ObjectStoreRecordIdentifier*) = 0;
    virtual bool deleteIndexDataForRecord(int64_t databaseId, int64_t objectStoreId, int64_t indexId, const ObjectStoreRecordIdentifier*) = 0;
    virtual String getObjectViaIndex(int64_t databaseId, int64_t objectStoreId, int64_t indexId, const IDBKey&) = 0;
    virtual PassRefPtr<IDBKey> getPrimaryKeyViaIndex(int64_t databaseId, int64_t objectStoreId, int64_t indexId, const IDBKey&) = 0;
    virtual bool keyExistsInIndex(int64_t databaseid, int64_t objectStoreId, int64_t indexId, const IDBKey&) = 0;

    class Cursor : public RefCounted<Cursor> {
    public:
        virtual bool continueFunction(const IDBKey* = 0) = 0;
        virtual PassRefPtr<IDBKey> key() = 0;
        virtual PassRefPtr<IDBKey> primaryKey() = 0;
        virtual String value() = 0;
        virtual PassRefPtr<ObjectStoreRecordIdentifier> objectStoreRecordIdentifier() = 0;
        virtual int64_t indexDataId() = 0;
        virtual ~Cursor() {};
    };

    virtual PassRefPtr<Cursor> openObjectStoreCursor(int64_t databaseId, int64_t objectStoreId, const IDBKeyRange*, IDBCursor::Direction) = 0;
    virtual PassRefPtr<Cursor> openIndexKeyCursor(int64_t databaseId, int64_t objectStoreId, int64_t indexId, const IDBKeyRange*, IDBCursor::Direction) = 0;
    virtual PassRefPtr<Cursor> openIndexCursor(int64_t databaseId, int64_t objectStoreId, int64_t indexId, const IDBKeyRange*, IDBCursor::Direction) = 0;

    class Transaction : public RefCounted<Transaction> {
    public:
        virtual void begin() = 0;
        virtual void commit() = 0;
        virtual void rollback() = 0;
    };
    virtual PassRefPtr<Transaction> createTransaction() = 0;
};

} // namespace WebCore

#endif

#endif // IDBBackingStore_h
