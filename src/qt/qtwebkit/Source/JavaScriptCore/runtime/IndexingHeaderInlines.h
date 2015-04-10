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

#ifndef IndexingHeaderInlines_h
#define IndexingHeaderInlines_h

#include "ArrayStorage.h"
#include "IndexingHeader.h"
#include "Structure.h"

namespace JSC {

inline size_t IndexingHeader::preCapacity(Structure* structure)
{
    if (LIKELY(!hasArrayStorage(structure->indexingType())))
        return 0;
    
    return arrayStorage()->m_indexBias;
}

inline size_t IndexingHeader::indexingPayloadSizeInBytes(Structure* structure)
{
    switch (structure->indexingType()) {
    case ALL_UNDECIDED_INDEXING_TYPES:
    case ALL_INT32_INDEXING_TYPES:
    case ALL_DOUBLE_INDEXING_TYPES:
    case ALL_CONTIGUOUS_INDEXING_TYPES:
        return vectorLength() * sizeof(EncodedJSValue);
        
    case ALL_ARRAY_STORAGE_INDEXING_TYPES:
        return ArrayStorage::sizeFor(arrayStorage()->vectorLength());
        
    default:
        ASSERT(!hasIndexedProperties(structure->indexingType()));
        return 0;
    }
}

} // namespace JSC

#endif // IndexingHeaderInlines_h

