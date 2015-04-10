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
#include "IDBLevelDBCoding.h"

#if ENABLE(INDEXED_DATABASE)
#if USE(LEVELDB)

#include "IDBKey.h"
#include "IDBKeyPath.h"
#include "LevelDBSlice.h"
#include <wtf/ByteOrder.h>
#include <wtf/text/StringBuilder.h>

// LevelDB stores key/value pairs. Keys and values are strings of bytes, normally of type Vector<char>.
//
// The keys in the backing store are variable-length tuples with different types
// of fields. Each key in the backing store starts with a ternary prefix: (database id, object store id, index id). For each, 0 is reserved for meta-data.
// The prefix makes sure that data for a specific database, object store, and index are grouped together. The locality is important for performance: common
// operations should only need a minimal number of seek operations. For example, all the meta-data for a database is grouped together so that reading that
// meta-data only requires one seek.
//
// Each key type has a class (in square brackets below) which knows how to encode, decode, and compare that key type.
//
// Global meta-data have keys with prefix (0,0,0), followed by a type byte:
//
//     <0, 0, 0, 0>                                           => IndexedDB/LevelDB schema version [SchemaVersionKey]
//     <0, 0, 0, 1>                                           => The maximum database id ever allocated [MaxDatabaseIdKey]
//     <0, 0, 0, 2>                                           => SerializedScriptValue version [DataVersionKey]
//     <0, 0, 0, 100, database id>                            => Existence implies the database id is in the free list [DatabaseFreeListKey]
//     <0, 0, 0, 201, utf16 origin name, utf16 database name> => Database id [DatabaseNameKey]
//
//
// Database meta-data:
//
//     Again, the prefix is followed by a type byte.
//
//     <database id, 0, 0, 0> => utf16 origin name [DatabaseMetaDataKey]
//     <database id, 0, 0, 1> => utf16 database name [DatabaseMetaDataKey]
//     <database id, 0, 0, 2> => utf16 user version data [DatabaseMetaDataKey]
//     <database id, 0, 0, 3> => maximum object store id ever allocated [DatabaseMetaDataKey]
//     <database id, 0, 0, 4> => user integer version (var int) [DatabaseMetaDataKey]
//
//
// Object store meta-data:
//
//     The prefix is followed by a type byte, then a variable-length integer, and then another type byte.
//
//     <database id, 0, 0, 50, object store id, 0> => utf16 object store name [ObjectStoreMetaDataKey]
//     <database id, 0, 0, 50, object store id, 1> => utf16 key path [ObjectStoreMetaDataKey]
//     <database id, 0, 0, 50, object store id, 2> => has auto increment [ObjectStoreMetaDataKey]
//     <database id, 0, 0, 50, object store id, 3> => is evictable [ObjectStoreMetaDataKey]
//     <database id, 0, 0, 50, object store id, 4> => last "version" number [ObjectStoreMetaDataKey]
//     <database id, 0, 0, 50, object store id, 5> => maximum index id ever allocated [ObjectStoreMetaDataKey]
//     <database id, 0, 0, 50, object store id, 6> => has key path (vs. null) [ObjectStoreMetaDataKey]
//     <database id, 0, 0, 50, object store id, 7> => key generator current number [ObjectStoreMetaDataKey]
//
//
// Index meta-data:
//
//     The prefix is followed by a type byte, then two variable-length integers, and then another type byte.
//
//     <database id, 0, 0, 100, object store id, index id, 0> => utf16 index name [IndexMetaDataKey]
//     <database id, 0, 0, 100, object store id, index id, 1> => are index keys unique [IndexMetaDataKey]
//     <database id, 0, 0, 100, object store id, index id, 2> => utf16 key path [IndexMetaDataKey]
//     <database id, 0, 0, 100, object store id, index id, 3> => is index multi-entry [IndexMetaDataKey]
//
//
// Other object store and index meta-data:
//
//     The prefix is followed by a type byte. The object store and index id are variable length integers, the utf16 strings are variable length strings.
//
//     <database id, 0, 0, 150, object store id>                   => existence implies the object store id is in the free list [ObjectStoreFreeListKey]
//     <database id, 0, 0, 151, object store id, index id>         => existence implies the index id is in the free list [IndexFreeListKey]
//     <database id, 0, 0, 200, utf16 object store name>           => object store id [ObjectStoreNamesKey]
//     <database id, 0, 0, 201, object store id, utf16 index name> => index id [IndexNamesKey]
//
//
// Object store data:
//
//     The prefix is followed by a type byte. The user key is an encoded IDBKey.
//
//     <database id, object store id, 1, user key> => "version", serialized script value [ObjectStoreDataKey]
//
//
// "Exists" entry:
//
//     The prefix is followed by a type byte. The user key is an encoded IDBKey.
//
//     <database id, object store id, 2, user key> => "version" [ExistsEntryKey]
//
//
// Index data:
//
//     The prefix is followed by a type byte. The index key is an encoded IDBKey. The sequence number is a variable length integer.
//     The primary key is an encoded IDBKey.
//
//     <database id, object store id, index id, index key, sequence number, primary key> => "version", primary key [IndexDataKey]
//
//     (The sequence number is obsolete; it was used to allow two entries with
//     the same user (index) key in non-unique indexes prior to the inclusion of
//     the primary key in the data. The "version" field is used to weed out stale
//     index data. Whenever new object store data is inserted, it gets a new
//     "version" number, and new index data is written with this number. When
//     the index is used for look-ups, entries are validated against the
//     "exists" entries, and records with old "version" numbers are deleted
//     when they are encountered in getPrimaryKeyViaIndex,
//     IndexCursorImpl::loadCurrentRow, and IndexKeyCursorImpl::loadCurrentRow).

namespace WebCore {
namespace IDBLevelDBCoding {

#ifndef INT64_MAX
#define INT64_MAX 0x7fffffffffffffffLL
#endif
#ifndef INT32_MAX
#define INT32_MAX 0x7fffffffL
#endif

static const unsigned char IDBKeyNullTypeByte = 0;
static const unsigned char IDBKeyStringTypeByte = 1;
static const unsigned char IDBKeyDateTypeByte = 2;
static const unsigned char IDBKeyNumberTypeByte = 3;
static const unsigned char IDBKeyArrayTypeByte = 4;
static const unsigned char IDBKeyMinKeyTypeByte = 5;

static const unsigned char IDBKeyPathTypeCodedByte1 = 0;
static const unsigned char IDBKeyPathTypeCodedByte2 = 0;

static const unsigned char ObjectStoreDataIndexId = 1;
static const unsigned char ExistsEntryIndexId = 2;

static const unsigned char SchemaVersionTypeByte = 0;
static const unsigned char MaxDatabaseIdTypeByte = 1;
static const unsigned char DataVersionTypeByte = 2;
static const unsigned char MaxSimpleGlobalMetaDataTypeByte = 3; // Insert before this and increment.
static const unsigned char DatabaseFreeListTypeByte = 100;
static const unsigned char DatabaseNameTypeByte = 201;

static const unsigned char ObjectStoreMetaDataTypeByte = 50;
static const unsigned char IndexMetaDataTypeByte = 100;
static const unsigned char ObjectStoreFreeListTypeByte = 150;
static const unsigned char IndexFreeListTypeByte = 151;
static const unsigned char ObjectStoreNamesTypeByte = 200;
static const unsigned char IndexNamesKeyTypeByte = 201;

static const unsigned char ObjectMetaDataTypeMaximum = 255;
static const unsigned char IndexMetaDataTypeMaximum = 255;

Vector<char> encodeByte(unsigned char c)
{
    Vector<char, DefaultInlineBufferSize> v;
    v.append(c);

    ASSERT(v.size() <= DefaultInlineBufferSize);
    return v;
}

const char* decodeByte(const char* p, const char* limit, unsigned char& foundChar)
{
    if (p >= limit)
        return 0;

    foundChar = *p++;
    return p;
}

Vector<char> maxIDBKey()
{
    return encodeByte(IDBKeyNullTypeByte);
}

Vector<char> minIDBKey()
{
    return encodeByte(IDBKeyMinKeyTypeByte);
}

Vector<char> encodeBool(bool b)
{
    Vector<char, DefaultInlineBufferSize> ret;
    ret.append(b ? 1 : 0);

    ASSERT(ret.size() <= DefaultInlineBufferSize);
    return ret;
}

bool decodeBool(const char* begin, const char* end)
{
    ASSERT_UNUSED(end, begin < end);
    return *begin;
}

Vector<char> encodeInt(int64_t nParam)
{
    ASSERT(nParam >= 0);
    uint64_t n = static_cast<uint64_t>(nParam);
    Vector<char, DefaultInlineBufferSize> ret;

    do {
        unsigned char c = n;
        ret.append(c);
        n >>= 8;
    } while (n);

    ASSERT(ret.size() <= DefaultInlineBufferSize);
    return ret;
}

int64_t decodeInt(const char* begin, const char* end)
{
    ASSERT(begin <= end);
    int64_t ret = 0;

    int shift = 0;
    while (begin < end) {
        unsigned char c = *begin++;
        ret |= static_cast<int64_t>(c) << shift;
        shift += 8;
    }

    return ret;
}

static int compareInts(int64_t a, int64_t b)
{
    ASSERT(a >= 0);
    ASSERT(b >= 0);

    int64_t diff = a - b;
    if (diff < 0)
        return -1;
    if (diff > 0)
        return 1;
    return 0;
}

Vector<char> encodeVarInt(int64_t nParam)
{
    ASSERT(nParam >= 0);
    uint64_t n = static_cast<uint64_t>(nParam);
    Vector<char, DefaultInlineBufferSize> ret;

    do {
        unsigned char c = n & 0x7f;
        n >>= 7;
        if (n)
            c |= 0x80;
        ret.append(c);
    } while (n);

    ASSERT(ret.size() <= DefaultInlineBufferSize);
    return ret;
}

const char* decodeVarInt(const char* p, const char* limit, int64_t& foundInt)
{
    ASSERT(limit >= p);
    foundInt = 0;
    int shift = 0;

    do {
        if (p >= limit)
            return 0;

        unsigned char c = *p;
        foundInt |= static_cast<int64_t>(c & 0x7f) << shift;
        shift += 7;
    } while (*p++ & 0x80);
    return p;
}

Vector<char> encodeString(const String& s)
{
    // Backing store is UTF-16BE, convert from host endianness.
    size_t length = s.length();
    Vector<char> ret(length * sizeof(UChar));

    const UChar* src = s.characters();
    UChar* dst = reinterpret_cast<UChar*>(ret.data());
    for (unsigned i = 0; i < length; ++i)
        *dst++ = htons(*src++);

    return ret;
}

String decodeString(const char* start, const char* end)
{
    // Backing store is UTF-16BE, convert to host endianness.
    ASSERT(end >= start);
    ASSERT(!((end - start) % sizeof(UChar)));

    size_t length = (end - start) / sizeof(UChar);
    Vector<UChar> buffer(length);

    const UChar* src = reinterpret_cast<const UChar*>(start);
    UChar* dst = buffer.data();
    for (unsigned i = 0; i < length; ++i)
        *dst++ = ntohs(*src++);

    return String::adopt(buffer);
}

Vector<char> encodeStringWithLength(const String& s)
{
    Vector<char> ret = encodeVarInt(s.length());
    ret.appendVector(encodeString(s));
    return ret;
}

const char* decodeStringWithLength(const char* p, const char* limit, String& foundString)
{
    ASSERT(limit >= p);
    int64_t len;
    p = decodeVarInt(p, limit, len);
    if (!p || len < 0 || p + len * 2 > limit)
        return 0;

    foundString = decodeString(p, p + len * 2);
    p += len * 2;
    return p;
}

int compareEncodedStringsWithLength(const char*& p, const char* limitP, const char*& q, const char* limitQ, bool& ok)
{
    ASSERT(&p != &q);
    ASSERT(limitP >= p);
    ASSERT(limitQ >= q);
    int64_t lenP, lenQ;
    p = decodeVarInt(p, limitP, lenP);
    q = decodeVarInt(q, limitQ, lenQ);
    if (!p || !q || lenP < 0 || lenQ < 0) {
        ok = false;
        return 0;
    }
    ASSERT(p && q);
    ASSERT(lenP >= 0);
    ASSERT(lenQ >= 0);
    ASSERT(p + lenP * 2 <= limitP);
    ASSERT(q + lenQ * 2 <= limitQ);

    const char* startP = p;
    const char* startQ = q;
    p += lenP * 2;
    q += lenQ * 2;

    if (p > limitP || q > limitQ) {
        ok = false;
        return 0;
    }

    ok = true;
    const size_t lmin = static_cast<size_t>(lenP < lenQ ? lenP : lenQ);
    if (int x = memcmp(startP, startQ, lmin * 2))
        return x;

    if (lenP == lenQ)
        return 0;

    return (lenP > lenQ) ? 1 : -1;
}

Vector<char> encodeDouble(double x)
{
    // FIXME: It would be nice if we could be byte order independent.
    const char* p = reinterpret_cast<char*>(&x);
    Vector<char, DefaultInlineBufferSize> v;
    v.append(p, sizeof(x));

    ASSERT(v.size() <= DefaultInlineBufferSize);
    return v;
}

const char* decodeDouble(const char* p, const char* limit, double* d)
{
    if (p + sizeof(*d) > limit)
        return 0;

    char* x = reinterpret_cast<char*>(d);
    for (size_t i = 0; i < sizeof(*d); ++i)
        *x++ = *p++;
    return p;
}

Vector<char> encodeIDBKey(const IDBKey& key)
{
    Vector<char, DefaultInlineBufferSize> ret;
    encodeIDBKey(key, ret);
    return ret;
}

void encodeIDBKey(const IDBKey& key, Vector<char, DefaultInlineBufferSize>& into)
{
    size_t previousSize = into.size();
    ASSERT(key.isValid());
    switch (key.type()) {
    case IDBKey::InvalidType:
    case IDBKey::MinType:
        ASSERT_NOT_REACHED();
        into.appendVector(encodeByte(IDBKeyNullTypeByte));
        return;
    case IDBKey::ArrayType: {
        into.appendVector(encodeByte(IDBKeyArrayTypeByte));
        size_t length = key.array().size();
        into.appendVector(encodeVarInt(length));
        for (size_t i = 0; i < length; ++i)
            encodeIDBKey(*key.array()[i], into);
        ASSERT_UNUSED(previousSize, into.size() > previousSize);
        return;
    }
    case IDBKey::StringType:
        into.appendVector(encodeByte(IDBKeyStringTypeByte));
        into.appendVector(encodeStringWithLength(key.string()));
        ASSERT_UNUSED(previousSize, into.size() > previousSize);
        return;
    case IDBKey::DateType:
        into.appendVector(encodeByte(IDBKeyDateTypeByte));
        into.appendVector(encodeDouble(key.date()));
        ASSERT_UNUSED(previousSize, into.size() - previousSize == 9);
        return;
    case IDBKey::NumberType:
        into.appendVector(encodeByte(IDBKeyNumberTypeByte));
        into.appendVector(encodeDouble(key.number()));
        ASSERT_UNUSED(previousSize, into.size() - previousSize == 9);
        return;
    }

    ASSERT_NOT_REACHED();
}


const char* decodeIDBKey(const char* p, const char* limit, RefPtr<IDBKey>& foundKey)
{
    ASSERT(limit >= p);
    if (p >= limit)
        return 0;

    unsigned char type = *p++;

    switch (type) {
    case IDBKeyNullTypeByte:
        foundKey = IDBKey::createInvalid();
        return p;

    case IDBKeyArrayTypeByte: {
        int64_t length;
        p = decodeVarInt(p, limit, length);
        if (!p || length < 0)
            return 0;
        IDBKey::KeyArray array;
        while (length--) {
            RefPtr<IDBKey> key;
            p = decodeIDBKey(p, limit, key);
            if (!p)
                return 0;
            array.append(key);
        }
        foundKey = IDBKey::createArray(array);
        return p;
    }
    case IDBKeyStringTypeByte: {
        String s;
        p = decodeStringWithLength(p, limit, s);
        if (!p)
            return 0;
        foundKey = IDBKey::createString(s);
        return p;
    }
    case IDBKeyDateTypeByte: {
        double d;
        p = decodeDouble(p, limit, &d);
        if (!p)
            return 0;
        foundKey = IDBKey::createDate(d);
        return p;
    }
    case IDBKeyNumberTypeByte: {
        double d;
        p = decodeDouble(p, limit, &d);
        if (!p)
            return 0;
        foundKey = IDBKey::createNumber(d);
        return p;
    }
    }

    ASSERT_NOT_REACHED();
    return 0;
}

const char* extractEncodedIDBKey(const char* start, const char* limit, Vector<char>* result = 0)
{
    const char* p = start;
    if (p >= limit)
        return 0;

    unsigned char type = *p++;

    switch (type) {
    case IDBKeyNullTypeByte:
    case IDBKeyMinKeyTypeByte:
        break;
    case IDBKeyArrayTypeByte: {
        int64_t length;
        p = decodeVarInt(p, limit, length);
        if (!p || length < 0)
            return 0;
        while (length--) {
            p = extractEncodedIDBKey(p, limit);
            if (!p)
                return 0;
        }
        break;
    }
    case IDBKeyStringTypeByte: {
        int64_t length;
        p = decodeVarInt(p, limit, length);
        if (!p || length < 0 || p + length * 2 > limit)
            return 0;
        p += length * 2;
        break;
    }
    case IDBKeyDateTypeByte:
    case IDBKeyNumberTypeByte:
        if (p + sizeof(double) > limit)
            return 0;
        p += sizeof(double);
        break;
    }

    if (result) {
        ASSERT(p);
        ASSERT(p <= limit);
        result->clear();
        result->append(start, p - start);
    }

    return p;
}

static IDBKey::Type keyTypeByteToKeyType(unsigned char type)
{
    switch (type) {
    case IDBKeyNullTypeByte:
        return IDBKey::InvalidType;
    case IDBKeyArrayTypeByte:
        return IDBKey::ArrayType;
    case IDBKeyStringTypeByte:
        return IDBKey::StringType;
    case IDBKeyDateTypeByte:
        return IDBKey::DateType;
    case IDBKeyNumberTypeByte:
        return IDBKey::NumberType;
    case IDBKeyMinKeyTypeByte:
        return IDBKey::MinType;
    }

    ASSERT_NOT_REACHED();
    return IDBKey::InvalidType;
}

int compareEncodedIDBKeys(const char*& ptrA, const char* limitA, const char*& ptrB, const char* limitB, bool& ok)
{
    ok = true;
    ASSERT(&ptrA != &ptrB);
    ASSERT(ptrA < limitA);
    ASSERT(ptrB < limitB);
    unsigned char typeA = *ptrA++;
    unsigned char typeB = *ptrB++;

    if (int x = IDBKey::compareTypes(keyTypeByteToKeyType(typeA), keyTypeByteToKeyType(typeB)))
        return x;

    switch (typeA) {
    case IDBKeyNullTypeByte:
    case IDBKeyMinKeyTypeByte:
        // Null type or max type; no payload to compare.
        return 0;
    case IDBKeyArrayTypeByte: {
        int64_t lengthA, lengthB;
        ptrA = decodeVarInt(ptrA, limitA, lengthA);
        ptrB = decodeVarInt(ptrB, limitB, lengthB);
        if (!ptrA || !ptrB || lengthA < 0 || lengthB < 0) {
            ok = false;
            return 0;
        }
        for (int64_t i = 0; i < lengthA && i < lengthB; ++i) {
            int result = compareEncodedIDBKeys(ptrA, limitA, ptrB, limitB, ok);
            if (!ok || result)
                return result;
        }
        if (lengthA < lengthB)
            return -1;
        if (lengthA > lengthB)
            return 1;
        return 0;
    }
    case IDBKeyStringTypeByte:
        return compareEncodedStringsWithLength(ptrA, limitA, ptrB, limitB, ok);
    case IDBKeyDateTypeByte:
    case IDBKeyNumberTypeByte: {
        double d, e;
        ptrA = decodeDouble(ptrA, limitA, &d);
        ptrB = decodeDouble(ptrB, limitB, &e);
        ASSERT(ptrA);
        ASSERT(ptrB);
        if (!ptrA || !ptrB) {
            ok = false;
            return 0;
        }
        if (d < e)
            return -1;
        if (d > e)
            return 1;
        return 0;
    }
    }

    ASSERT_NOT_REACHED();
    return 0;
}

int compareEncodedIDBKeys(const Vector<char>& keyA, const Vector<char>& keyB, bool& ok)
{
    ASSERT(keyA.size() >= 1);
    ASSERT(keyB.size() >= 1);

    const char* ptrA = keyA.data();
    const char* limitA = ptrA + keyA.size();
    const char* ptrB = keyB.data();
    const char* limitB = ptrB + keyB.size();

    return compareEncodedIDBKeys(ptrA, limitA, ptrB, limitB, ok);
}

Vector<char> encodeIDBKeyPath(const IDBKeyPath& keyPath)
{
    // May be typed, or may be a raw string. An invalid leading
    // byte is used to identify typed coding. New records are
    // always written as typed.
    Vector<char, DefaultInlineBufferSize> ret;
    ret.append(IDBKeyPathTypeCodedByte1);
    ret.append(IDBKeyPathTypeCodedByte2);
    ret.append(static_cast<char>(keyPath.type()));
    switch (keyPath.type()) {
    case IDBKeyPath::NullType:
        break;
    case IDBKeyPath::StringType:
        ret.appendVector(encodeStringWithLength(keyPath.string()));
        break;
    case IDBKeyPath::ArrayType: {
        const Vector<String>& array = keyPath.array();
        size_t count = array.size();
        ret.appendVector(encodeVarInt(count));
        for (size_t i = 0; i < count; ++i)
            ret.appendVector(encodeStringWithLength(array[i]));
        break;
    }
    }
    return ret;
}

IDBKeyPath decodeIDBKeyPath(const char* p, const char* limit)
{
    // May be typed, or may be a raw string. An invalid leading
    // byte sequence is used to identify typed coding. New records are
    // always written as typed.
    if (p == limit || (limit - p >= 2 && (*p != IDBKeyPathTypeCodedByte1 || *(p + 1) != IDBKeyPathTypeCodedByte2)))
        return IDBKeyPath(decodeString(p, limit));
    p += 2;

    ASSERT(p != limit);
    IDBKeyPath::Type type = static_cast<IDBKeyPath::Type>(*p++);
    switch (type) {
    case IDBKeyPath::NullType:
        ASSERT(p == limit);
        return IDBKeyPath();
    case IDBKeyPath::StringType: {
        String string;
        p = decodeStringWithLength(p, limit, string);
        ASSERT(p == limit);
        return IDBKeyPath(string);
    }
    case IDBKeyPath::ArrayType: {
        Vector<String> array;
        int64_t count;
        p = decodeVarInt(p, limit, count);
        ASSERT(p);
        ASSERT(count >= 0);
        while (count--) {
            String string;
            p = decodeStringWithLength(p, limit, string);
            ASSERT(p);
            array.append(string);
        }
        ASSERT(p == limit);
        return IDBKeyPath(array);
    }
    }
    ASSERT_NOT_REACHED();
    return IDBKeyPath();
}

namespace {

template<typename KeyType>
int compare(const LevelDBSlice& a, const LevelDBSlice& b, bool, bool& ok)
{
    KeyType keyA;
    KeyType keyB;

    const char* ptrA = KeyType::decode(a.begin(), a.end(), &keyA);
    ASSERT(ptrA);
    if (!ptrA) {
        ok = false;
        return 0;
    }
    const char* ptrB = KeyType::decode(b.begin(), b.end(), &keyB);
    ASSERT(ptrB);
    if (!ptrB) {
        ok = false;
        return 0;
    }

    ok = true;
    return keyA.compare(keyB);
}

template<>
int compare<ExistsEntryKey>(const LevelDBSlice& a, const LevelDBSlice& b, bool, bool& ok)
{
    KeyPrefix prefixA;
    KeyPrefix prefixB;
    const char* ptrA = KeyPrefix::decode(a.begin(), a.end(), &prefixA);
    const char* ptrB = KeyPrefix::decode(b.begin(), b.end(), &prefixB);
    ASSERT(ptrA);
    ASSERT(ptrB);
    ASSERT(prefixA.m_databaseId);
    ASSERT(prefixA.m_objectStoreId);
    ASSERT(prefixA.m_indexId == ExistsEntryKey::SpecialIndexNumber);
    ASSERT(prefixB.m_databaseId);
    ASSERT(prefixB.m_objectStoreId);
    ASSERT(prefixB.m_indexId == ExistsEntryKey::SpecialIndexNumber);
    ASSERT(ptrA != a.end());
    ASSERT(ptrB != b.end());
    // Prefixes are not compared - it is assumed this was already done.
    ASSERT(!prefixA.compare(prefixB));

    return compareEncodedIDBKeys(ptrA, a.end(), ptrB, b.end(), ok);
}

template<>
int compare<ObjectStoreDataKey>(const LevelDBSlice& a, const LevelDBSlice& b, bool, bool& ok)
{
    KeyPrefix prefixA;
    KeyPrefix prefixB;
    const char* ptrA = KeyPrefix::decode(a.begin(), a.end(), &prefixA);
    const char* ptrB = KeyPrefix::decode(b.begin(), b.end(), &prefixB);
    ASSERT(ptrA);
    ASSERT(ptrB);
    ASSERT(prefixA.m_databaseId);
    ASSERT(prefixA.m_objectStoreId);
    ASSERT(prefixA.m_indexId == ObjectStoreDataKey::SpecialIndexNumber);
    ASSERT(prefixB.m_databaseId);
    ASSERT(prefixB.m_objectStoreId);
    ASSERT(prefixB.m_indexId == ObjectStoreDataKey::SpecialIndexNumber);
    ASSERT(ptrA != a.end());
    ASSERT(ptrB != b.end());
    // Prefixes are not compared - it is assumed this was already done.
    ASSERT(!prefixA.compare(prefixB));

    return compareEncodedIDBKeys(ptrA, a.end(), ptrB, b.end(), ok);
}

template<>
int compare<IndexDataKey>(const LevelDBSlice& a, const LevelDBSlice& b, bool ignoreDuplicates, bool& ok)
{
    KeyPrefix prefixA;
    KeyPrefix prefixB;
    const char* ptrA = KeyPrefix::decode(a.begin(), a.end(), &prefixA);
    const char* ptrB = KeyPrefix::decode(b.begin(), b.end(), &prefixB);
    ASSERT(ptrA);
    ASSERT(ptrB);
    ASSERT(prefixA.m_databaseId);
    ASSERT(prefixA.m_objectStoreId);
    ASSERT(prefixA.m_indexId >= MinimumIndexId);
    ASSERT(prefixB.m_databaseId);
    ASSERT(prefixB.m_objectStoreId);
    ASSERT(prefixB.m_indexId >= MinimumIndexId);
    ASSERT(ptrA != a.end());
    ASSERT(ptrB != b.end());
    // Prefixes are not compared - it is assumed this was already done.
    ASSERT(!prefixA.compare(prefixB));

    // index key
    int result = compareEncodedIDBKeys(ptrA, a.end(), ptrB, b.end(), ok);
    if (!ok || result)
        return result;
    if (ignoreDuplicates)
        return 0;

    // sequence number [optional]
    int64_t sequenceNumberA = -1;
    int64_t sequenceNumberB = -1;
    if (ptrA != a.end())
        ptrA = decodeVarInt(ptrA, a.end(), sequenceNumberA);
    if (ptrB != b.end())
        ptrB = decodeVarInt(ptrB, b.end(), sequenceNumberB);

    // primary key [optional]
    if (!ptrA || !ptrB)
        return 0;
    if (ptrA == a.end() && ptrB == b.end())
        return 0;
    if (ptrA == a.end())
        return -1;
    if (ptrB == b.end())
        return 1;

    result = compareEncodedIDBKeys(ptrA, a.end(), ptrB, b.end(), ok);
    if (!ok || result)
        return result;

    return compareInts(sequenceNumberA, sequenceNumberB);
}

int compare(const LevelDBSlice& a, const LevelDBSlice& b, bool indexKeys, bool& ok)
{
    const char* ptrA = a.begin();
    const char* ptrB = b.begin();
    const char* endA = a.end();
    const char* endB = b.end();

    KeyPrefix prefixA;
    KeyPrefix prefixB;

    ptrA = KeyPrefix::decode(ptrA, endA, &prefixA);
    ptrB = KeyPrefix::decode(ptrB, endB, &prefixB);
    ASSERT(ptrA);
    ASSERT(ptrB);
    if (!ptrA || !ptrB) {
        ok = false;
        return 0;
    }

    ok = true;
    if (int x = prefixA.compare(prefixB))
        return x;

    if (prefixA.type() == KeyPrefix::GlobalMetaData) {
        ASSERT(ptrA != endA);
        ASSERT(ptrB != endB);

        unsigned char typeByteA = *ptrA++;
        unsigned char typeByteB = *ptrB++;

        if (int x = typeByteA - typeByteB)
            return x;
        if (typeByteA < MaxSimpleGlobalMetaDataTypeByte)
            return 0;

        const bool ignoreDuplicates = false;
        if (typeByteA == DatabaseFreeListTypeByte)
            return compare<DatabaseFreeListKey>(a, b, ignoreDuplicates, ok);
        if (typeByteA == DatabaseNameTypeByte)
            return compare<DatabaseNameKey>(a, b, ignoreDuplicates, ok);
    }

    if (prefixA.type() == KeyPrefix::DatabaseMetaData) {
        ASSERT(ptrA != endA);
        ASSERT(ptrB != endB);

        unsigned char typeByteA = *ptrA++;
        unsigned char typeByteB = *ptrB++;

        if (int x = typeByteA - typeByteB)
            return x;
        if (typeByteA < DatabaseMetaDataKey::MaxSimpleMetaDataType)
            return 0;

        const bool ignoreDuplicates = false;
        if (typeByteA == ObjectStoreMetaDataTypeByte)
            return compare<ObjectStoreMetaDataKey>(a, b, ignoreDuplicates, ok);
        if (typeByteA == IndexMetaDataTypeByte)
            return compare<IndexMetaDataKey>(a, b, ignoreDuplicates, ok);
        if (typeByteA == ObjectStoreFreeListTypeByte)
            return compare<ObjectStoreFreeListKey>(a, b, ignoreDuplicates, ok);
        if (typeByteA == IndexFreeListTypeByte)
            return compare<IndexFreeListKey>(a, b, ignoreDuplicates, ok);
        if (typeByteA == ObjectStoreNamesTypeByte)
            return compare<ObjectStoreNamesKey>(a, b, ignoreDuplicates, ok);
        if (typeByteA == IndexNamesKeyTypeByte)
            return compare<IndexNamesKey>(a, b, ignoreDuplicates, ok);
    }

    if (prefixA.type() == KeyPrefix::ObjectStoreData) {
        if (ptrA == endA && ptrB == endB)
            return 0;
        if (ptrA == endA)
            return -1;
        if (ptrB == endB)
            return 1; // FIXME: This case of non-existing user keys should not have to be handled this way.

        const bool ignoreDuplicates = false;
        return compare<ObjectStoreDataKey>(a, b, ignoreDuplicates, ok);
    }
    if (prefixA.type() == KeyPrefix::ExistsEntry) {
        if (ptrA == endA && ptrB == endB)
            return 0;
        if (ptrA == endA)
            return -1;
        if (ptrB == endB)
            return 1; // FIXME: This case of non-existing user keys should not have to be handled this way.

        const bool ignoreDuplicates = false;
        return compare<ExistsEntryKey>(a, b, ignoreDuplicates, ok);
    }
    if (prefixA.type() == KeyPrefix::IndexData) {
        if (ptrA == endA && ptrB == endB)
            return 0;
        if (ptrA == endA)
            return -1;
        if (ptrB == endB)
            return 1; // FIXME: This case of non-existing user keys should not have to be handled this way.

        bool ignoreDuplicates = indexKeys;
        return compare<IndexDataKey>(a, b, ignoreDuplicates, ok);
    }

    ASSERT_NOT_REACHED();
    ok = false;
    return 0;
}

}

int compare(const LevelDBSlice& a, const LevelDBSlice& b, bool indexKeys)
{
    bool ok;
    int result = compare(a, b, indexKeys, ok);
    ASSERT(ok);
    if (!ok)
        return 0;
    return result;
}

KeyPrefix::KeyPrefix()
    : m_databaseId(InvalidType)
    , m_objectStoreId(InvalidType)
    , m_indexId(InvalidType)
{
}

KeyPrefix::KeyPrefix(int64_t databaseId)
    : m_databaseId(databaseId)
    , m_objectStoreId(0)
    , m_indexId(0)
{
    ASSERT(KeyPrefix::isValidDatabaseId(databaseId));
}

KeyPrefix::KeyPrefix(int64_t databaseId, int64_t objectStoreId)
    : m_databaseId(databaseId)
    , m_objectStoreId(objectStoreId)
    , m_indexId(0)
{
    ASSERT(KeyPrefix::isValidDatabaseId(databaseId));
    ASSERT(KeyPrefix::isValidObjectStoreId(objectStoreId));
}

KeyPrefix::KeyPrefix(int64_t databaseId, int64_t objectStoreId, int64_t indexId)
    : m_databaseId(databaseId)
    , m_objectStoreId(objectStoreId)
    , m_indexId(indexId)
{
    ASSERT(KeyPrefix::isValidDatabaseId(databaseId));
    ASSERT(KeyPrefix::isValidObjectStoreId(objectStoreId));
    ASSERT(KeyPrefix::isValidIndexId(indexId));
}

KeyPrefix::KeyPrefix(Type type, int64_t databaseId, int64_t objectStoreId, int64_t indexId)
    : m_databaseId(databaseId)
    , m_objectStoreId(objectStoreId)
    , m_indexId(indexId)
{
    ASSERT_UNUSED(type, type == InvalidType);
    ASSERT(KeyPrefix::isValidDatabaseId(databaseId));
    ASSERT(KeyPrefix::isValidObjectStoreId(objectStoreId));
}


KeyPrefix KeyPrefix::createWithSpecialIndex(int64_t databaseId, int64_t objectStoreId, int64_t indexId)
{
    ASSERT(KeyPrefix::isValidDatabaseId(databaseId));
    ASSERT(KeyPrefix::isValidObjectStoreId(objectStoreId));
    ASSERT(indexId);
    return KeyPrefix(InvalidType, databaseId, objectStoreId, indexId);
}


bool KeyPrefix::isValidDatabaseId(int64_t databaseId)
{
    return (databaseId > 0) && (databaseId < KeyPrefix::kMaxDatabaseId);
}

bool KeyPrefix::isValidObjectStoreId(int64_t objectStoreId)
{
    return (objectStoreId > 0) && (objectStoreId < KeyPrefix::kMaxObjectStoreId);
}

bool KeyPrefix::isValidIndexId(int64_t indexId)
{
    return (indexId >= MinimumIndexId) && (indexId < KeyPrefix::kMaxIndexId);
}

const char* KeyPrefix::decode(const char* start, const char* limit, KeyPrefix* result)
{
    if (start == limit)
        return 0;

    unsigned char firstByte = *start++;

    int databaseIdBytes = ((firstByte >> 5) & 0x7) + 1;
    int objectStoreIdBytes = ((firstByte >> 2) & 0x7) + 1;
    int indexIdBytes = (firstByte & 0x3) + 1;

    if (start + databaseIdBytes + objectStoreIdBytes + indexIdBytes > limit)
        return 0;

    result->m_databaseId = decodeInt(start, start + databaseIdBytes);
    start += databaseIdBytes;
    result->m_objectStoreId = decodeInt(start, start + objectStoreIdBytes);
    start += objectStoreIdBytes;
    result->m_indexId = decodeInt(start, start + indexIdBytes);
    start += indexIdBytes;

    return start;
}

Vector<char> KeyPrefix::encodeEmpty()
{
    const Vector<char, 4> result(4, 0);
    ASSERT(encodeInternal(0, 0, 0) == Vector<char>(4, 0));
    return result;
}

Vector<char> KeyPrefix::encode() const
{
    ASSERT(m_databaseId != InvalidId);
    ASSERT(m_objectStoreId != InvalidId);
    ASSERT(m_indexId != InvalidId);
    return encodeInternal(m_databaseId, m_objectStoreId, m_indexId);
}

Vector<char> KeyPrefix::encodeInternal(int64_t databaseId, int64_t objectStoreId, int64_t indexId)
{
    Vector<char> databaseIdString = encodeIntSafely(databaseId, kMaxDatabaseId);
    Vector<char> objectStoreIdString = encodeIntSafely(objectStoreId, kMaxObjectStoreId);
    Vector<char> indexIdString = encodeIntSafely(indexId, kMaxIndexId);

    ASSERT(databaseIdString.size() <= kMaxDatabaseIdSizeBytes);
    ASSERT(objectStoreIdString.size() <= kMaxObjectStoreIdSizeBytes);
    ASSERT(indexIdString.size() <= kMaxIndexIdSizeBytes);

    unsigned char firstByte = (databaseIdString.size() - 1) << (kMaxObjectStoreIdSizeBits + kMaxIndexIdSizeBits) | (objectStoreIdString.size() - 1) << kMaxIndexIdSizeBits | (indexIdString.size() - 1);
    COMPILE_ASSERT(kMaxDatabaseIdSizeBits + kMaxObjectStoreIdSizeBits + kMaxIndexIdSizeBits == sizeof(firstByte) * 8, CANT_ENCODE_IDS);
    Vector<char, DefaultInlineBufferSize> ret;
    ret.append(firstByte);
    ret.appendVector(databaseIdString);
    ret.appendVector(objectStoreIdString);
    ret.appendVector(indexIdString);

    ASSERT(ret.size() <= DefaultInlineBufferSize);
    return ret;
}

int KeyPrefix::compare(const KeyPrefix& other) const
{
    ASSERT(m_databaseId != InvalidId);
    ASSERT(m_objectStoreId != InvalidId);
    ASSERT(m_indexId != InvalidId);

    if (m_databaseId != other.m_databaseId)
        return compareInts(m_databaseId, other.m_databaseId);
    if (m_objectStoreId != other.m_objectStoreId)
        return compareInts(m_objectStoreId, other.m_objectStoreId);
    if (m_indexId != other.m_indexId)
        return compareInts(m_indexId, other.m_indexId);
    return 0;
}

KeyPrefix::Type KeyPrefix::type() const
{
    ASSERT(m_databaseId != InvalidId);
    ASSERT(m_objectStoreId != InvalidId);
    ASSERT(m_indexId != InvalidId);

    if (!m_databaseId)
        return GlobalMetaData;
    if (!m_objectStoreId)
        return DatabaseMetaData;
    if (m_indexId == ObjectStoreDataIndexId)
        return ObjectStoreData;
    if (m_indexId == ExistsEntryIndexId)
        return ExistsEntry;
    if (m_indexId >= MinimumIndexId)
        return IndexData;

    ASSERT_NOT_REACHED();
    return InvalidType;
}

Vector<char> SchemaVersionKey::encode()
{
    Vector<char> ret = KeyPrefix::encodeEmpty();
    ret.appendVector(encodeByte(SchemaVersionTypeByte));
    return ret;
}

Vector<char> MaxDatabaseIdKey::encode()
{
    Vector<char> ret = KeyPrefix::encodeEmpty();
    ret.appendVector(encodeByte(MaxDatabaseIdTypeByte));
    return ret;
}

Vector<char> DataVersionKey::encode()
{
    Vector<char> ret = KeyPrefix::encodeEmpty();
    ret.appendVector(encodeByte(DataVersionTypeByte));
    return ret;
}

DatabaseFreeListKey::DatabaseFreeListKey()
    : m_databaseId(-1)
{
}

const char* DatabaseFreeListKey::decode(const char* start, const char* limit, DatabaseFreeListKey* result)
{
    KeyPrefix prefix;
    const char* p = KeyPrefix::decode(start, limit, &prefix);
    if (!p)
        return 0;
    ASSERT(!prefix.m_databaseId);
    ASSERT(!prefix.m_objectStoreId);
    ASSERT(!prefix.m_indexId);
    if (p == limit)
        return 0;
    unsigned char typeByte = 0;
    p = decodeByte(p, limit, typeByte);
    ASSERT_UNUSED(typeByte, typeByte == DatabaseFreeListTypeByte);
    if (p == limit)
        return 0;
    return decodeVarInt(p, limit, result->m_databaseId);
}

Vector<char> DatabaseFreeListKey::encode(int64_t databaseId)
{
    Vector<char> ret = KeyPrefix::encodeEmpty();
    ret.appendVector(encodeByte(DatabaseFreeListTypeByte));
    ret.appendVector(encodeVarInt(databaseId));
    return ret;
}

Vector<char> DatabaseFreeListKey::encodeMaxKey()
{
    return encode(INT64_MAX);
}

int64_t DatabaseFreeListKey::databaseId() const
{
    ASSERT(m_databaseId >= 0);
    return m_databaseId;
}

int DatabaseFreeListKey::compare(const DatabaseFreeListKey& other) const
{
    ASSERT(m_databaseId >= 0);
    return compareInts(m_databaseId, other.m_databaseId);
}

const char* DatabaseNameKey::decode(const char* start, const char* limit, DatabaseNameKey* result)
{
    KeyPrefix prefix;
    const char* p = KeyPrefix::decode(start, limit, &prefix);
    if (!p)
        return p;
    ASSERT(!prefix.m_databaseId);
    ASSERT(!prefix.m_objectStoreId);
    ASSERT(!prefix.m_indexId);
    if (p == limit)
        return 0;
    unsigned char typeByte = 0;
    p = decodeByte(p, limit, typeByte);
    ASSERT_UNUSED(typeByte, typeByte == DatabaseNameTypeByte);
    if (p == limit)
        return 0;
    p = decodeStringWithLength(p, limit, result->m_origin);
    if (!p)
        return 0;
    return decodeStringWithLength(p, limit, result->m_databaseName);
}

Vector<char> DatabaseNameKey::encode(const String& origin, const String& databaseName)
{
    Vector<char> ret = KeyPrefix::encodeEmpty();
    ret.appendVector(encodeByte(DatabaseNameTypeByte));
    ret.appendVector(encodeStringWithLength(origin));
    ret.appendVector(encodeStringWithLength(databaseName));
    return ret;
}

Vector<char> DatabaseNameKey::encodeMinKeyForOrigin(const String& origin)
{
    return encode(origin, "");
}

Vector<char> DatabaseNameKey::encodeStopKeyForOrigin(const String& origin)
{
    // just after origin in collation order
    return encodeMinKeyForOrigin(origin + "\x01");
}

int DatabaseNameKey::compare(const DatabaseNameKey& other)
{
    if (int x = codePointCompare(m_origin, other.m_origin))
        return x;
    return codePointCompare(m_databaseName, other.m_databaseName);
}

Vector<char> DatabaseMetaDataKey::encode(int64_t databaseId, MetaDataType metaDataType)
{
    KeyPrefix prefix(databaseId);
    Vector<char> ret = prefix.encode();
    ret.appendVector(encodeByte(metaDataType));
    return ret;
}

ObjectStoreMetaDataKey::ObjectStoreMetaDataKey()
    : m_objectStoreId(-1)
    , m_metaDataType(-1)
{
}

const char* ObjectStoreMetaDataKey::decode(const char* start, const char* limit, ObjectStoreMetaDataKey* result)
{
    KeyPrefix prefix;
    const char* p = KeyPrefix::decode(start, limit, &prefix);
    if (!p)
        return 0;
    ASSERT(prefix.m_databaseId);
    ASSERT(!prefix.m_objectStoreId);
    ASSERT(!prefix.m_indexId);
    if (p == limit)
        return 0;
    unsigned char typeByte = 0;
    p = decodeByte(p, limit, typeByte);
    ASSERT_UNUSED(typeByte, typeByte == ObjectStoreMetaDataTypeByte);
    if (p == limit)
        return 0;
    p = decodeVarInt(p, limit, result->m_objectStoreId);
    if (!p)
        return 0;
    ASSERT(result->m_objectStoreId);
    if (p == limit)
        return 0;
    return decodeByte(p, limit, result->m_metaDataType);
}

Vector<char> ObjectStoreMetaDataKey::encode(int64_t databaseId, int64_t objectStoreId, unsigned char metaDataType)
{
    KeyPrefix prefix(databaseId);
    Vector<char> ret = prefix.encode();
    ret.appendVector(encodeByte(ObjectStoreMetaDataTypeByte));
    ret.appendVector(encodeVarInt(objectStoreId));
    ret.appendVector(encodeByte(metaDataType));
    return ret;
}

Vector<char> ObjectStoreMetaDataKey::encodeMaxKey(int64_t databaseId)
{
    return encode(databaseId, INT64_MAX, ObjectMetaDataTypeMaximum);
}

Vector<char> ObjectStoreMetaDataKey::encodeMaxKey(int64_t databaseId, int64_t objectStoreId)
{
    return encode(databaseId, objectStoreId, ObjectMetaDataTypeMaximum);
}

int64_t ObjectStoreMetaDataKey::objectStoreId() const
{
    ASSERT(m_objectStoreId >= 0);
    return m_objectStoreId;
}
unsigned char ObjectStoreMetaDataKey::metaDataType() const
{
    return m_metaDataType;
}

int ObjectStoreMetaDataKey::compare(const ObjectStoreMetaDataKey& other)
{
    ASSERT(m_objectStoreId >= 0);
    if (int x = compareInts(m_objectStoreId, other.m_objectStoreId))
        return x;
    int64_t result = m_metaDataType - other.m_metaDataType;
    if (result < 0)
        return -1;
    return (result > 0) ? 1 : result;
}

IndexMetaDataKey::IndexMetaDataKey()
    : m_objectStoreId(-1)
    , m_indexId(-1)
    , m_metaDataType(0)
{
}

const char* IndexMetaDataKey::decode(const char* start, const char* limit, IndexMetaDataKey* result)
{
    KeyPrefix prefix;
    const char* p = KeyPrefix::decode(start, limit, &prefix);
    if (!p)
        return 0;
    ASSERT(prefix.m_databaseId);
    ASSERT(!prefix.m_objectStoreId);
    ASSERT(!prefix.m_indexId);
    if (p == limit)
        return 0;
    unsigned char typeByte = 0;
    p = decodeByte(p, limit, typeByte);
    ASSERT_UNUSED(typeByte, typeByte == IndexMetaDataTypeByte);
    if (p == limit)
        return 0;
    p = decodeVarInt(p, limit, result->m_objectStoreId);
    if (!p)
        return 0;
    p = decodeVarInt(p, limit, result->m_indexId);
    if (!p)
        return 0;
    if (p == limit)
        return 0;
    return decodeByte(p, limit, result->m_metaDataType);
}

Vector<char> IndexMetaDataKey::encode(int64_t databaseId, int64_t objectStoreId, int64_t indexId, unsigned char metaDataType)
{
    KeyPrefix prefix(databaseId);
    Vector<char> ret = prefix.encode();
    ret.appendVector(encodeByte(IndexMetaDataTypeByte));
    ret.appendVector(encodeVarInt(objectStoreId));
    ret.appendVector(encodeVarInt(indexId));
    ret.appendVector(encodeByte(metaDataType));
    return ret;
}

Vector<char> IndexMetaDataKey::encodeMaxKey(int64_t databaseId, int64_t objectStoreId)
{
    return encode(databaseId, objectStoreId, INT64_MAX, IndexMetaDataTypeMaximum);
}

Vector<char> IndexMetaDataKey::encodeMaxKey(int64_t databaseId, int64_t objectStoreId, int64_t indexId)
{
    return encode(databaseId, objectStoreId, indexId, IndexMetaDataTypeMaximum);
}

int IndexMetaDataKey::compare(const IndexMetaDataKey& other)
{
    ASSERT(m_objectStoreId >= 0);
    ASSERT(m_indexId >= 0);

    if (int x = compareInts(m_objectStoreId, other.m_objectStoreId))
        return x;
    if (int x = compareInts(m_indexId, other.m_indexId))
        return x;
    return m_metaDataType - other.m_metaDataType;
}

int64_t IndexMetaDataKey::indexId() const
{
    ASSERT(m_indexId >= 0);
    return m_indexId;
}

ObjectStoreFreeListKey::ObjectStoreFreeListKey()
    : m_objectStoreId(-1)
{
}

const char* ObjectStoreFreeListKey::decode(const char* start, const char* limit, ObjectStoreFreeListKey* result)
{
    KeyPrefix prefix;
    const char* p = KeyPrefix::decode(start, limit, &prefix);
    if (!p)
        return 0;
    ASSERT(prefix.m_databaseId);
    ASSERT(!prefix.m_objectStoreId);
    ASSERT(!prefix.m_indexId);
    if (p == limit)
        return 0;
    unsigned char typeByte = 0;
    p = decodeByte(p, limit, typeByte);
    ASSERT_UNUSED(typeByte, typeByte == ObjectStoreFreeListTypeByte);
    if (p == limit)
        return 0;
    return decodeVarInt(p, limit, result->m_objectStoreId);
}

Vector<char> ObjectStoreFreeListKey::encode(int64_t databaseId, int64_t objectStoreId)
{
    KeyPrefix prefix(databaseId);
    Vector<char> ret = prefix.encode();
    ret.appendVector(encodeByte(ObjectStoreFreeListTypeByte));
    ret.appendVector(encodeVarInt(objectStoreId));
    return ret;
}

Vector<char> ObjectStoreFreeListKey::encodeMaxKey(int64_t databaseId)
{
    return encode(databaseId, INT64_MAX);
}

int64_t ObjectStoreFreeListKey::objectStoreId() const
{
    ASSERT(m_objectStoreId >= 0);
    return m_objectStoreId;
}

int ObjectStoreFreeListKey::compare(const ObjectStoreFreeListKey& other)
{
    // FIXME: It may seem strange that we're not comparing database id's,
    // but that comparison will have been made earlier.
    // We should probably make this more clear, though...
    ASSERT(m_objectStoreId >= 0);
    return compareInts(m_objectStoreId, other.m_objectStoreId);
}

IndexFreeListKey::IndexFreeListKey()
    : m_objectStoreId(-1)
    , m_indexId(-1)
{
}

const char* IndexFreeListKey::decode(const char* start, const char* limit, IndexFreeListKey* result)
{
    KeyPrefix prefix;
    const char* p = KeyPrefix::decode(start, limit, &prefix);
    if (!p)
        return 0;
    ASSERT(prefix.m_databaseId);
    ASSERT(!prefix.m_objectStoreId);
    ASSERT(!prefix.m_indexId);
    if (p == limit)
        return 0;
    unsigned char typeByte = 0;
    p = decodeByte(p, limit, typeByte);
    ASSERT_UNUSED(typeByte, typeByte == IndexFreeListTypeByte);
    if (p == limit)
        return 0;
    p = decodeVarInt(p, limit, result->m_objectStoreId);
    if (!p)
        return 0;
    return decodeVarInt(p, limit, result->m_indexId);
}

Vector<char> IndexFreeListKey::encode(int64_t databaseId, int64_t objectStoreId, int64_t indexId)
{
    KeyPrefix prefix(databaseId);
    Vector<char> ret = prefix.encode();
    ret.appendVector(encodeByte(IndexFreeListTypeByte));
    ret.appendVector(encodeVarInt(objectStoreId));
    ret.appendVector(encodeVarInt(indexId));
    return ret;
}

Vector<char> IndexFreeListKey::encodeMaxKey(int64_t databaseId, int64_t objectStoreId)
{
    return encode(databaseId, objectStoreId, INT64_MAX);
}

int IndexFreeListKey::compare(const IndexFreeListKey& other)
{
    ASSERT(m_objectStoreId >= 0);
    ASSERT(m_indexId >= 0);
    if (int x = compareInts(m_objectStoreId, other.m_objectStoreId))
        return x;
    return compareInts(m_indexId, other.m_indexId);
}

int64_t IndexFreeListKey::objectStoreId() const
{
    ASSERT(m_objectStoreId >= 0);
    return m_objectStoreId;
}

int64_t IndexFreeListKey::indexId() const
{
    ASSERT(m_indexId >= 0);
    return m_indexId;
}

// FIXME: We never use this to look up object store ids, because a mapping
// is kept in the IDBDatabaseBackendImpl. Can the mapping become unreliable?
// Can we remove this?
const char* ObjectStoreNamesKey::decode(const char* start, const char* limit, ObjectStoreNamesKey* result)
{
    KeyPrefix prefix;
    const char* p = KeyPrefix::decode(start, limit, &prefix);
    if (!p)
        return 0;
    ASSERT(prefix.m_databaseId);
    ASSERT(!prefix.m_objectStoreId);
    ASSERT(!prefix.m_indexId);
    if (p == limit)
        return 0;
    unsigned char typeByte = 0;
    p = decodeByte(p, limit, typeByte);
    ASSERT_UNUSED(typeByte, typeByte == ObjectStoreNamesTypeByte);
    return decodeStringWithLength(p, limit, result->m_objectStoreName);
}

Vector<char> ObjectStoreNamesKey::encode(int64_t databaseId, const String& objectStoreName)
{
    KeyPrefix prefix(databaseId);
    Vector<char> ret = prefix.encode();
    ret.appendVector(encodeByte(ObjectStoreNamesTypeByte));
    ret.appendVector(encodeStringWithLength(objectStoreName));
    return ret;
}

int ObjectStoreNamesKey::compare(const ObjectStoreNamesKey& other)
{
    return codePointCompare(m_objectStoreName, other.m_objectStoreName);
}

IndexNamesKey::IndexNamesKey()
    : m_objectStoreId(-1)
{
}

// FIXME: We never use this to look up index ids, because a mapping
// is kept at a higher level.
const char* IndexNamesKey::decode(const char* start, const char* limit, IndexNamesKey* result)
{
    KeyPrefix prefix;
    const char* p = KeyPrefix::decode(start, limit, &prefix);
    if (!p)
        return 0;
    ASSERT(prefix.m_databaseId);
    ASSERT(!prefix.m_objectStoreId);
    ASSERT(!prefix.m_indexId);
    if (p == limit)
        return 0;
    unsigned char typeByte = 0;
    p = decodeByte(p, limit, typeByte);
    ASSERT_UNUSED(typeByte, typeByte == IndexNamesKeyTypeByte);
    if (p == limit)
        return 0;
    p = decodeVarInt(p, limit, result->m_objectStoreId);
    if (!p)
        return 0;
    return decodeStringWithLength(p, limit, result->m_indexName);
}

Vector<char> IndexNamesKey::encode(int64_t databaseId, int64_t objectStoreId, const String& indexName)
{
    KeyPrefix prefix(databaseId);
    Vector<char> ret = prefix.encode();
    ret.appendVector(encodeByte(IndexNamesKeyTypeByte));
    ret.appendVector(encodeVarInt(objectStoreId));
    ret.appendVector(encodeStringWithLength(indexName));
    return ret;
}

int IndexNamesKey::compare(const IndexNamesKey& other)
{
    ASSERT(m_objectStoreId >= 0);
    if (int x = compareInts(m_objectStoreId, other.m_objectStoreId))
        return x;
    return codePointCompare(m_indexName, other.m_indexName);
}

const char* ObjectStoreDataKey::decode(const char* start, const char* end, ObjectStoreDataKey* result)
{
    KeyPrefix prefix;
    const char* p = KeyPrefix::decode(start, end, &prefix);
    if (!p)
        return 0;
    ASSERT(prefix.m_databaseId);
    ASSERT(prefix.m_objectStoreId);
    ASSERT(prefix.m_indexId == SpecialIndexNumber);
    if (p == end)
        return 0;
    return extractEncodedIDBKey(p, end, &result->m_encodedUserKey);
}

Vector<char> ObjectStoreDataKey::encode(int64_t databaseId, int64_t objectStoreId, const Vector<char> encodedUserKey)
{
    KeyPrefix prefix(KeyPrefix::createWithSpecialIndex(databaseId, objectStoreId, SpecialIndexNumber));
    Vector<char> ret = prefix.encode();
    ret.appendVector(encodedUserKey);

    return ret;
}

Vector<char> ObjectStoreDataKey::encode(int64_t databaseId, int64_t objectStoreId, const IDBKey& userKey)
{
    return encode(databaseId, objectStoreId, encodeIDBKey(userKey));
}

int ObjectStoreDataKey::compare(const ObjectStoreDataKey& other, bool& ok)
{
    return compareEncodedIDBKeys(m_encodedUserKey, other.m_encodedUserKey, ok);
}

PassRefPtr<IDBKey> ObjectStoreDataKey::userKey() const
{
    RefPtr<IDBKey> key;
    decodeIDBKey(m_encodedUserKey.begin(), m_encodedUserKey.end(), key);
    return key;
}

const int64_t ObjectStoreDataKey::SpecialIndexNumber = ObjectStoreDataIndexId;

const char* ExistsEntryKey::decode(const char* start, const char* end, ExistsEntryKey* result)
{
    KeyPrefix prefix;
    const char* p = KeyPrefix::decode(start, end, &prefix);
    if (!p)
        return 0;
    ASSERT(prefix.m_databaseId);
    ASSERT(prefix.m_objectStoreId);
    ASSERT(prefix.m_indexId == SpecialIndexNumber);
    if (p == end)
        return 0;
    return extractEncodedIDBKey(p, end, &result->m_encodedUserKey);
}

Vector<char> ExistsEntryKey::encode(int64_t databaseId, int64_t objectStoreId, const Vector<char>& encodedKey)
{
    KeyPrefix prefix(KeyPrefix::createWithSpecialIndex(databaseId, objectStoreId, SpecialIndexNumber));
    Vector<char> ret = prefix.encode();
    ret.appendVector(encodedKey);
    return ret;
}

Vector<char> ExistsEntryKey::encode(int64_t databaseId, int64_t objectStoreId, const IDBKey& userKey)
{
    return encode(databaseId, objectStoreId, encodeIDBKey(userKey));
}

int ExistsEntryKey::compare(const ExistsEntryKey& other, bool& ok)
{
    return compareEncodedIDBKeys(m_encodedUserKey, other.m_encodedUserKey, ok);
}

PassRefPtr<IDBKey> ExistsEntryKey::userKey() const
{
    RefPtr<IDBKey> key;
    decodeIDBKey(m_encodedUserKey.begin(), m_encodedUserKey.end(), key);
    return key;
}

const int64_t ExistsEntryKey::SpecialIndexNumber = ExistsEntryIndexId;

IndexDataKey::IndexDataKey()
    : m_databaseId(-1)
    , m_objectStoreId(-1)
    , m_indexId(-1)
    , m_sequenceNumber(-1)
{
}

const char* IndexDataKey::decode(const char* start, const char* limit, IndexDataKey* result)
{
    KeyPrefix prefix;
    const char* p = KeyPrefix::decode(start, limit, &prefix);
    if (!p)
        return 0;
    ASSERT(prefix.m_databaseId);
    ASSERT(prefix.m_objectStoreId);
    ASSERT(prefix.m_indexId >= MinimumIndexId);
    result->m_databaseId = prefix.m_databaseId;
    result->m_objectStoreId = prefix.m_objectStoreId;
    result->m_indexId = prefix.m_indexId;
    result->m_sequenceNumber = -1;
    result->m_encodedPrimaryKey = minIDBKey();

    p = extractEncodedIDBKey(p, limit, &result->m_encodedUserKey);
    if (!p)
        return 0;

    // [optional] sequence number
    if (p == limit)
        return p;
    p =  decodeVarInt(p, limit, result->m_sequenceNumber);
    if (!p)
        return 0;

    // [optional] primary key
    if (p == limit)
        return p;
    p = extractEncodedIDBKey(p, limit, &result->m_encodedPrimaryKey);
    if (!p)
        return 0;

    return p;
}

Vector<char> IndexDataKey::encode(int64_t databaseId, int64_t objectStoreId, int64_t indexId, const Vector<char>& encodedUserKey, const Vector<char>& encodedPrimaryKey, int64_t sequenceNumber)
{
    KeyPrefix prefix(databaseId, objectStoreId, indexId);
    Vector<char> ret = prefix.encode();
    ret.appendVector(encodedUserKey);
    ret.appendVector(encodeVarInt(sequenceNumber));
    ret.appendVector(encodedPrimaryKey);
    return ret;
}

Vector<char> IndexDataKey::encode(int64_t databaseId, int64_t objectStoreId, int64_t indexId, const IDBKey& userKey)
{
    return encode(databaseId, objectStoreId, indexId, encodeIDBKey(userKey), minIDBKey());
}

Vector<char> IndexDataKey::encodeMinKey(int64_t databaseId, int64_t objectStoreId, int64_t indexId)
{
    return encode(databaseId, objectStoreId, indexId, minIDBKey(), minIDBKey());
}

Vector<char> IndexDataKey::encodeMaxKey(int64_t databaseId, int64_t objectStoreId, int64_t indexId)
{
    return encode(databaseId, objectStoreId, indexId, maxIDBKey(), maxIDBKey(), INT64_MAX);
}

int IndexDataKey::compare(const IndexDataKey& other, bool ignoreDuplicates, bool& ok)
{
    ASSERT(m_databaseId >= 0);
    ASSERT(m_objectStoreId >= 0);
    ASSERT(m_indexId >= 0);
    int result = compareEncodedIDBKeys(m_encodedUserKey, other.m_encodedUserKey, ok);
    if (!ok || result)
        return result;
    if (ignoreDuplicates)
        return 0;
    result = compareEncodedIDBKeys(m_encodedPrimaryKey, other.m_encodedPrimaryKey, ok);
    if (!ok || result)
        return result;
    return compareInts(m_sequenceNumber, other.m_sequenceNumber);
}

int64_t IndexDataKey::databaseId() const
{
    ASSERT(m_databaseId >= 0);
    return m_databaseId;
}

int64_t IndexDataKey::objectStoreId() const
{
    ASSERT(m_objectStoreId >= 0);
    return m_objectStoreId;
}

int64_t IndexDataKey::indexId() const
{
    ASSERT(m_indexId >= 0);
    return m_indexId;
}

PassRefPtr<IDBKey> IndexDataKey::userKey() const
{
    RefPtr<IDBKey> key;
    decodeIDBKey(m_encodedUserKey.begin(), m_encodedUserKey.end(), key);
    return key;
}

PassRefPtr<IDBKey> IndexDataKey::primaryKey() const
{
    RefPtr<IDBKey> key;
    decodeIDBKey(m_encodedPrimaryKey.begin(), m_encodedPrimaryKey.end(), key);
    return key;
}

} // namespace IDBLevelDBCoding
} // namespace WebCore

#endif // USE(LEVELDB)
#endif // ENABLE(INDEXED_DATABASE)
