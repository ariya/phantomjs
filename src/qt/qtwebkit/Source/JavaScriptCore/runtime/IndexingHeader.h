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

#ifndef IndexingHeader_h
#define IndexingHeader_h

#include "PropertyStorage.h"
#include <wtf/Platform.h>

namespace JSC {

class Butterfly;
class LLIntOffsetsExtractor;
class Structure;
struct ArrayStorage;

class IndexingHeader {
public:
    // Define the maximum storage vector length to be 2^32 / sizeof(JSValue) / 2 to ensure that
    // there is no risk of overflow.
    enum { maximumLength = 0x10000000 };
    
    static ptrdiff_t offsetOfIndexingHeader() { return -static_cast<ptrdiff_t>(sizeof(IndexingHeader)); }
    
    static ptrdiff_t offsetOfPublicLength() { return OBJECT_OFFSETOF(IndexingHeader, u.lengths.publicLength); }
    static ptrdiff_t offsetOfVectorLength() { return OBJECT_OFFSETOF(IndexingHeader, u.lengths.vectorLength); }
    
    IndexingHeader()
    {
        u.lengths.publicLength = 0;
        u.lengths.vectorLength = 0;
    }
    
    uint32_t vectorLength() const { return u.lengths.vectorLength; }
    
    void setVectorLength(uint32_t length)
    {
        RELEASE_ASSERT(length <= maximumLength);
        u.lengths.vectorLength = length;
    }
    
    uint32_t publicLength() { return u.lengths.publicLength; }
    void setPublicLength(uint32_t auxWord) { u.lengths.publicLength = auxWord; }
    
    static IndexingHeader* from(Butterfly* butterfly)
    {
        return reinterpret_cast<IndexingHeader*>(butterfly) - 1;
    }
    
    static const IndexingHeader* from(const Butterfly* butterfly)
    {
        return reinterpret_cast<const IndexingHeader*>(butterfly) - 1;
    }
    
    static IndexingHeader* from(ArrayStorage* arrayStorage)
    {
        return reinterpret_cast<IndexingHeader*>(arrayStorage) - 1;
    }
    
    static IndexingHeader* fromEndOf(PropertyStorage propertyStorage)
    {
        return reinterpret_cast<IndexingHeader*>(propertyStorage);
    }
    
    PropertyStorage propertyStorage()
    {
        return reinterpret_cast_ptr<PropertyStorage>(this);
    }
    
    ConstPropertyStorage propertyStorage() const
    {
        return reinterpret_cast_ptr<ConstPropertyStorage>(this);
    }
    
    ArrayStorage* arrayStorage()
    {
        return reinterpret_cast<ArrayStorage*>(this + 1);
    }
    
    Butterfly* butterfly()
    {
        return reinterpret_cast<Butterfly*>(this + 1);
    }
    
    // These methods are not standalone in the sense that they cannot be
    // used on a copy of the IndexingHeader.
    size_t preCapacity(Structure*);
    size_t indexingPayloadSizeInBytes(Structure*);
    
private:
    friend class LLIntOffsetsExtractor;

    union {
        struct {
            uint32_t publicLength; // The meaning of this field depends on the array type, but for all JSArrays we rely on this being the publicly visible length (array.length).
            uint32_t vectorLength; // The length of the indexed property storage. The actual size of the storage depends on this, and the type.
        } lengths;
    } u;
};

} // namespace JSC

#endif // IndexingHeader_h

