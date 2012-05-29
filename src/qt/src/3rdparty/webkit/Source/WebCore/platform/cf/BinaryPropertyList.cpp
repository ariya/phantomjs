/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "BinaryPropertyList.h"

#include <wtf/HashMap.h>
#include <wtf/HashSet.h>
#include <wtf/text/StringHash.h>
#include <limits>

using namespace std;

namespace WebCore {

static const size_t headerSize = 8;
static const size_t trailerSize = 32;

static const UInt8 booleanTrueMarkerByte = 0x09;
static const UInt8 oneByteIntegerMarkerByte = 0x10;
static const UInt8 twoByteIntegerMarkerByte = 0x11;
static const UInt8 fourByteIntegerMarkerByte = 0x12;
static const UInt8 eightByteIntegerMarkerByte = 0x13;
static const UInt8 asciiStringMarkerByte = 0x50;
static const UInt8 asciiStringWithSeparateLengthMarkerByte = 0x5F;
static const UInt8 unicodeStringMarkerByte = 0x60;
static const UInt8 unicodeStringWithSeparateLengthMarkerByte = 0x6F;
static const UInt8 arrayMarkerByte = 0xA0;
static const UInt8 arrayWithSeparateLengthMarkerByte = 0xAF;
static const UInt8 dictionaryMarkerByte = 0xD0;
static const UInt8 dictionaryWithSeparateLengthMarkerByte = 0xDF;
static const size_t maxLengthInMarkerByte = 0xE;

class IntegerArray {
public:
    IntegerArray() : m_integers(0), m_size(0) { }
    IntegerArray(const int* integers, size_t size) : m_integers(integers), m_size(size) { ASSERT(integers); ASSERT(size); }

    void markDeleted() { m_integers = 0; m_size = deletedValueSize(); }
    bool isDeletedValue() const { return m_size == deletedValueSize(); }

    const int* integers() const { ASSERT(!isDeletedValue()); return m_integers; }
    size_t size() const { ASSERT(!isDeletedValue()); return m_size; }

private:
    static size_t deletedValueSize() { return numeric_limits<size_t>::max(); }

    friend bool operator==(const IntegerArray&, const IntegerArray&);

    const int* m_integers;
    size_t m_size;
};

inline bool operator==(const IntegerArray& a, const IntegerArray& b)
{
    return a.m_integers == b.m_integers &&  a.m_size == b.m_size;
}

struct IntegerArrayHashTraits : WTF::GenericHashTraits<IntegerArray> {
    static const bool needsDestruction = false;
    static void constructDeletedValue(IntegerArray& slot) { slot.markDeleted(); }
    static bool isDeletedValue(const IntegerArray& array) { return array.isDeletedValue(); }
};

struct IntegerArrayHash {
    static unsigned hash(const IntegerArray&);
    static bool equal(const IntegerArray&, const IntegerArray&);
    static const bool safeToCompareToEmptyOrDeleted = true;
};

unsigned IntegerArrayHash::hash(const IntegerArray& array)
{
    return StringHasher::hashMemory(array.integers(), array.size() * sizeof(int));
}

bool IntegerArrayHash::equal(const IntegerArray& a, const IntegerArray& b)
{
    if (a.isDeletedValue() || b.isDeletedValue())
        return a.isDeletedValue() == b.isDeletedValue();
    if (a.size() != b.size())
        return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (a.integers()[i] != b.integers()[i])
            return false;
    }
    return true;
}

typedef size_t ObjectReference;

class BinaryPropertyListPlan : private BinaryPropertyListObjectStream {
public:
    BinaryPropertyListPlan(BinaryPropertyListWriter&);

    ObjectReference booleanTrueObjectReference() const;
    ObjectReference integerObjectReference(int) const;
    ObjectReference stringObjectReference(const String&) const;
    ObjectReference integerArrayObjectReference(const int*, size_t) const;

    ObjectReference objectCount() const { return m_currentObjectReference; }

    ObjectReference byteCount() const { return m_byteCount; }
    ObjectReference objectReferenceCount() const { return m_objectReferenceCount; }

private:
    virtual void writeBooleanTrue();
    virtual void writeInteger(int);
    virtual void writeString(const String&);
    virtual void writeIntegerArray(const int*, size_t);
    virtual void writeUniqueString(const String&);
    virtual void writeUniqueString(const char*);
    virtual size_t writeArrayStart();
    virtual void writeArrayEnd(size_t);
    virtual size_t writeDictionaryStart();
    virtual void writeDictionaryEnd(size_t);

    void writeArrayObject(size_t);
    void writeDictionaryObject(size_t);
    void writeStringObject(const String&);
    void writeStringObject(const char*);

    static ObjectReference invalidObjectReference() { return numeric_limits<ObjectReference>::max(); }

    typedef HashMap<IntegerArray, ObjectReference, IntegerArrayHash, IntegerArrayHashTraits> IntegerArrayMap;

    ObjectReference m_booleanTrueObjectReference;
    ObjectReference m_integerZeroObjectReference;
    HashMap<int, ObjectReference> m_integers;
    HashMap<String, ObjectReference> m_strings;
    IntegerArrayMap m_integerArrays;

    ObjectReference m_currentObjectReference;

    size_t m_currentAggregateSize;

    size_t m_byteCount;
    size_t m_objectReferenceCount;
};

BinaryPropertyListPlan::BinaryPropertyListPlan(BinaryPropertyListWriter& client)
    : m_booleanTrueObjectReference(invalidObjectReference())
    , m_integerZeroObjectReference(invalidObjectReference())
    , m_currentObjectReference(0)
    , m_currentAggregateSize(0)
    , m_byteCount(0)
    , m_objectReferenceCount(0)
{
    client.writeObjects(*this);
    ASSERT(m_currentAggregateSize == 1);
}

void BinaryPropertyListPlan::writeBooleanTrue()
{
    ++m_currentAggregateSize;
    if (m_booleanTrueObjectReference != invalidObjectReference())
        return;
    m_booleanTrueObjectReference = m_currentObjectReference++;
    ++m_byteCount;
}

static inline int integerByteCount(size_t integer)
{
    if (integer <= 0xFF)
        return 2;
    if (integer <= 0xFFFF)
        return 3;
#ifdef __LP64__
    if (integer <= 0xFFFFFFFFULL)
        return 5;
    return 9;
#else
    return 5;
#endif
}

void BinaryPropertyListPlan::writeInteger(int integer)
{
    ASSERT(integer >= 0);
    ++m_currentAggregateSize;
    if (!integer) {
        if (m_integerZeroObjectReference != invalidObjectReference())
            return;
        m_integerZeroObjectReference = m_currentObjectReference;
    } else {
        if (!m_integers.add(integer, m_currentObjectReference).second)
            return;
    }
    ++m_currentObjectReference;
    m_byteCount += integerByteCount(integer);
}

void BinaryPropertyListPlan::writeString(const String& string)
{
    ++m_currentAggregateSize;
    if (!m_strings.add(string, m_currentObjectReference).second)
        return;
    ++m_currentObjectReference;
    writeStringObject(string);
}

void BinaryPropertyListPlan::writeIntegerArray(const int* integers, size_t size)
{
    size_t savedAggregateSize = ++m_currentAggregateSize;
    ASSERT(size);
    pair<IntegerArrayMap::iterator, bool> addResult = m_integerArrays.add(IntegerArray(integers, size), 0);
    if (!addResult.second)
        return;
    for (size_t i = 0; i < size; ++i)
        writeInteger(integers[i]);
    addResult.first->second = m_currentObjectReference++;
    writeArrayObject(size);
    m_currentAggregateSize = savedAggregateSize;
}

void BinaryPropertyListPlan::writeUniqueString(const String& string)
{
    ++m_currentAggregateSize;
    ++m_currentObjectReference;
    writeStringObject(string);
}

void BinaryPropertyListPlan::writeUniqueString(const char* string)
{
    ++m_currentAggregateSize;
    ++m_currentObjectReference;
    writeStringObject(string);
}

size_t BinaryPropertyListPlan::writeArrayStart()
{
    size_t savedAggregateSize = m_currentAggregateSize;
    m_currentAggregateSize = 0;
    return savedAggregateSize;
}

void BinaryPropertyListPlan::writeArrayEnd(size_t savedAggregateSize)
{
    ++m_currentObjectReference;
    writeArrayObject(m_currentAggregateSize);
    m_currentAggregateSize = savedAggregateSize + 1;
}

size_t BinaryPropertyListPlan::writeDictionaryStart()
{
    size_t savedAggregateSize = m_currentAggregateSize;
    m_currentAggregateSize = 0;
    return savedAggregateSize;
}

void BinaryPropertyListPlan::writeDictionaryEnd(size_t savedAggregateSize)
{
    ++m_currentObjectReference;
    writeDictionaryObject(m_currentAggregateSize);
    m_currentAggregateSize = savedAggregateSize + 1;
}

static size_t markerPlusLengthByteCount(size_t length)
{
    if (length <= maxLengthInMarkerByte)
        return 1;
    return 1 + integerByteCount(length);
}

void BinaryPropertyListPlan::writeStringObject(const String& string)
{
    const UChar* characters = string.characters();
    unsigned length = string.length();
    m_byteCount += markerPlusLengthByteCount(length) + length;
    if (!charactersAreAllASCII(characters, length))
        m_byteCount += length;
}

void BinaryPropertyListPlan::writeStringObject(const char* string)
{
    unsigned length = strlen(string);
    m_byteCount += markerPlusLengthByteCount(length) + length;
}

void BinaryPropertyListPlan::writeArrayObject(size_t size)
{
    ASSERT(size);
    m_byteCount += markerPlusLengthByteCount(size);
    m_objectReferenceCount += size;
}

void BinaryPropertyListPlan::writeDictionaryObject(size_t size)
{
    ASSERT(size);
    ASSERT(!(size & 1));
    m_byteCount += markerPlusLengthByteCount(size / 2);
    m_objectReferenceCount += size;
}

ObjectReference BinaryPropertyListPlan::booleanTrueObjectReference() const
{
    ASSERT(m_booleanTrueObjectReference != invalidObjectReference());
    return m_booleanTrueObjectReference;
}

ObjectReference BinaryPropertyListPlan::integerObjectReference(int integer) const
{
    ASSERT(integer >= 0);
    if (!integer) {
        ASSERT(m_integerZeroObjectReference != invalidObjectReference());
        return m_integerZeroObjectReference;
    }
    ASSERT(m_integers.contains(integer));
    return m_integers.get(integer);
}

ObjectReference BinaryPropertyListPlan::stringObjectReference(const String& string) const
{
    ASSERT(m_strings.contains(string));
    return m_strings.get(string);
}

ObjectReference BinaryPropertyListPlan::integerArrayObjectReference(const int* integers, size_t size) const
{
    ASSERT(m_integerArrays.contains(IntegerArray(integers, size)));
    return m_integerArrays.get(IntegerArray(integers, size));
}

class BinaryPropertyListSerializer : private BinaryPropertyListObjectStream {
public:
    BinaryPropertyListSerializer(BinaryPropertyListWriter&);

private:
    virtual void writeBooleanTrue();
    virtual void writeInteger(int);
    virtual void writeString(const String&);
    virtual void writeIntegerArray(const int*, size_t);
    virtual void writeUniqueString(const String&);
    virtual void writeUniqueString(const char*);
    virtual size_t writeArrayStart();
    virtual void writeArrayEnd(size_t);
    virtual size_t writeDictionaryStart();
    virtual void writeDictionaryEnd(size_t);

    ObjectReference writeIntegerWithoutAddingAggregateObjectReference(int);

    void appendIntegerObject(int);
    void appendStringObject(const String&);
    void appendStringObject(const char*);
    void appendIntegerArrayObject(const int*, size_t);

    void appendByte(unsigned char);
    void appendByte(unsigned);
    void appendByte(unsigned long);
    void appendByte(int);

    void appendInteger(size_t);

    void appendObjectReference(ObjectReference);

    void addAggregateObjectReference(ObjectReference);

    void startObject();

    const BinaryPropertyListPlan m_plan;
    const int m_objectReferenceSize;
    const size_t m_offsetTableStart;
    const int m_offsetSize;
    const size_t m_bufferSize;
    UInt8* const m_buffer;

    UInt8* m_currentByte;
    ObjectReference m_currentObjectReference;
    UInt8* m_currentAggregateBufferByte;
};

inline void BinaryPropertyListSerializer::appendByte(unsigned char byte)
{
    *m_currentByte++ = byte;
    ASSERT(m_currentByte <= m_currentAggregateBufferByte);
}

inline void BinaryPropertyListSerializer::appendByte(unsigned byte)
{
    *m_currentByte++ = byte;
    ASSERT(m_currentByte <= m_currentAggregateBufferByte);
}

inline void BinaryPropertyListSerializer::appendByte(unsigned long byte)
{
    *m_currentByte++ = byte;
    ASSERT(m_currentByte <= m_currentAggregateBufferByte);
}

inline void BinaryPropertyListSerializer::appendByte(int byte)
{
    *m_currentByte++ = byte;
    ASSERT(m_currentByte <= m_currentAggregateBufferByte);
}

static int bytesNeeded(size_t count)
{
    ASSERT(count);
    int bytesNeeded = 1;
    for (size_t mask = numeric_limits<size_t>::max() << 8; count & mask; mask <<= 8)
        ++bytesNeeded;
    return bytesNeeded;
}

static inline void storeLength(UInt8* destination, size_t length)
{
#ifdef __LP64__
    destination[0] = length >> 56;
    destination[1] = length >> 48;
    destination[2] = length >> 40;
    destination[3] = length >> 32;
#else
    destination[0] = 0;
    destination[1] = 0;
    destination[2] = 0;
    destination[3] = 0;
#endif
    destination[4] = length >> 24;
    destination[5] = length >> 16;
    destination[6] = length >> 8;
    destination[7] = length;
}

// Like memmove, but reverses the bytes.
static void moveAndReverseBytes(UInt8* destination, const UInt8* source, size_t length)
{
    ASSERT(length);
    memmove(destination, source, length);
    UInt8* start = destination;
    UInt8* end = destination + length;
    while (end - start > 1)
        std::swap(*start++, *--end);
}

// The serializer uses a single buffer for the property list.
// The buffer contains:
//
//    8-byte header
//    object data
//    offset table
//    32-byte trailer
//
// While serializing object, the offset table entry for each object is written just before
// the object data for that object is written. Aggregates, arrays and dictionaries, are a
// special case. The objects that go into an aggregate are written before the aggregate is.
// As each object is written, the object reference is put in the aggregate buffer. Then,
// when the aggregate is written, the aggregate buffer is copied into place in the object
// data. Finally, the header and trailer are written.
//
// The aggregate buffer shares space with the object data, like this:
//
//    8-byte header
//    object data
//    >>> aggregate buffer <<<
//    offset table
//    32-byte trailer
//
// To make it easy to build it incrementally, the buffer starts at the end of the object
// data space, and grows backwards. We're guaranteed the aggregate buffer will never collide
// with the object data pointer because we know that the object data is correctly sized
// based on our plan, and all the data in the aggregate buffer will be used to create the
// actual aggregate objects; in the worst case the aggregate buffer will already be in
// exactly the right place, but backwards.

BinaryPropertyListSerializer::BinaryPropertyListSerializer(BinaryPropertyListWriter& client)
    : m_plan(client)
    , m_objectReferenceSize(bytesNeeded(m_plan.objectCount()))
    , m_offsetTableStart(headerSize + m_plan.byteCount() + m_plan.objectReferenceCount() * m_objectReferenceSize)
    , m_offsetSize(bytesNeeded(m_offsetTableStart))
    , m_bufferSize(m_offsetTableStart + m_plan.objectCount() * m_offsetSize + trailerSize)
    , m_buffer(client.buffer(m_bufferSize))
    , m_currentObjectReference(0)
{
    ASSERT(m_objectReferenceSize > 0);
    ASSERT(m_offsetSize > 0);

#ifdef __LP64__
    ASSERT(m_objectReferenceSize <= 8);
    ASSERT(m_offsetSize <= 8);
#else
    ASSERT(m_objectReferenceSize <= 4);
    ASSERT(m_offsetSize <= 4);
#endif

    if (!m_buffer)
        return;

    // Write objects and offset table.
    m_currentByte = m_buffer + headerSize;
    m_currentAggregateBufferByte = m_buffer + m_offsetTableStart;
    client.writeObjects(*this);
    ASSERT(m_currentObjectReference == m_plan.objectCount());
    ASSERT(m_currentAggregateBufferByte == m_buffer + m_offsetTableStart);
    ASSERT(m_currentByte == m_buffer + m_offsetTableStart);

    // Write header.
    memcpy(m_buffer, "bplist00", headerSize);

    // Write trailer.
    UInt8* trailer = m_buffer + m_bufferSize - trailerSize;
    memset(trailer, 0, 6);
    trailer[6] = m_offsetSize;
    trailer[7] = m_objectReferenceSize;
    storeLength(trailer + 8, m_plan.objectCount());
    storeLength(trailer + 16, m_plan.objectCount() - 1);
    storeLength(trailer + 24, m_offsetTableStart);
}

void BinaryPropertyListSerializer::writeBooleanTrue()
{
    ObjectReference reference = m_plan.booleanTrueObjectReference();
    if (m_currentObjectReference != reference)
        ASSERT(reference < m_currentObjectReference);
    else {
        startObject();
        appendByte(booleanTrueMarkerByte);
    }
    addAggregateObjectReference(reference);
}

inline ObjectReference BinaryPropertyListSerializer::writeIntegerWithoutAddingAggregateObjectReference(int integer)
{
    ObjectReference reference = m_plan.integerObjectReference(integer);
    if (m_currentObjectReference != reference)
        ASSERT(reference < m_currentObjectReference);
    else
        appendIntegerObject(integer);
    return reference;
}

void BinaryPropertyListSerializer::writeInteger(int integer)
{
    addAggregateObjectReference(writeIntegerWithoutAddingAggregateObjectReference(integer));
}

void BinaryPropertyListSerializer::writeString(const String& string)
{
    ObjectReference reference = m_plan.stringObjectReference(string);
    if (m_currentObjectReference != reference)
        ASSERT(reference < m_currentObjectReference);
    else
        appendStringObject(string);
    addAggregateObjectReference(reference);
}

void BinaryPropertyListSerializer::writeIntegerArray(const int* integers, size_t size)
{
    ObjectReference reference = m_plan.integerArrayObjectReference(integers, size);
    for (size_t i = 0; i < size; ++i)
        writeIntegerWithoutAddingAggregateObjectReference(integers[i]);
    if (m_currentObjectReference != reference)
        ASSERT(reference < m_currentObjectReference);
    else
        appendIntegerArrayObject(integers, size);
    addAggregateObjectReference(reference);
}

void BinaryPropertyListSerializer::writeUniqueString(const char* string)
{
    addAggregateObjectReference(m_currentObjectReference);
    appendStringObject(string);
}

void BinaryPropertyListSerializer::writeUniqueString(const String& string)
{
    addAggregateObjectReference(m_currentObjectReference);
    appendStringObject(string);
}

size_t BinaryPropertyListSerializer::writeArrayStart()
{
    return m_currentAggregateBufferByte - m_buffer;
}

void BinaryPropertyListSerializer::writeArrayEnd(size_t savedAggregateBufferOffset)
{
    ObjectReference reference = m_currentObjectReference;
    startObject();
    size_t aggregateBufferByteCount = savedAggregateBufferOffset - (m_currentAggregateBufferByte - m_buffer);
    ASSERT(aggregateBufferByteCount);
    ASSERT(!(aggregateBufferByteCount % m_objectReferenceSize));
    size_t size = aggregateBufferByteCount / m_objectReferenceSize;
    if (size <= maxLengthInMarkerByte)
        appendByte(arrayMarkerByte | size);
    else {
        appendByte(arrayWithSeparateLengthMarkerByte);
        appendInteger(size);
    }
    m_currentAggregateBufferByte = m_buffer + savedAggregateBufferOffset;
    ASSERT(m_currentByte <= m_currentAggregateBufferByte);
    moveAndReverseBytes(m_currentByte, m_currentAggregateBufferByte - aggregateBufferByteCount, aggregateBufferByteCount);
    m_currentByte += aggregateBufferByteCount;
    ASSERT(m_currentByte <= m_currentAggregateBufferByte);
    if (m_currentObjectReference < m_plan.objectCount())
        addAggregateObjectReference(reference);
    else
        ASSERT(m_currentObjectReference == m_plan.objectCount());
}

size_t BinaryPropertyListSerializer::writeDictionaryStart()
{
    return m_currentAggregateBufferByte - m_buffer;
}

void BinaryPropertyListSerializer::writeDictionaryEnd(size_t savedAggregateBufferOffset)
{
    ObjectReference reference = m_currentObjectReference;
    startObject();
    size_t aggregateBufferByteCount = savedAggregateBufferOffset - (m_currentAggregateBufferByte - m_buffer);
    ASSERT(aggregateBufferByteCount);
    ASSERT(!(aggregateBufferByteCount % (m_objectReferenceSize * 2)));
    size_t size = aggregateBufferByteCount / (m_objectReferenceSize * 2);
    if (size <= maxLengthInMarkerByte)
        appendByte(dictionaryMarkerByte | size);
    else {
        appendByte(dictionaryWithSeparateLengthMarkerByte);
        appendInteger(size);
    }
    m_currentAggregateBufferByte = m_buffer + savedAggregateBufferOffset;
    ASSERT(m_currentByte <= m_currentAggregateBufferByte);
    moveAndReverseBytes(m_currentByte, m_currentAggregateBufferByte - aggregateBufferByteCount, aggregateBufferByteCount);
    m_currentByte += aggregateBufferByteCount;
    ASSERT(m_currentByte <= m_currentAggregateBufferByte);
    if (m_currentObjectReference != m_plan.objectCount())
        addAggregateObjectReference(reference);
    else
        ASSERT(m_currentObjectReference == m_plan.objectCount());
}

void BinaryPropertyListSerializer::appendIntegerObject(int integer)
{
    startObject();
    ASSERT(integer >= 0);
    appendInteger(integer);
}

void BinaryPropertyListSerializer::appendInteger(size_t integer)
{
    if (integer <= 0xFF) {
        appendByte(oneByteIntegerMarkerByte);
        appendByte(integer);
        return;
    }
    if (integer <= 0xFFFF) {
        appendByte(twoByteIntegerMarkerByte);
        appendByte(integer >> 8);
        appendByte(integer);
        return;
    }
#ifdef __LP64__
    if (integer <= 0xFFFFFFFFULL) {
#endif
        appendByte(fourByteIntegerMarkerByte);
        appendByte(integer >> 24);
        appendByte(integer >> 16);
        appendByte(integer >> 8);
        appendByte(integer);
#ifdef __LP64__
        return;
    }
    appendByte(eightByteIntegerMarkerByte);
    appendByte(integer >> 56);
    appendByte(integer >> 48);
    appendByte(integer >> 40);
    appendByte(integer >> 32);
    appendByte(integer >> 24);
    appendByte(integer >> 16);
    appendByte(integer >> 8);
    appendByte(integer);
#endif
}

void BinaryPropertyListSerializer::appendStringObject(const String& string)
{
    startObject();
    const UChar* characters = string.characters();
    unsigned length = string.length();
    if (charactersAreAllASCII(characters, length)) {
        if (length <= maxLengthInMarkerByte)
            appendByte(asciiStringMarkerByte | length);
        else {
            appendByte(asciiStringWithSeparateLengthMarkerByte);
            appendInteger(length);
        }
        for (unsigned i = 0; i < length; ++i)
            appendByte(characters[i]);
    } else {
        if (length <= maxLengthInMarkerByte)
            appendByte(unicodeStringMarkerByte | length);
        else {
            appendByte(unicodeStringWithSeparateLengthMarkerByte);
            appendInteger(length);
        }
        for (unsigned i = 0; i < length; ++i) {
            appendByte(characters[i] >> 8);
            appendByte(characters[i]);
        }
    }
}

void BinaryPropertyListSerializer::appendStringObject(const char* string)
{
    startObject();
    unsigned length = strlen(string);
    if (length <= maxLengthInMarkerByte)
        appendByte(asciiStringMarkerByte | length);
    else {
        appendByte(asciiStringWithSeparateLengthMarkerByte);
        appendInteger(length);
    }
    for (unsigned i = 0; i < length; ++i)
        appendByte(string[i]);
}

void BinaryPropertyListSerializer::appendIntegerArrayObject(const int* integers, size_t size)
{
    startObject();
    if (size <= maxLengthInMarkerByte)
        appendByte(arrayMarkerByte | size);
    else {
        appendByte(arrayWithSeparateLengthMarkerByte);
        appendInteger(size);
    }
    for (unsigned i = 0; i < size; ++i)
        appendObjectReference(m_plan.integerObjectReference(integers[i]));
}

void BinaryPropertyListSerializer::appendObjectReference(ObjectReference reference)
{
    switch (m_objectReferenceSize) {
#ifdef __LP64__
        case 8:
            appendByte(reference >> 56);
        case 7:
            appendByte(reference >> 48);
        case 6:
            appendByte(reference >> 40);
        case 5:
            appendByte(reference >> 32);
#endif
        case 4:
            appendByte(reference >> 24);
        case 3:
            appendByte(reference >> 16);
        case 2:
            appendByte(reference >> 8);
        case 1:
            appendByte(reference);
    }
}

void BinaryPropertyListSerializer::startObject()
{
    ObjectReference reference = m_currentObjectReference++;

    size_t offset = m_currentByte - m_buffer;

    UInt8* offsetTableEntry = m_buffer + m_offsetTableStart + reference * m_offsetSize + m_offsetSize;
    switch (m_offsetSize) {
#ifdef __LP64__
        case 8:
            offsetTableEntry[-8] = offset >> 56;
        case 7:
            offsetTableEntry[-7] = offset >> 48;
        case 6:
            offsetTableEntry[-6] = offset >> 40;
        case 5:
            offsetTableEntry[-5] = offset >> 32;
#endif
        case 4:
            offsetTableEntry[-4] = offset >> 24;
        case 3:
            offsetTableEntry[-3] = offset >> 16;
        case 2:
            offsetTableEntry[-2] = offset >> 8;
        case 1:
            offsetTableEntry[-1] = offset;
    }
}

void BinaryPropertyListSerializer::addAggregateObjectReference(ObjectReference reference)
{
    switch (m_objectReferenceSize) {
#ifdef __LP64__
        case 8:
            *--m_currentAggregateBufferByte = reference >> 56;
        case 7:
            *--m_currentAggregateBufferByte = reference >> 48;
        case 6:
            *--m_currentAggregateBufferByte = reference >> 40;
        case 5:
            *--m_currentAggregateBufferByte = reference >> 32;
#endif
        case 4:
            *--m_currentAggregateBufferByte = reference >> 24;
        case 3:
            *--m_currentAggregateBufferByte = reference >> 16;
        case 2:
            *--m_currentAggregateBufferByte = reference >> 8;
        case 1:
            *--m_currentAggregateBufferByte = reference;
    }
    ASSERT(m_currentByte <= m_currentAggregateBufferByte);
}

void BinaryPropertyListWriter::writePropertyList()
{
    BinaryPropertyListSerializer(*this);
}

}
