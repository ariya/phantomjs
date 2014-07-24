/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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

#ifndef Butterfly_h
#define Butterfly_h

#include "IndexingHeader.h"
#include "PropertyOffset.h"
#include "PropertyStorage.h"
#include <wtf/Noncopyable.h>
#include <wtf/Platform.h>

namespace JSC {

class VM;
class CopyVisitor;
struct ArrayStorage;

template <typename T> struct ContiguousData {
    ContiguousData()
        : m_data(0)
#if !ASSERT_DISABLED
        , m_length(0)
#endif
    {
    }
    ContiguousData(T* data, size_t length)
        : m_data(data)
#if !ASSERT_DISABLED
        , m_length(length)
#endif
    {
        UNUSED_PARAM(length);
    }

    const T& operator[](size_t index) const { ASSERT(index < m_length); return m_data[index]; }
    T& operator[](size_t index) { ASSERT(index < m_length); return m_data[index]; }

    T* data() const { return m_data; }
#if !ASSERT_DISABLED
    size_t length() const { return m_length; }
#endif

private:
    T* m_data;
#if !ASSERT_DISABLED
    size_t m_length;
#endif
};

typedef ContiguousData<double> ContiguousDoubles;
typedef ContiguousData<WriteBarrier<Unknown> > ContiguousJSValues;

class Butterfly {
    WTF_MAKE_NONCOPYABLE(Butterfly);
private:
    Butterfly() { } // Not instantiable.
public:
    
    static size_t totalSize(size_t preCapacity, size_t propertyCapacity, bool hasIndexingHeader, size_t indexingPayloadSizeInBytes)
    {
        ASSERT(indexingPayloadSizeInBytes ? hasIndexingHeader : true);
        ASSERT(sizeof(EncodedJSValue) == sizeof(IndexingHeader));
        return (preCapacity + propertyCapacity) * sizeof(EncodedJSValue) + (hasIndexingHeader ? sizeof(IndexingHeader) : 0) + indexingPayloadSizeInBytes;
    }

    static Butterfly* fromBase(void* base, size_t preCapacity, size_t propertyCapacity)
    {
        return reinterpret_cast<Butterfly*>(static_cast<EncodedJSValue*>(base) + preCapacity + propertyCapacity + 1);
    }
    
    // This method is here not just because it's handy, but to remind you that
    // the whole point of butterflies is to do evil pointer arithmetic.
    static Butterfly* fromPointer(char* ptr)
    {
        return reinterpret_cast<Butterfly*>(ptr);
    }
    
    char* pointer() { return reinterpret_cast<char*>(this); }
    
    static ptrdiff_t offsetOfIndexingHeader() { return IndexingHeader::offsetOfIndexingHeader(); }
    static ptrdiff_t offsetOfPublicLength() { return offsetOfIndexingHeader() + IndexingHeader::offsetOfPublicLength(); }
    static ptrdiff_t offsetOfVectorLength() { return offsetOfIndexingHeader() + IndexingHeader::offsetOfVectorLength(); }
    
    static Butterfly* createUninitialized(VM&, size_t preCapacity, size_t propertyCapacity, bool hasIndexingHeader, size_t indexingPayloadSizeInBytes);

    static Butterfly* create(VM&, size_t preCapacity, size_t propertyCapacity, bool hasIndexingHeader, const IndexingHeader&, size_t indexingPayloadSizeInBytes);
    static Butterfly* create(VM&, Structure*);
    static Butterfly* createUninitializedDuringCollection(CopyVisitor&, size_t preCapacity, size_t propertyCapacity, bool hasIndexingHeader, size_t indexingPayloadSizeInBytes);
    
    IndexingHeader* indexingHeader() { return IndexingHeader::from(this); }
    const IndexingHeader* indexingHeader() const { return IndexingHeader::from(this); }
    PropertyStorage propertyStorage() { return indexingHeader()->propertyStorage(); }
    ConstPropertyStorage propertyStorage() const { return indexingHeader()->propertyStorage(); }
    
    uint32_t publicLength() { return indexingHeader()->publicLength(); }
    uint32_t vectorLength() { return indexingHeader()->vectorLength(); }
    void setPublicLength(uint32_t value) { indexingHeader()->setPublicLength(value); }
    void setVectorLength(uint32_t value) { indexingHeader()->setVectorLength(value); }
    
    template<typename T>
    T* indexingPayload() { return reinterpret_cast_ptr<T*>(this); }
    ArrayStorage* arrayStorage() { return indexingPayload<ArrayStorage>(); }
    ContiguousJSValues contiguousInt32() { return ContiguousJSValues(indexingPayload<WriteBarrier<Unknown> >(), vectorLength()); }

    ContiguousDoubles contiguousDouble() { return ContiguousDoubles(indexingPayload<double>(), vectorLength()); }
    ContiguousJSValues contiguous() { return ContiguousJSValues(indexingPayload<WriteBarrier<Unknown> >(), vectorLength()); }
    
    static Butterfly* fromContiguous(WriteBarrier<Unknown>* contiguous)
    {
        return reinterpret_cast<Butterfly*>(contiguous);
    }
    static Butterfly* fromContiguous(double* contiguous)
    {
        return reinterpret_cast<Butterfly*>(contiguous);
    }
    
    static ptrdiff_t offsetOfPropertyStorage() { return -static_cast<ptrdiff_t>(sizeof(IndexingHeader)); }
    static int indexOfPropertyStorage()
    {
        ASSERT(sizeof(IndexingHeader) == sizeof(EncodedJSValue));
        return -1;
    }

    void* base(size_t preCapacity, size_t propertyCapacity) { return propertyStorage() - propertyCapacity - preCapacity; }
    void* base(Structure*);

    static Butterfly* createOrGrowArrayRight(Butterfly*, VM&, Structure* oldStructure, size_t propertyCapacity, bool hadIndexingHeader, size_t oldIndexingPayloadSizeInBytes, size_t newIndexingPayloadSizeInBytes); 

    // The butterfly reallocation methods perform the reallocation itself but do not change any
    // of the meta-data to reflect that the reallocation occurred. Note that this set of
    // methods is not exhaustive and is not intended to encapsulate all possible allocation
    // modes of butterflies - there are code paths that allocate butterflies by calling
    // directly into Heap::tryAllocateStorage.
    Butterfly* growPropertyStorage(VM&, size_t preCapacity, size_t oldPropertyCapacity, bool hasIndexingHeader, size_t indexingPayloadSizeInBytes, size_t newPropertyCapacity);
    Butterfly* growPropertyStorage(VM&, Structure* oldStructure, size_t oldPropertyCapacity, size_t newPropertyCapacity);
    Butterfly* growPropertyStorage(VM&, Structure* oldStructure, size_t newPropertyCapacity);
    Butterfly* growArrayRight(VM&, Structure* oldStructure, size_t propertyCapacity, bool hadIndexingHeader, size_t oldIndexingPayloadSizeInBytes, size_t newIndexingPayloadSizeInBytes); // Assumes that preCapacity is zero, and asserts as much.
    Butterfly* growArrayRight(VM&, Structure*, size_t newIndexingPayloadSizeInBytes);
    Butterfly* resizeArray(VM&, size_t propertyCapacity, bool oldHasIndexingHeader, size_t oldIndexingPayloadSizeInBytes, size_t newPreCapacity, bool newHasIndexingHeader, size_t newIndexingPayloadSizeInBytes);
    Butterfly* resizeArray(VM&, Structure*, size_t newPreCapacity, size_t newIndexingPayloadSizeInBytes); // Assumes that you're not changing whether or not the object has an indexing header.
    Butterfly* unshift(Structure*, size_t numberOfSlots);
    Butterfly* shift(Structure*, size_t numberOfSlots);
};

} // namespace JSC

#endif // Butterfly_h

