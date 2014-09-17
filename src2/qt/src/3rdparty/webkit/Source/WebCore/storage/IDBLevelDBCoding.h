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

#ifndef IDBLevelDBCoding_h
#define IDBLevelDBCoding_h

#if ENABLE(INDEXED_DATABASE)
#if ENABLE(LEVELDB)

#include <wtf/RefPtr.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class IDBKey;
class LevelDBSlice;

namespace IDBLevelDBCoding {

const unsigned char kMinimumIndexId = 30;

Vector<char> encodeByte(unsigned char);
Vector<char> maxIDBKey();
Vector<char> minIDBKey();
Vector<char> encodeInt(int64_t);
int64_t decodeInt(const char* begin, const char* end);
Vector<char> encodeVarInt(int64_t);
const char* decodeVarInt(const char *p, const char* limit, int64_t& foundInt);
Vector<char> encodeString(const String&);
String decodeString(const char* p, const char* end);
Vector<char> encodeStringWithLength(const String&);
const char* decodeStringWithLength(const char* p, const char* limit, String& foundString);
Vector<char> encodeDouble(double);
const char* decodeDouble(const char* p, const char* limit, double*);
Vector<char> encodeIDBKey(const IDBKey&);
const char* decodeIDBKey(const char* p, const char* limit, RefPtr<IDBKey>& foundKey);
const char* extractEncodedIDBKey(const char* start, const char* limit, Vector<char>* result);
int compareEncodedIDBKeys(const Vector<char>&, const Vector<char>&);

int compare(const LevelDBSlice&, const LevelDBSlice&, bool indexKeys = false);

class KeyPrefix {
public:
    KeyPrefix();
    KeyPrefix(int64_t databaseId, int64_t objectStoreId, int64_t indexId);

    static const char* decode(const char* start, const char* limit, KeyPrefix* result);
    Vector<char> encode() const;
    int compare(const KeyPrefix& other) const;

    enum Type {
        kGlobalMetaData,
        kDatabaseMetaData,
        kObjectStoreData,
        kExistsEntry,
        kIndexData,
        kInvalidType
    };

    Type type() const;

    int64_t m_databaseId;
    int64_t m_objectStoreId;
    int64_t m_indexId;

    static const int64_t kInvalidId = -1;
};

class SchemaVersionKey {
public:
    static Vector<char> encode();
};

class MaxDatabaseIdKey {
public:
    static Vector<char> encode();
};

class DatabaseFreeListKey {
public:
    DatabaseFreeListKey();
    static const char* decode(const char* start, const char* limit, DatabaseFreeListKey* result);
    static Vector<char> encode(int64_t databaseId);
    int64_t databaseId() const;
    int compare(const DatabaseFreeListKey& other) const;

private:
    int64_t m_databaseId;
};

class DatabaseNameKey {
public:
    static const char* decode(const char* start, const char* limit, DatabaseNameKey* result);
    static Vector<char> encode(const String& origin, const String& databaseName);
    String origin() const { return m_origin; }
    String databaseName() const { return m_databaseName; }
    int compare(const DatabaseNameKey& other);

private:
    String m_origin; // FIXME: Store encoded strings, or just pointers.
    String m_databaseName;
};

class DatabaseMetaDataKey {
public:
    enum MetaDataType {
        kOriginName = 0,
        kDatabaseName = 1,
        kUserVersion = 2,
        kMaxObjectStoreId = 3
    };

    static Vector<char> encode(int64_t databaseId, MetaDataType);
};

class ObjectStoreMetaDataKey {
public:
    ObjectStoreMetaDataKey();
    static const char* decode(const char* start, const char* limit, ObjectStoreMetaDataKey* result);
    static Vector<char> encode(int64_t databaseId, int64_t objectStoreId, int64_t metaDataType);
    int64_t objectStoreId() const;
    int64_t metaDataType() const;
    int compare(const ObjectStoreMetaDataKey& other);

private:
    int64_t m_objectStoreId;
    int64_t m_metaDataType; // FIXME: Make this a byte.
};

class IndexMetaDataKey {
public:
    IndexMetaDataKey();
    static const char* decode(const char* start, const char* limit, IndexMetaDataKey* result);
    static Vector<char> encode(int64_t databaseId, int64_t objectStoreId, int64_t indexId, unsigned char metaDataType);
    int compare(const IndexMetaDataKey& other);
    int64_t indexId() const;
    unsigned char metaDataType() const { return m_metaDataType; }

private:
    int64_t m_objectStoreId;
    int64_t m_indexId;
    unsigned char m_metaDataType;
};

class ObjectStoreFreeListKey {
public:
    ObjectStoreFreeListKey();
    static const char* decode(const char* start, const char* limit, ObjectStoreFreeListKey* result);
    static Vector<char> encode(int64_t databaseId, int64_t objectStoreId);
    int64_t objectStoreId() const;
    int compare(const ObjectStoreFreeListKey& other);

private:
    int64_t m_objectStoreId;
};

class IndexFreeListKey {
public:
    IndexFreeListKey();
    static const char* decode(const char* start, const char* limit, IndexFreeListKey* result);
    static Vector<char> encode(int64_t databaseId, int64_t objectStoreId, int64_t indexId);
    int compare(const IndexFreeListKey& other);
    int64_t objectStoreId() const;
    int64_t indexId() const;

private:
    int64_t m_objectStoreId;
    int64_t m_indexId;
};

class ObjectStoreNamesKey {
public:
    // FIXME: We never use this to look up object store ids, because a mapping
    // is kept in the IDBDatabaseBackendImpl. Can the mapping become unreliable?
    // Can we remove this?
    static const char* decode(const char* start, const char* limit, ObjectStoreNamesKey* result);
    static Vector<char> encode(int64_t databaseId, const String& objectStoreName);
    int compare(const ObjectStoreNamesKey& other);
    String objectStoreName() const { return m_objectStoreName; }

private:
    String m_objectStoreName; // FIXME: Store the encoded string, or just pointers to it.
};

class IndexNamesKey {
public:
    IndexNamesKey();
    // FIXME: We never use this to look up index ids, because a mapping
    // is kept at a higher level.
    static const char* decode(const char* start, const char* limit, IndexNamesKey* result);
    static Vector<char> encode(int64_t databaseId, int64_t objectStoreId, const String& indexName);
    int compare(const IndexNamesKey& other);
    String indexName() const { return m_indexName; }

private:
    int64_t m_objectStoreId;
    String m_indexName;
};

class ObjectStoreDataKey {
public:
    static const char* decode(const char* start, const char* end, ObjectStoreDataKey* result);
    static Vector<char> encode(int64_t databaseId, int64_t objectStoreId, const Vector<char> encodedUserKey);
    static Vector<char> encode(int64_t databaseId, int64_t objectStoreId, const IDBKey& userKey);
    int compare(const ObjectStoreDataKey& other);
    PassRefPtr<IDBKey> userKey() const;
    static const int64_t kSpecialIndexNumber;

private:
    Vector<char> m_encodedUserKey;
};

class ExistsEntryKey {
public:
    static const char* decode(const char* start, const char* end, ExistsEntryKey* result);
    static Vector<char> encode(int64_t databaseId, int64_t objectStoreId, const Vector<char>& encodedKey);
    static Vector<char> encode(int64_t databaseId, int64_t objectStoreId, const IDBKey& userKey);
    int compare(const ExistsEntryKey& other);
    PassRefPtr<IDBKey> userKey() const;

    static const int64_t kSpecialIndexNumber;

private:
    Vector<char> m_encodedUserKey;
};

class IndexDataKey {
public:
    IndexDataKey();
    static const char* decode(const char* start, const char* limit, IndexDataKey* result);
    static Vector<char> encode(int64_t databaseId, int64_t objectStoreId, int64_t indexId, const Vector<char>& encodedUserKey, int64_t sequenceNumber);
    static Vector<char> encode(int64_t databaseId, int64_t objectStoreId, int64_t indexId, const IDBKey& userKey, int64_t sequenceNumber);
    static Vector<char> encodeMaxKey(int64_t databaseId, int64_t objectStoreId);
    int compare(const IndexDataKey& other, bool ignoreSequenceNumber);
    int64_t databaseId() const;
    int64_t objectStoreId() const;
    int64_t indexId() const;
    PassRefPtr<IDBKey> userKey() const;

private:
    int64_t m_databaseId;
    int64_t m_objectStoreId;
    int64_t m_indexId;
    Vector<char> m_encodedUserKey;
    int64_t m_sequenceNumber;
};

} // namespace IDBLevelDBCoding

} // namespace WebCore

#endif // ENABLE(LEVELDB)
#endif // ENABLE(INDEXED_DATABASE)

#endif // IDBLevelDBCoding_h
