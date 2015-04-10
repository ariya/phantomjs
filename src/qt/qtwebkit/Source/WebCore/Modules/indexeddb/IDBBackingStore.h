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

#include "IDBKey.h"
#include "IDBMetadata.h"
#include "IndexedDB.h"
#include "LevelDBTransaction.h"
#include <wtf/OwnPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/WeakPtr.h>

namespace WebCore {

class LevelDBComparator;
class LevelDBDatabase;
class LevelDBIterator;
class LevelDBTransaction;
class IDBKey;
class IDBKeyRange;
class SecurityOrigin;
class SharedBuffer;

class LevelDBFactory {
public:
    virtual PassOwnPtr<LevelDBDatabase> openLevelDB(const String& fileName, const LevelDBComparator*) = 0;
    virtual bool destroyLevelDB(const String& fileName) = 0;
};

class IDBBackingStore : public RefCounted<IDBBackingStore> {
public:
    class Transaction;

    virtual ~IDBBackingStore();
    static PassRefPtr<IDBBackingStore> open(SecurityOrigin*, const String& pathBase, const String& fileIdentifier);
    static PassRefPtr<IDBBackingStore> open(SecurityOrigin*, const String& pathBase, const String& fileIdentifier, LevelDBFactory*);
    static PassRefPtr<IDBBackingStore> openInMemory(SecurityOrigin*, const String& identifier);
    static PassRefPtr<IDBBackingStore> openInMemory(SecurityOrigin*, const String& identifier, LevelDBFactory*);
    WeakPtr<IDBBackingStore> createWeakPtr() { return m_weakFactory.createWeakPtr(); }

    virtual Vector<String> getDatabaseNames();
    virtual bool getIDBDatabaseMetaData(const String& name, IDBDatabaseMetadata*, bool& success) WARN_UNUSED_RETURN;
    virtual bool createIDBDatabaseMetaData(const String& name, const String& version, int64_t intVersion, int64_t& rowId);
    virtual bool updateIDBDatabaseMetaData(IDBBackingStore::Transaction*, int64_t rowId, const String& version);
    virtual bool updateIDBDatabaseIntVersion(IDBBackingStore::Transaction*, int64_t rowId, int64_t intVersion);
    virtual bool deleteDatabase(const String& name);

    bool getObjectStores(int64_t databaseId, IDBDatabaseMetadata::ObjectStoreMap*) WARN_UNUSED_RETURN;
    virtual bool createObjectStore(IDBBackingStore::Transaction*, int64_t databaseId, int64_t objectStoreId, const String& name, const IDBKeyPath&, bool autoIncrement);
    virtual bool deleteObjectStore(IDBBackingStore::Transaction*, int64_t databaseId, int64_t objectStoreId) WARN_UNUSED_RETURN;

    class RecordIdentifier {
        WTF_MAKE_NONCOPYABLE(RecordIdentifier);
    public:
        RecordIdentifier(const Vector<char>& primaryKey, int64_t version) : m_primaryKey(primaryKey), m_version(version) { ASSERT(!primaryKey.isEmpty()); }
        RecordIdentifier() : m_primaryKey(), m_version(-1) { }

        const Vector<char> primaryKey() const { return m_primaryKey; }
        int64_t version() const { return m_version; }
        void reset(const Vector<char>& primaryKey, int64_t version) { m_primaryKey = primaryKey; m_version = version; }

    private:
        Vector<char> m_primaryKey; // FIXME: Make it more clear that this is the *encoded* version of the key.
        int64_t m_version;
    };

    virtual bool getRecord(IDBBackingStore::Transaction*, int64_t databaseId, int64_t objectStoreId, const IDBKey&, Vector<char>& record) WARN_UNUSED_RETURN;
    virtual bool putRecord(IDBBackingStore::Transaction*, int64_t databaseId, int64_t objectStoreId, const IDBKey&, PassRefPtr<SharedBuffer> value, RecordIdentifier*) WARN_UNUSED_RETURN;
    virtual bool clearObjectStore(IDBBackingStore::Transaction*, int64_t databaseId, int64_t objectStoreId) WARN_UNUSED_RETURN;
    virtual bool deleteRecord(IDBBackingStore::Transaction*, int64_t databaseId, int64_t objectStoreId, const RecordIdentifier&) WARN_UNUSED_RETURN;
    virtual bool getKeyGeneratorCurrentNumber(IDBBackingStore::Transaction*, int64_t databaseId, int64_t objectStoreId, int64_t& currentNumber) WARN_UNUSED_RETURN;
    virtual bool maybeUpdateKeyGeneratorCurrentNumber(IDBBackingStore::Transaction*, int64_t databaseId, int64_t objectStoreId, int64_t newState, bool checkCurrent) WARN_UNUSED_RETURN;
    virtual bool keyExistsInObjectStore(IDBBackingStore::Transaction*, int64_t databaseId, int64_t objectStoreId, const IDBKey&, RecordIdentifier* foundRecordIdentifier, bool& found) WARN_UNUSED_RETURN;

    virtual bool createIndex(IDBBackingStore::Transaction*, int64_t databaseId, int64_t objectStoreId, int64_t indexId, const String& name, const IDBKeyPath&, bool isUnique, bool isMultiEntry) WARN_UNUSED_RETURN;
    virtual bool deleteIndex(IDBBackingStore::Transaction*, int64_t databaseId, int64_t objectStoreId, int64_t indexId) WARN_UNUSED_RETURN;
    virtual bool putIndexDataForRecord(IDBBackingStore::Transaction*, int64_t databaseId, int64_t objectStoreId, int64_t indexId, const IDBKey&, const RecordIdentifier&) WARN_UNUSED_RETURN;
    virtual bool getPrimaryKeyViaIndex(IDBBackingStore::Transaction*, int64_t databaseId, int64_t objectStoreId, int64_t indexId, const IDBKey&, RefPtr<IDBKey>& primaryKey) WARN_UNUSED_RETURN;
    virtual bool keyExistsInIndex(IDBBackingStore::Transaction*, int64_t databaseId, int64_t objectStoreId, int64_t indexId, const IDBKey& indexKey, RefPtr<IDBKey>& foundPrimaryKey, bool& exists) WARN_UNUSED_RETURN;

    class Cursor : public RefCounted<Cursor> {
    public:
        enum IteratorState {
            Ready = 0,
            Seek
        };

        struct CursorOptions {
            int64_t databaseId;
            int64_t objectStoreId;
            int64_t indexId;
            Vector<char> lowKey;
            bool lowOpen;
            Vector<char> highKey;
            bool highOpen;
            bool forward;
            bool unique;
        };

        PassRefPtr<IDBKey> key() const { return m_currentKey; }
        bool continueFunction(const IDBKey* = 0, IteratorState = Seek);
        bool advance(unsigned long);
        bool firstSeek();

        virtual PassRefPtr<Cursor> clone() = 0;
        virtual PassRefPtr<IDBKey> primaryKey() const { return m_currentKey; }
        virtual PassRefPtr<SharedBuffer> value() const = 0;
        virtual const RecordIdentifier& recordIdentifier() const { return m_recordIdentifier; }
        virtual ~Cursor() { }
        virtual bool loadCurrentRow() = 0;

    protected:
        Cursor(LevelDBTransaction* transaction, const CursorOptions& cursorOptions)
            : m_transaction(transaction)
            , m_cursorOptions(cursorOptions)
        {
        }
        explicit Cursor(const IDBBackingStore::Cursor* other);

        virtual Vector<char> encodeKey(const IDBKey&) = 0;

        bool isPastBounds() const;
        bool haveEnteredRange() const;

        LevelDBTransaction* m_transaction;
        const CursorOptions m_cursorOptions;
        OwnPtr<LevelDBIterator> m_iterator;
        RefPtr<IDBKey> m_currentKey;
        IDBBackingStore::RecordIdentifier m_recordIdentifier;
    };

    virtual PassRefPtr<Cursor> openObjectStoreKeyCursor(IDBBackingStore::Transaction*, int64_t databaseId, int64_t objectStoreId, const IDBKeyRange*, IndexedDB::CursorDirection);
    virtual PassRefPtr<Cursor> openObjectStoreCursor(IDBBackingStore::Transaction*, int64_t databaseId, int64_t objectStoreId, const IDBKeyRange*, IndexedDB::CursorDirection);
    virtual PassRefPtr<Cursor> openIndexKeyCursor(IDBBackingStore::Transaction*, int64_t databaseId, int64_t objectStoreId, int64_t indexId, const IDBKeyRange*, IndexedDB::CursorDirection);
    virtual PassRefPtr<Cursor> openIndexCursor(IDBBackingStore::Transaction*, int64_t databaseId, int64_t objectStoreId, int64_t indexId, const IDBKeyRange*, IndexedDB::CursorDirection);

    class Transaction {
    public:
        explicit Transaction(IDBBackingStore*);
        void begin();
        bool commit();
        void rollback();
        void reset() { m_backingStore = 0; m_transaction = 0; }

        static LevelDBTransaction* levelDBTransactionFrom(Transaction* transaction)
        {
            return static_cast<Transaction*>(transaction)->m_transaction.get();
        }

    private:
        IDBBackingStore* m_backingStore;
        RefPtr<LevelDBTransaction> m_transaction;
    };

protected:
    IDBBackingStore(const String& identifier, PassOwnPtr<LevelDBDatabase>, PassOwnPtr<LevelDBComparator>);

    // Should only used for mocking.
    IDBBackingStore();

private:
    static PassRefPtr<IDBBackingStore> create(const String& identifier, PassOwnPtr<LevelDBDatabase>, PassOwnPtr<LevelDBComparator>);

    bool findKeyInIndex(IDBBackingStore::Transaction*, int64_t databaseId, int64_t objectStoreId, int64_t indexId, const IDBKey&, Vector<char>& foundEncodedPrimaryKey, bool& found);
    bool getIndexes(int64_t databaseId, int64_t objectStoreId, IDBObjectStoreMetadata::IndexMap*) WARN_UNUSED_RETURN;

    String m_identifier;

    OwnPtr<LevelDBDatabase> m_db;
    OwnPtr<LevelDBComparator> m_comparator;
    WeakPtrFactory<IDBBackingStore> m_weakFactory;
};

} // namespace WebCore

#endif // ENABLE(INDEXED_DATABASE)

#endif // IDBBackingStore_h
