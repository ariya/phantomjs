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
#include "IDBLevelDBBackingStore.h"

#if ENABLE(INDEXED_DATABASE)
#if ENABLE(LEVELDB)

#include "Assertions.h"
#include "FileSystem.h"
#include "IDBFactoryBackendImpl.h"
#include "IDBKeyRange.h"
#include "IDBLevelDBCoding.h"
#include "LevelDBComparator.h"
#include "LevelDBDatabase.h"
#include "LevelDBIterator.h"
#include "LevelDBSlice.h"
#include "SecurityOrigin.h"

#ifndef INT64_MAX
// FIXME: We shouldn't need to rely on these macros.
#define INT64_MAX 0x7fffffffffffffffLL
#endif

namespace WebCore {

using namespace IDBLevelDBCoding;

static bool getInt(LevelDBDatabase* db, const Vector<char>& key, int64_t& foundInt)
{
    Vector<char> result;
    if (!db->get(key, result))
        return false;

    foundInt = decodeInt(result.begin(), result.end());
    return true;
}

static bool putInt(LevelDBDatabase* db, const Vector<char>& key, int64_t value)
{
    return db->put(key, encodeInt(value));
}

static bool getString(LevelDBDatabase* db, const Vector<char>& key, String& foundString)
{
    Vector<char> result;
    if (!db->get(key, result))
        return false;

    foundString = decodeString(result.begin(), result.end());
    return true;
}

static bool putString(LevelDBDatabase* db, const Vector<char> key, const String& value)
{
    if (!db->put(key, encodeString(value)))
        return false;
    return true;
}

static int compareKeys(const LevelDBSlice& a, const LevelDBSlice& b)
{
    return compare(a, b);
}

static int compareIndexKeys(const LevelDBSlice& a, const LevelDBSlice& b)
{
    return compare(a, b, true);
}

class Comparator : public LevelDBComparator {
public:
    virtual int compare(const LevelDBSlice& a, const LevelDBSlice& b) const { return IDBLevelDBCoding::compare(a, b); }
    virtual const char* name() const { return "idb_cmp1"; }
};

static bool setUpMetadata(LevelDBDatabase* db)
{
    const Vector<char> metaDataKey = SchemaVersionKey::encode();

    int64_t schemaVersion = 0;
    if (!getInt(db, metaDataKey, schemaVersion)) {
        schemaVersion = 0;
        if (!putInt(db, metaDataKey, schemaVersion))
            return false;
    }

    // FIXME: Eventually, we'll need to be able to transition between schemas.
    if (schemaVersion)
        return false; // Don't know what to do with this version.

    return true;
}

IDBLevelDBBackingStore::IDBLevelDBBackingStore(String identifier, IDBFactoryBackendImpl* factory, PassOwnPtr<LevelDBDatabase> db)
    : m_identifier(identifier)
    , m_factory(factory)
    , m_db(db)
{
    m_factory->addIDBBackingStore(identifier, this);
}

IDBLevelDBBackingStore::~IDBLevelDBBackingStore()
{
    m_factory->removeIDBBackingStore(m_identifier);
}

PassRefPtr<IDBBackingStore> IDBLevelDBBackingStore::open(SecurityOrigin* securityOrigin, const String& pathBaseArg, int64_t maximumSize, const String& fileIdentifier, IDBFactoryBackendImpl* factory)
{
    String pathBase = pathBaseArg;

    if (pathBase.isEmpty()) {
        ASSERT_NOT_REACHED(); // FIXME: We need to handle this case for incognito and DumpRenderTree.
        return PassRefPtr<IDBBackingStore>();
    }

    if (!makeAllDirectories(pathBase)) {
        LOG_ERROR("Unable to create IndexedDB database path %s", pathBase.utf8().data());
        return PassRefPtr<IDBBackingStore>();
    }
    // FIXME: We should eventually use the same LevelDB database for all origins.
    String path = pathByAppendingComponent(pathBase, securityOrigin->databaseIdentifier() + ".indexeddb.leveldb");

    OwnPtr<LevelDBComparator> comparator = adoptPtr(new Comparator());
    OwnPtr<LevelDBDatabase> db = LevelDBDatabase::open(path, comparator.get());
    if (!db)
        return PassRefPtr<IDBBackingStore>();

    // FIXME: Handle comparator name changes.

    RefPtr<IDBLevelDBBackingStore> backingStore(adoptRef(new IDBLevelDBBackingStore(fileIdentifier, factory, db.release())));
    backingStore->m_comparator = comparator.release();

    if (!setUpMetadata(backingStore->m_db.get()))
        return PassRefPtr<IDBBackingStore>();

    return backingStore.release();
}

bool IDBLevelDBBackingStore::extractIDBDatabaseMetaData(const String& name, String& foundVersion, int64_t& foundId)
{
    const Vector<char> key = DatabaseNameKey::encode(m_identifier, name);

    bool ok = getInt(m_db.get(), key, foundId);
    if (!ok)
        return false;

    ok = getString(m_db.get(), DatabaseMetaDataKey::encode(foundId, DatabaseMetaDataKey::kUserVersion), foundVersion);
    if (!ok)
        return false;

    return true;
}

static int64_t getNewDatabaseId(LevelDBDatabase* db)
{
    const Vector<char> freeListStartKey = DatabaseFreeListKey::encode(0);
    const Vector<char> freeListStopKey = DatabaseFreeListKey::encode(INT64_MAX);

    OwnPtr<LevelDBIterator> it = db->createIterator();
    for (it->seek(freeListStartKey); it->isValid() && compareKeys(it->key(), freeListStopKey) < 0; it->next()) {
        const char *p = it->key().begin();
        const char *limit = it->key().end();

        DatabaseFreeListKey freeListKey;
        p = DatabaseFreeListKey::decode(p, limit, &freeListKey);
        ASSERT(p);

        bool ok = db->remove(it->key());
        ASSERT_UNUSED(ok, ok);

        return freeListKey.databaseId();
    }

    // If we got here, there was no free-list.
    int64_t maxDatabaseId = -1;
    if (!getInt(db, MaxDatabaseIdKey::encode(), maxDatabaseId))
        maxDatabaseId = 0;

    ASSERT(maxDatabaseId >= 0);

    int64_t databaseId = maxDatabaseId + 1;
    bool ok = putInt(db, MaxDatabaseIdKey::encode(), databaseId);
    ASSERT_UNUSED(ok, ok);

    return databaseId;

}

bool IDBLevelDBBackingStore::setIDBDatabaseMetaData(const String& name, const String& version, int64_t& rowId, bool invalidRowId)
{
    if (invalidRowId) {
        rowId = getNewDatabaseId(m_db.get());

        const Vector<char> key = DatabaseNameKey::encode(m_identifier, name);
        if (!putInt(m_db.get(), key, rowId))
            return false;
    }

    if (!putString(m_db.get(), DatabaseMetaDataKey::encode(rowId, DatabaseMetaDataKey::kUserVersion), version))
        return false;

    return true;
}

void IDBLevelDBBackingStore::getObjectStores(int64_t databaseId, Vector<int64_t>& foundIds, Vector<String>& foundNames, Vector<String>& foundKeyPaths, Vector<bool>& foundAutoIncrementFlags)
{
    const Vector<char> startKey = ObjectStoreMetaDataKey::encode(databaseId, 1, 0);
    const Vector<char> stopKey = ObjectStoreMetaDataKey::encode(databaseId, INT64_MAX, 0);

    OwnPtr<LevelDBIterator> it = m_db->createIterator();
    for (it->seek(startKey); it->isValid() && compareKeys(it->key(), stopKey) < 0; it->next()) {
        const char *p = it->key().begin();
        const char *limit = it->key().end();

        ObjectStoreMetaDataKey metaDataKey;
        p = ObjectStoreMetaDataKey::decode(p, limit, &metaDataKey);
        ASSERT(p);

        int64_t objectStoreId = metaDataKey.objectStoreId();

        String objectStoreName = decodeString(it->value().begin(), it->value().end());

        it->next();
        if (!it->isValid()) {
            LOG_ERROR("Internal Indexed DB error.");
            return;
        }
        String keyPath = decodeString(it->value().begin(), it->value().end());

        it->next();
        if (!it->isValid()) {
            LOG_ERROR("Internal Indexed DB error.");
            return;
        }
        bool autoIncrement = *it->value().begin();

        it->next(); // Is evicatble.
        if (!it->isValid()) {
            LOG_ERROR("Internal Indexed DB error.");
            return;
        }

        it->next(); // Last version.
        if (!it->isValid()) {
            LOG_ERROR("Internal Indexed DB error.");
            return;
        }

        it->next(); // Maxium index id allocated.
        if (!it->isValid()) {
            LOG_ERROR("Internal Indexed DB error.");
            return;
        }

        foundIds.append(objectStoreId);
        foundNames.append(objectStoreName);
        foundKeyPaths.append(keyPath);
        foundAutoIncrementFlags.append(autoIncrement);
    }
}

static int64_t getNewObjectStoreId(LevelDBDatabase* db, int64_t databaseId)
{
    const Vector<char> freeListStartKey = ObjectStoreFreeListKey::encode(databaseId, 0);
    const Vector<char> freeListStopKey = ObjectStoreFreeListKey::encode(databaseId, INT64_MAX);

    OwnPtr<LevelDBIterator> it = db->createIterator();
    for (it->seek(freeListStartKey); it->isValid() && compareKeys(it->key(), freeListStopKey) < 0; it->next()) {
        const char* p = it->key().begin();
        const char* limit = it->key().end();

        ObjectStoreFreeListKey freeListKey;
        p = ObjectStoreFreeListKey::decode(p, limit, &freeListKey);
        ASSERT(p);

        bool ok = db->remove(it->key());
        ASSERT_UNUSED(ok, ok);

        return freeListKey.objectStoreId();
    }

    int64_t maxObjectStoreId = -1;
    const Vector<char> maxObjectStoreIdKey = DatabaseMetaDataKey::encode(databaseId, DatabaseMetaDataKey::kMaxObjectStoreId);
    if (!getInt(db, maxObjectStoreIdKey, maxObjectStoreId))
        maxObjectStoreId = 0;

    ASSERT(maxObjectStoreId >= 0);

    int64_t objectStoreId = maxObjectStoreId + 1;
    bool ok = putInt(db, maxObjectStoreIdKey, objectStoreId);
    ASSERT_UNUSED(ok, ok);

    return objectStoreId;
}

bool IDBLevelDBBackingStore::createObjectStore(int64_t databaseId, const String& name, const String& keyPath, bool autoIncrement, int64_t& assignedObjectStoreId)
{
    int64_t objectStoreId = getNewObjectStoreId(m_db.get(), databaseId);

    const Vector<char> nameKey = ObjectStoreMetaDataKey::encode(databaseId, objectStoreId, 0);
    const Vector<char> keyPathKey = ObjectStoreMetaDataKey::encode(databaseId, objectStoreId, 1);
    const Vector<char> autoIncrementKey = ObjectStoreMetaDataKey::encode(databaseId, objectStoreId, 2);
    const Vector<char> evictableKey = ObjectStoreMetaDataKey::encode(databaseId, objectStoreId, 3);
    const Vector<char> lastVersionKey = ObjectStoreMetaDataKey::encode(databaseId, objectStoreId, 4);
    const Vector<char> maxIndexIdKey = ObjectStoreMetaDataKey::encode(databaseId, objectStoreId, 5);
    const Vector<char> namesKey = ObjectStoreNamesKey::encode(databaseId, name);

    bool ok = putString(m_db.get(), nameKey, name);
    if (!ok) {
        LOG_ERROR("Internal Indexed DB error.");
        return false;
    }

    ok = putString(m_db.get(), keyPathKey, keyPath);
    if (!ok) {
        LOG_ERROR("Internal Indexed DB error.");
        return false;
    }

    ok = putInt(m_db.get(), autoIncrementKey, autoIncrement);
    if (!ok) {
        LOG_ERROR("Internal Indexed DB error.");
        return false;
    }

    ok = putInt(m_db.get(), evictableKey, false);
    if (!ok) {
        LOG_ERROR("Internal Indexed DB error.");
        return false;
    }

    ok = putInt(m_db.get(), lastVersionKey, 1);
    if (!ok) {
        LOG_ERROR("Internal Indexed DB error.");
        return false;
    }

    ok = putInt(m_db.get(), maxIndexIdKey, kMinimumIndexId);
    if (!ok) {
        LOG_ERROR("Internal Indexed DB error.");
        return false;
    }

    ok = putInt(m_db.get(), namesKey, objectStoreId);
    if (!ok) {
        LOG_ERROR("Internal Indexed DB error.");
        return false;
    }

    assignedObjectStoreId = objectStoreId;

    return true;
}

static bool deleteRange(LevelDBDatabase* db, const Vector<char>& begin, const Vector<char>& end)
{
    // FIXME: LevelDB may be able to provide a bulk operation that we can do first.
    OwnPtr<LevelDBIterator> it = db->createIterator();
    for (it->seek(begin); it->isValid() && compareKeys(it->key(), end) < 0; it->next()) {
        if (!db->remove(it->key()))
            return false;
    }

    return true;
}

void IDBLevelDBBackingStore::deleteObjectStore(int64_t databaseId, int64_t objectStoreId)
{
    String objectStoreName;
    getString(m_db.get(), ObjectStoreMetaDataKey::encode(databaseId, objectStoreId, 0), objectStoreName);

    if (!deleteRange(m_db.get(), ObjectStoreMetaDataKey::encode(databaseId, objectStoreId, 0), ObjectStoreMetaDataKey::encode(databaseId, objectStoreId, 6)))
        return; // FIXME: Report error.

    putString(m_db.get(), ObjectStoreFreeListKey::encode(databaseId, objectStoreId), "");
    m_db->remove(ObjectStoreNamesKey::encode(databaseId, objectStoreName));

    if (!deleteRange(m_db.get(), IndexFreeListKey::encode(databaseId, objectStoreId, 0), IndexFreeListKey::encode(databaseId, objectStoreId, INT64_MAX)))
        return; // FIXME: Report error.
    if (!deleteRange(m_db.get(), IndexMetaDataKey::encode(databaseId, objectStoreId, 0, 0), IndexMetaDataKey::encode(databaseId, objectStoreId, INT64_MAX, 0)))
        return; // FIXME: Report error.

    clearObjectStore(databaseId, objectStoreId);
}

String IDBLevelDBBackingStore::getObjectStoreRecord(int64_t databaseId, int64_t objectStoreId, const IDBKey& key)
{
    const Vector<char> leveldbKey = ObjectStoreDataKey::encode(databaseId, objectStoreId, key);
    Vector<char> data;

    if (!m_db->get(leveldbKey, data))
        return String();

    int64_t version;
    const char* p = decodeVarInt(data.begin(), data.end(), version);
    if (!p)
        return String();
    (void) version;

    return decodeString(p, data.end());
}

namespace {
class LevelDBRecordIdentifier : public IDBBackingStore::ObjectStoreRecordIdentifier {
public:
    static PassRefPtr<LevelDBRecordIdentifier> create(const Vector<char>& primaryKey, int64_t version) { return adoptRef(new LevelDBRecordIdentifier(primaryKey, version)); }
    static PassRefPtr<LevelDBRecordIdentifier> create() { return adoptRef(new LevelDBRecordIdentifier()); }

    virtual bool isValid() const { return m_primaryKey.isEmpty(); }
    Vector<char> primaryKey() const { return m_primaryKey; }
    void setPrimaryKey(const Vector<char>& primaryKey) { m_primaryKey = primaryKey; }
    int64_t version() const { return m_version; }
    void setVersion(int64_t version) { m_version = version; }

private:
    LevelDBRecordIdentifier(const Vector<char>& primaryKey, int64_t version) : m_primaryKey(primaryKey), m_version(version) { ASSERT(!primaryKey.isEmpty()); }
    LevelDBRecordIdentifier() : m_primaryKey(), m_version(-1) {}

    Vector<char> m_primaryKey; // FIXME: Make it more clear that this is the *encoded* version of the key.
    int64_t m_version;
};
}

static int64_t getNewVersionNumber(LevelDBDatabase* db, int64_t databaseId, int64_t objectStoreId)
{
    const Vector<char> lastVersionKey = ObjectStoreMetaDataKey::encode(databaseId, objectStoreId, 4);

    int64_t lastVersion = -1;
    if (!getInt(db, lastVersionKey, lastVersion))
        lastVersion = 0;

    ASSERT(lastVersion >= 0);

    int64_t version = lastVersion + 1;
    bool ok = putInt(db, lastVersionKey, version);
    ASSERT_UNUSED(ok, ok);

    ASSERT(version > lastVersion); // FIXME: Think about how we want to handle the overflow scenario.

    return version;
}

bool IDBLevelDBBackingStore::putObjectStoreRecord(int64_t databaseId, int64_t objectStoreId, const IDBKey& key, const String& value, ObjectStoreRecordIdentifier* recordIdentifier)
{
    int64_t version = getNewVersionNumber(m_db.get(), databaseId, objectStoreId);
    const Vector<char> objectStoredataKey = ObjectStoreDataKey::encode(databaseId, objectStoreId, key);

    Vector<char> v;
    v.append(encodeVarInt(version));
    v.append(encodeString(value));

    if (!m_db->put(objectStoredataKey, v))
        return false;

    const Vector<char> existsEntryKey = ExistsEntryKey::encode(databaseId, objectStoreId, key);
    if (!m_db->put(existsEntryKey, encodeInt(version)))
        return false;

    LevelDBRecordIdentifier* levelDBRecordIdentifier = static_cast<LevelDBRecordIdentifier*>(recordIdentifier);
    levelDBRecordIdentifier->setPrimaryKey(encodeIDBKey(key));
    levelDBRecordIdentifier->setVersion(version);
    return true;
}

void IDBLevelDBBackingStore::clearObjectStore(int64_t databaseId, int64_t objectStoreId)
{
    const Vector<char> startKey = KeyPrefix(databaseId, objectStoreId, 0).encode();
    const Vector<char> stopKey = KeyPrefix(databaseId, objectStoreId + 1, 0).encode();

    deleteRange(m_db.get(), startKey, stopKey);
}

PassRefPtr<IDBBackingStore::ObjectStoreRecordIdentifier> IDBLevelDBBackingStore::createInvalidRecordIdentifier()
{
    return LevelDBRecordIdentifier::create();
}

void IDBLevelDBBackingStore::deleteObjectStoreRecord(int64_t databaseId, int64_t objectStoreId, const ObjectStoreRecordIdentifier* recordIdentifier)
{
    const LevelDBRecordIdentifier* levelDBRecordIdentifier = static_cast<const LevelDBRecordIdentifier*>(recordIdentifier);
    const Vector<char> key = ObjectStoreDataKey::encode(databaseId, objectStoreId, levelDBRecordIdentifier->primaryKey());
    m_db->remove(key);
}

double IDBLevelDBBackingStore::nextAutoIncrementNumber(int64_t databaseId, int64_t objectStoreId)
{
    const Vector<char> startKey = ObjectStoreDataKey::encode(databaseId, objectStoreId, minIDBKey());
    const Vector<char> stopKey = ObjectStoreDataKey::encode(databaseId, objectStoreId, maxIDBKey());

    OwnPtr<LevelDBIterator> it = m_db->createIterator();

    int maxNumericKey = 0;

    // FIXME: Be more efficient: seek to something after the object store data, then reverse.

    for (it->seek(startKey); it->isValid() && compareKeys(it->key(), stopKey) < 0; it->next()) {
        const char *p = it->key().begin();
        const char *limit = it->key().end();

        ObjectStoreDataKey dataKey;
        p = ObjectStoreDataKey::decode(p, limit, &dataKey);
        ASSERT(p);

        if (dataKey.userKey()->type() == IDBKey::NumberType) {
            int64_t n = static_cast<int64_t>(dataKey.userKey()->number());
            if (n > maxNumericKey)
                maxNumericKey = n;
        }
    }

    return maxNumericKey + 1;
}

bool IDBLevelDBBackingStore::keyExistsInObjectStore(int64_t databaseId, int64_t objectStoreId, const IDBKey& key, ObjectStoreRecordIdentifier* foundRecordIdentifier)
{
    const Vector<char> leveldbKey = ObjectStoreDataKey::encode(databaseId, objectStoreId, key);
    Vector<char> data;

    if (!m_db->get(leveldbKey, data))
        return false;

    int64_t version;
    if (!decodeVarInt(data.begin(), data.end(), version))
        return false;

    LevelDBRecordIdentifier* levelDBRecordIdentifier = static_cast<LevelDBRecordIdentifier*>(foundRecordIdentifier);
    levelDBRecordIdentifier->setPrimaryKey(encodeIDBKey(key));
    levelDBRecordIdentifier->setVersion(version);
    return true;
}

bool IDBLevelDBBackingStore::forEachObjectStoreRecord(int64_t databaseId, int64_t objectStoreId, ObjectStoreRecordCallback& callback)
{
    const Vector<char> startKey = ObjectStoreDataKey::encode(databaseId, objectStoreId, minIDBKey());
    const Vector<char> stopKey = ObjectStoreDataKey::encode(databaseId, objectStoreId, maxIDBKey());

    OwnPtr<LevelDBIterator> it = m_db->createIterator();
    for (it->seek(startKey); it->isValid() && compareKeys(it->key(), stopKey) < 0; it->next()) {
        const char *p = it->key().begin();
        const char *limit = it->key().end();
        ObjectStoreDataKey dataKey;
        p = ObjectStoreDataKey::decode(p, limit, &dataKey);
        ASSERT(p);

        RefPtr<IDBKey> primaryKey = dataKey.userKey();

        int64_t version;
        const char* q = decodeVarInt(it->value().begin(), it->value().end(), version);
        if (!q)
            return false;

        RefPtr<LevelDBRecordIdentifier> ri = LevelDBRecordIdentifier::create(encodeIDBKey(*primaryKey), version);
        String idbValue = decodeString(q, it->value().end());

        callback.callback(ri.get(), idbValue);
    }

    return true;
}

void IDBLevelDBBackingStore::getIndexes(int64_t databaseId, int64_t objectStoreId, Vector<int64_t>& foundIds, Vector<String>& foundNames, Vector<String>& foundKeyPaths, Vector<bool>& foundUniqueFlags)
{
    const Vector<char> startKey = IndexMetaDataKey::encode(databaseId, objectStoreId, 0, 0);
    const Vector<char> stopKey = IndexMetaDataKey::encode(databaseId, objectStoreId + 1, 0, 0);

    OwnPtr<LevelDBIterator> it = m_db->createIterator();
    for (it->seek(startKey); it->isValid() && compareKeys(it->key(), stopKey) < 0; it->next()) {
        const char* p = it->key().begin();
        const char* limit = it->key().end();

        IndexMetaDataKey metaDataKey;
        p = IndexMetaDataKey::decode(p, limit, &metaDataKey);
        ASSERT(p);

        int64_t indexId = metaDataKey.indexId();
        ASSERT(!metaDataKey.metaDataType());

        String indexName = decodeString(it->value().begin(), it->value().end());
        it->next();
        if (!it->isValid()) {
            LOG_ERROR("Internal Indexed DB error.");
            return;
        }

        bool indexUnique = *it->value().begin();
        it->next();
        if (!it->isValid()) {
            LOG_ERROR("Internal Indexed DB error.");
            return;
        }

        String keyPath = decodeString(it->value().begin(), it->value().end());

        foundIds.append(indexId);
        foundNames.append(indexName);
        foundKeyPaths.append(keyPath);
        foundUniqueFlags.append(indexUnique);
    }
}

static int64_t getNewIndexId(LevelDBDatabase* db, int64_t databaseId, int64_t objectStoreId)
{
    const Vector<char> startKey = IndexFreeListKey::encode(databaseId, objectStoreId, 0);
    const Vector<char> stopKey = IndexFreeListKey::encode(databaseId, objectStoreId, INT64_MAX);

    OwnPtr<LevelDBIterator> it = db->createIterator();
    for (it->seek(startKey); it->isValid() && compareKeys(it->key(), stopKey) < 0; it->next()) {
        const char* p = it->key().begin();
        const char* limit = it->key().end();

        IndexFreeListKey freeListKey;
        p = IndexFreeListKey::decode(p, limit, &freeListKey);
        ASSERT(p);

        bool ok = db->remove(it->key());
        ASSERT_UNUSED(ok, ok);

        ASSERT(freeListKey.indexId() >= kMinimumIndexId);
        return freeListKey.indexId();
    }

    int64_t maxIndexId = -1;
    const Vector<char> maxIndexIdKey = ObjectStoreMetaDataKey::encode(databaseId, objectStoreId, 5);
    if (!getInt(db, maxIndexIdKey, maxIndexId))
        maxIndexId = kMinimumIndexId;

    ASSERT(maxIndexId >= 0);

    int64_t indexId = maxIndexId + 1;
    bool ok = putInt(db, maxIndexIdKey, indexId);
    if (!ok)
        return false;

    return indexId;
}

bool IDBLevelDBBackingStore::createIndex(int64_t databaseId, int64_t objectStoreId, const String& name, const String& keyPath, bool isUnique, int64_t& indexId)
{
    indexId = getNewIndexId(m_db.get(), databaseId, objectStoreId);

    const Vector<char> nameKey = IndexMetaDataKey::encode(databaseId, objectStoreId, indexId, 0);
    const Vector<char> uniqueKey = IndexMetaDataKey::encode(databaseId, objectStoreId, indexId, 1);
    const Vector<char> keyPathKey = IndexMetaDataKey::encode(databaseId, objectStoreId, indexId, 2);

    bool ok = putString(m_db.get(), nameKey, name);
    if (!ok) {
        LOG_ERROR("Internal Indexed DB error.");
        return false;
    }

    ok = putInt(m_db.get(), uniqueKey, isUnique);
    if (!ok) {
        LOG_ERROR("Internal Indexed DB error.");
        return false;
    }

    ok = putString(m_db.get(), keyPathKey, keyPath);
    if (!ok) {
        LOG_ERROR("Internal Indexed DB error.");
        return false;
    }

    return true;
}

void IDBLevelDBBackingStore::deleteIndex(int64_t, int64_t, int64_t)
{
    ASSERT_NOT_REACHED(); // FIXME: Implement and add layout test.
    return;
}

bool IDBLevelDBBackingStore::putIndexDataForRecord(int64_t databaseId, int64_t objectStoreId, int64_t indexId, const IDBKey& key, const ObjectStoreRecordIdentifier* recordIdentifier)
{
    ASSERT(indexId >= kMinimumIndexId);
    const LevelDBRecordIdentifier* levelDBRecordIdentifier = static_cast<const LevelDBRecordIdentifier*>(recordIdentifier);

    const int64_t globalSequenceNumber = getNewVersionNumber(m_db.get(), databaseId, objectStoreId);
    const Vector<char> indexDataKey = IndexDataKey::encode(databaseId, objectStoreId, indexId, key, globalSequenceNumber);

    Vector<char> data;
    data.append(encodeVarInt(levelDBRecordIdentifier->version()));
    data.append(levelDBRecordIdentifier->primaryKey());

    return m_db->put(indexDataKey, data);
}

static bool findGreatestKeyLessThan(LevelDBDatabase* db, const Vector<char>& target, Vector<char>& foundKey)
{
    OwnPtr<LevelDBIterator> it = db->createIterator();
    it->seek(target);

    if (!it->isValid()) {
        it->seekToLast();
        if (!it->isValid())
            return false;
    }

    while (compareIndexKeys(it->key(), target) >= 0) {
        it->prev();
        if (!it->isValid())
            return false;
    }

    foundKey.clear();
    foundKey.append(it->key().begin(), it->key().end() - it->key().begin());
    return true;
}

bool IDBLevelDBBackingStore::deleteIndexDataForRecord(int64_t, int64_t, int64_t, const ObjectStoreRecordIdentifier*)
{
    // FIXME: This isn't needed since we invalidate index data via the version number mechanism.
    return true;
}

String IDBLevelDBBackingStore::getObjectViaIndex(int64_t databaseId, int64_t objectStoreId, int64_t indexId, const IDBKey& key)
{
    RefPtr<IDBKey> primaryKey = getPrimaryKeyViaIndex(databaseId, objectStoreId, indexId, key);
    if (!primaryKey)
        return String();

    return getObjectStoreRecord(databaseId, objectStoreId, *primaryKey);
}

static bool versionExists(LevelDBDatabase* db, int64_t databaseId, int64_t objectStoreId, int64_t version, const Vector<char>& encodedPrimaryKey)
{
    const Vector<char> key = ExistsEntryKey::encode(databaseId, objectStoreId, encodedPrimaryKey);
    Vector<char> data;

    if (!db->get(key, data))
        return false;

    return decodeInt(data.begin(), data.end()) == version;
}

PassRefPtr<IDBKey> IDBLevelDBBackingStore::getPrimaryKeyViaIndex(int64_t databaseId, int64_t objectStoreId, int64_t indexId, const IDBKey& key)
{
    const Vector<char> leveldbKey = IndexDataKey::encode(databaseId, objectStoreId, indexId, key, 0);
    OwnPtr<LevelDBIterator> it = m_db->createIterator();
    it->seek(leveldbKey);

    for (;;) {
        if (!it->isValid())
            return 0;
        if (compareIndexKeys(it->key(), leveldbKey) > 0)
            return 0;

        int64_t version;
        const char* p = decodeVarInt(it->value().begin(), it->value().end(), version);
        if (!p)
            return 0;
        Vector<char> encodedPrimaryKey;
        encodedPrimaryKey.append(p, it->value().end() - p);

        if (!versionExists(m_db.get(), databaseId, objectStoreId, version, encodedPrimaryKey)) {
            // Delete stale index data entry and continue.
            m_db->remove(it->key());
            it->next();
            continue;
        }

        RefPtr<IDBKey> primaryKey;
        decodeIDBKey(encodedPrimaryKey.begin(), encodedPrimaryKey.end(), primaryKey);
        return primaryKey.release();
    }
}

bool IDBLevelDBBackingStore::keyExistsInIndex(int64_t databaseId, int64_t objectStoreId, int64_t indexId, const IDBKey& key)
{
    const Vector<char> levelDBKey = IndexDataKey::encode(databaseId, objectStoreId, indexId, key, 0);
    OwnPtr<LevelDBIterator> it = m_db->createIterator();

    bool found = false;

    it->seek(levelDBKey);
    if (it->isValid() && !compareIndexKeys(it->key(), levelDBKey))
        found = true;

    return found;
}

namespace {
class CursorImplCommon : public IDBBackingStore::Cursor {
public:
    // IDBBackingStore::Cursor
    virtual bool continueFunction(const IDBKey*);
    virtual PassRefPtr<IDBKey> key() { return m_currentKey; }
    virtual PassRefPtr<IDBKey> primaryKey() { return m_currentKey; }
    virtual String value() = 0;
    virtual PassRefPtr<IDBBackingStore::ObjectStoreRecordIdentifier> objectStoreRecordIdentifier() = 0; // FIXME: I don't think this is actually used, so drop it.
    virtual int64_t indexDataId() = 0;

    virtual bool loadCurrentRow() = 0;
    bool firstSeek();

protected:
    CursorImplCommon(LevelDBDatabase* db, const Vector<char>& lowKey, bool lowOpen, const Vector<char>& highKey, bool highOpen, bool forward)
        : m_db(db)
        , m_lowKey(lowKey)
        , m_lowOpen(lowOpen)
        , m_highKey(highKey)
        , m_highOpen(highOpen)
        , m_forward(forward)
    {
    }
    virtual ~CursorImplCommon() {}

    LevelDBDatabase* m_db;
    OwnPtr<LevelDBIterator> m_iterator;
    Vector<char> m_lowKey;
    bool m_lowOpen;
    Vector<char> m_highKey;
    bool m_highOpen;
    bool m_forward;
    RefPtr<IDBKey> m_currentKey;
};

bool CursorImplCommon::firstSeek()
{
    m_iterator = m_db->createIterator();

    if (m_forward)
        m_iterator->seek(m_lowKey);
    else
        m_iterator->seek(m_highKey);

    for (;;) {
        if (!m_iterator->isValid())
            return false;

        if (m_forward && m_highOpen && compareIndexKeys(m_iterator->key(), m_highKey) >= 0)
            return false;
        if (m_forward && !m_highOpen && compareIndexKeys(m_iterator->key(), m_highKey) > 0)
            return false;
        if (!m_forward && m_lowOpen && compareIndexKeys(m_iterator->key(), m_lowKey) <= 0)
            return false;
        if (!m_forward && !m_lowOpen && compareIndexKeys(m_iterator->key(), m_lowKey) < 0)
            return false;

        if (m_forward && m_lowOpen) {
            // lowKey not included in the range.
            if (compareIndexKeys(m_iterator->key(), m_lowKey) <= 0) {
                m_iterator->next();
                continue;
            }
        }
        if (!m_forward && m_highOpen) {
            // highKey not included in the range.
            if (compareIndexKeys(m_iterator->key(), m_highKey) >= 0) {
                m_iterator->prev();
                continue;
            }
        }

        if (!loadCurrentRow()) {
            if (m_forward)
                m_iterator->next();
            else
                m_iterator->prev();
            continue;
        }

        return true;
    }
}

bool CursorImplCommon::continueFunction(const IDBKey* key)
{
    // FIXME: This shares a lot of code with firstSeek.

    for (;;) {
        if (m_forward)
            m_iterator->next();
        else
            m_iterator->prev();

        if (!m_iterator->isValid())
            return false;

        Vector<char> trash;
        if (!m_db->get(m_iterator->key(), trash))
             continue;

        if (m_forward && m_highOpen && compareIndexKeys(m_iterator->key(), m_highKey) >= 0) // high key not included in range
            return false;
        if (m_forward && !m_highOpen && compareIndexKeys(m_iterator->key(), m_highKey) > 0)
            return false;
        if (!m_forward && m_lowOpen && compareIndexKeys(m_iterator->key(), m_lowKey) <= 0) // low key not included in range
            return false;
        if (!m_forward && !m_lowOpen && compareIndexKeys(m_iterator->key(), m_lowKey) < 0)
            return false;

        if (!loadCurrentRow())
            continue;

        if (key) {
            if (m_forward) {
                if (m_currentKey->isLessThan(key))
                    continue;
            } else {
                if (key->isLessThan(m_currentKey.get()))
                    continue;
            }
        }

        // FIXME: Obey the uniqueness constraint (and test for it!)

        break;
    }

    return true;
}

class ObjectStoreCursorImpl : public CursorImplCommon {
public:
    static PassRefPtr<ObjectStoreCursorImpl> create(LevelDBDatabase* db, const Vector<char>& lowKey, bool lowOpen, const Vector<char>& highKey, bool highOpen, bool forward)
    {
        return adoptRef(new ObjectStoreCursorImpl(db, lowKey, lowOpen, highKey, highOpen, forward));
    }

    // CursorImplCommon
    virtual String value() { return m_currentValue; }
    virtual PassRefPtr<IDBBackingStore::ObjectStoreRecordIdentifier> objectStoreRecordIdentifier() { ASSERT_NOT_REACHED(); return 0; }
    virtual int64_t indexDataId() { ASSERT_NOT_REACHED(); return 0; }
    virtual bool loadCurrentRow();

private:
    ObjectStoreCursorImpl(LevelDBDatabase* db, const Vector<char>& lowKey, bool lowOpen, const Vector<char>& highKey, bool highOpen, bool forward)
        : CursorImplCommon(db, lowKey, lowOpen, highKey, highOpen, forward)
    {
    }

    String m_currentValue;
};

bool ObjectStoreCursorImpl::loadCurrentRow()
{
    const char* p = m_iterator->key().begin();
    const char* keyLimit = m_iterator->key().end();

    ObjectStoreDataKey objectStoreDataKey;
    p = ObjectStoreDataKey::decode(p, keyLimit, &objectStoreDataKey);
    ASSERT(p);
    if (!p)
        return false;

    m_currentKey = objectStoreDataKey.userKey();

    int64_t version;
    const char* q = decodeVarInt(m_iterator->value().begin(), m_iterator->value().end(), version);
    ASSERT(q);
    if (!q)
        return false;
    (void) version;

    m_currentValue = decodeString(q, m_iterator->value().end());

    return true;
}

class IndexKeyCursorImpl : public CursorImplCommon {
public:
    static PassRefPtr<IndexKeyCursorImpl> create(LevelDBDatabase* db, const Vector<char>& lowKey, bool lowOpen, const Vector<char>& highKey, bool highOpen, bool forward)
    {
        return adoptRef(new IndexKeyCursorImpl(db, lowKey, lowOpen, highKey, highOpen, forward));
    }

    // CursorImplCommon
    virtual String value() { ASSERT_NOT_REACHED(); return String(); }
    virtual PassRefPtr<IDBKey> primaryKey() { return m_primaryKey; }
    virtual PassRefPtr<IDBBackingStore::ObjectStoreRecordIdentifier> objectStoreRecordIdentifier() { ASSERT_NOT_REACHED(); return 0; }
    virtual int64_t indexDataId() { ASSERT_NOT_REACHED(); return 0; }
    virtual bool loadCurrentRow();

private:
    IndexKeyCursorImpl(LevelDBDatabase* db, const Vector<char>& lowKey, bool lowOpen, const Vector<char>& highKey, bool highOpen, bool forward)
        : CursorImplCommon(db, lowKey, lowOpen, highKey, highOpen, forward)
    {
    }

    RefPtr<IDBKey> m_primaryKey;
};

bool IndexKeyCursorImpl::loadCurrentRow()
{
    const char* p = m_iterator->key().begin();
    const char* keyLimit = m_iterator->key().end();
    IndexDataKey indexDataKey;
    p = IndexDataKey::decode(p, keyLimit, &indexDataKey);

    m_currentKey = indexDataKey.userKey();

    int64_t indexDataVersion;
    const char* q = decodeVarInt(m_iterator->value().begin(), m_iterator->value().end(), indexDataVersion);
    ASSERT(q);
    if (!q)
        return false;

    q = decodeIDBKey(q, m_iterator->value().end(), m_primaryKey);
    ASSERT(q);
    if (!q)
        return false;

    Vector<char> primaryLevelDBKey = ObjectStoreDataKey::encode(indexDataKey.databaseId(), indexDataKey.objectStoreId(), *m_primaryKey);

    Vector<char> result;
    if (!m_db->get(primaryLevelDBKey, result))
        return false;

    int64_t objectStoreDataVersion;
    const char* t = decodeVarInt(result.begin(), result.end(), objectStoreDataVersion);
    ASSERT(t);
    if (!t)
        return false;

    if (objectStoreDataVersion != indexDataVersion) { // FIXME: This is probably not very well covered by the layout tests.
        m_db->remove(m_iterator->key());
        return false;
    }

    return true;
}

class IndexCursorImpl : public CursorImplCommon {
public:
    static PassRefPtr<IndexCursorImpl> create(LevelDBDatabase* db, const Vector<char>& lowKey, bool lowOpen, const Vector<char>& highKey, bool highOpen, bool forward)
    {
        return adoptRef(new IndexCursorImpl(db, lowKey, lowOpen, highKey, highOpen, forward));
    }

    // CursorImplCommon
    virtual String value() { return m_value; }
    virtual PassRefPtr<IDBKey> primaryKey() { return m_primaryKey; }
    virtual PassRefPtr<IDBBackingStore::ObjectStoreRecordIdentifier> objectStoreRecordIdentifier() { ASSERT_NOT_REACHED(); return 0; }
    virtual int64_t indexDataId() { ASSERT_NOT_REACHED(); return 0; }
    bool loadCurrentRow();

private:
    IndexCursorImpl(LevelDBDatabase* db, const Vector<char>& lowKey, bool lowOpen, const Vector<char>& highKey, bool highOpen, bool forward)
        : CursorImplCommon(db, lowKey, lowOpen, highKey, highOpen, forward)
    {
    }

    RefPtr<IDBKey> m_primaryKey;
    String m_value;
    Vector<char> m_primaryLevelDBKey;
};

bool IndexCursorImpl::loadCurrentRow()
{
    const char *p = m_iterator->key().begin();
    const char *limit = m_iterator->key().end();

    IndexDataKey indexDataKey;
    p = IndexDataKey::decode(p, limit, &indexDataKey);

    m_currentKey = indexDataKey.userKey();

    const char *q = m_iterator->value().begin();
    const char *valueLimit = m_iterator->value().end();

    int64_t indexDataVersion;
    q = decodeVarInt(q, valueLimit, indexDataVersion);
    ASSERT(q);
    if (!q)
        return false;
    q = decodeIDBKey(q, valueLimit, m_primaryKey);
    ASSERT(q);
    if (!q)
        return false;

    m_primaryLevelDBKey = ObjectStoreDataKey::encode(indexDataKey.databaseId(), indexDataKey.objectStoreId(), *m_primaryKey);

    Vector<char> result;
    if (!m_db->get(m_primaryLevelDBKey, result))
        return false;

    int64_t objectStoreDataVersion;
    const char* t = decodeVarInt(result.begin(), result.end(), objectStoreDataVersion);
    ASSERT(t);
    if (!t)
        return false;

    if (objectStoreDataVersion != indexDataVersion) {
        m_db->remove(m_iterator->key());
        return false;
    }

    m_value = decodeString(t, result.end());
    return true;
}

}

static bool findLastIndexKeyEqualTo(LevelDBDatabase* db, const Vector<char>& target, Vector<char>& foundKey)
{
    OwnPtr<LevelDBIterator> it = db->createIterator();
    it->seek(target);

    if (!it->isValid())
        return false;

    while (it->isValid() && !compareIndexKeys(it->key(), target)) {
        foundKey.clear();
        foundKey.append(it->key().begin(), it->key().end() - it->key().begin());
        it->next();
    }

    return true;
}

PassRefPtr<IDBBackingStore::Cursor> IDBLevelDBBackingStore::openObjectStoreCursor(int64_t databaseId, int64_t objectStoreId, const IDBKeyRange* range, IDBCursor::Direction direction)
{
    bool lowerBound = range && range->lower();
    bool upperBound = range && range->upper();
    bool forward = (direction == IDBCursor::NEXT_NO_DUPLICATE || direction == IDBCursor::NEXT);

    bool lowerOpen, upperOpen;
    Vector<char> startKey, stopKey;

    if (!lowerBound) {
        startKey = ObjectStoreDataKey::encode(databaseId, objectStoreId, minIDBKey());
        lowerOpen = true; // Not included.
    } else {
        startKey = ObjectStoreDataKey::encode(databaseId, objectStoreId, *range->lower());
        lowerOpen = range->lowerOpen();
    }

    if (!upperBound) {
        stopKey = ObjectStoreDataKey::encode(databaseId, objectStoreId, maxIDBKey());
        upperOpen = true; // Not included.

        if (!forward) { // We need a key that exists.
            if (!findGreatestKeyLessThan(m_db.get(), stopKey, stopKey))
                return 0;
            upperOpen = false;
        }
    } else {
        stopKey = ObjectStoreDataKey::encode(databaseId, objectStoreId, *range->upper());
        upperOpen = range->upperOpen();
    }

    RefPtr<ObjectStoreCursorImpl> cursor = ObjectStoreCursorImpl::create(m_db.get(), startKey, lowerOpen, stopKey, upperOpen, forward);
    if (!cursor->firstSeek())
        return 0;

    return cursor.release();
}

PassRefPtr<IDBBackingStore::Cursor> IDBLevelDBBackingStore::openIndexKeyCursor(int64_t databaseId, int64_t objectStoreId, int64_t indexId, const IDBKeyRange* range, IDBCursor::Direction direction)
{
    bool lowerBound = range && range->lower();
    bool upperBound = range && range->upper();
    bool forward = (direction == IDBCursor::NEXT_NO_DUPLICATE || direction == IDBCursor::NEXT);

    bool lowerOpen, upperOpen;
    Vector<char> startKey, stopKey;

    if (!lowerBound) {
        startKey = IndexDataKey::encode(databaseId, objectStoreId, indexId, minIDBKey(), 0);
        lowerOpen = false; // Included.
    } else {
        startKey = IndexDataKey::encode(databaseId, objectStoreId, indexId, *range->lower(), 0);
        lowerOpen = range->lowerOpen();
    }

    if (!upperBound) {
        stopKey = IndexDataKey::encode(databaseId, objectStoreId, indexId, maxIDBKey(), 0);
        upperOpen = false; // Included.

        if (!forward) { // We need a key that exists.
            if (!findGreatestKeyLessThan(m_db.get(), stopKey, stopKey))
                return 0;
            upperOpen = false;
        }
    } else {
        stopKey = IndexDataKey::encode(databaseId, objectStoreId, indexId, *range->upper(), 0);
        if (!findLastIndexKeyEqualTo(m_db.get(), stopKey, stopKey)) // Seek to the *last* key in the set of non-unique keys.
            return 0;
        upperOpen = range->upperOpen();
    }

    RefPtr<IndexKeyCursorImpl> cursor = IndexKeyCursorImpl::create(m_db.get(), startKey, lowerOpen, stopKey, upperOpen, forward);
    if (!cursor->firstSeek())
        return 0;

    return cursor.release();
}

PassRefPtr<IDBBackingStore::Cursor> IDBLevelDBBackingStore::openIndexCursor(int64_t databaseId, int64_t objectStoreId, int64_t indexId, const IDBKeyRange* range, IDBCursor::Direction direction)
{
    bool lowerBound = range && range->lower();
    bool upperBound = range && range->upper();
    bool forward = (direction == IDBCursor::NEXT_NO_DUPLICATE || direction == IDBCursor::NEXT);

    bool lowerOpen, upperOpen;
    Vector<char> startKey, stopKey;

    if (!lowerBound) {
        startKey = IndexDataKey::encode(databaseId, objectStoreId, indexId, minIDBKey(), 0);
        lowerOpen = false; // Included.
    } else {
        startKey = IndexDataKey::encode(databaseId, objectStoreId, indexId, *range->lower(), 0);
        lowerOpen = range->lowerOpen();
    }

    if (!upperBound) {
        stopKey = IndexDataKey::encode(databaseId, objectStoreId, indexId, maxIDBKey(), 0);
        upperOpen = false; // Included.

        if (!forward) { // We need a key that exists.
            if (!findGreatestKeyLessThan(m_db.get(), stopKey, stopKey))
                return 0;
            upperOpen = false;
        }
    } else {
        stopKey = IndexDataKey::encode(databaseId, objectStoreId, indexId, *range->upper(), 0);
        if (!findLastIndexKeyEqualTo(m_db.get(), stopKey, stopKey)) // Seek to the *last* key in the set of non-unique keys.
            return 0;
        upperOpen = range->upperOpen();
    }

    RefPtr<IndexCursorImpl> cursor = IndexCursorImpl::create(m_db.get(), startKey, lowerOpen, stopKey, upperOpen, forward);
    if (!cursor->firstSeek())
        return 0;

    return cursor.release();
}

namespace {
class DummyTransaction : public IDBBackingStore::Transaction {
public:
    virtual void begin() {}
    virtual void commit() {}
    virtual void rollback() {}
};
}

PassRefPtr<IDBBackingStore::Transaction> IDBLevelDBBackingStore::createTransaction()
{
    // FIXME: We need to implement a transaction abstraction that allows for roll-backs, and write tests for it.
    return adoptRef(new DummyTransaction());
}

// FIXME: deleteDatabase should be part of IDBBackingStore.

} // namespace WebCore

#endif // ENABLE(LEVELDB)
#endif // ENABLE(INDEXED_DATABASE)
