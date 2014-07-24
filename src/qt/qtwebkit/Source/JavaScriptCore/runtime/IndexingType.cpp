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

#include "config.h"
#include "IndexingType.h"

#include <stdio.h>
#include <wtf/StringExtras.h>

namespace JSC {

IndexingType leastUpperBoundOfIndexingTypes(IndexingType a, IndexingType b)
{
    // It doesn't make sense to LUB something that is an array with something that isn't.
    ASSERT((a & IsArray) == (b & IsArray));

    // Boy, this sure is easy right now.
    return std::max(a, b);
}

IndexingType leastUpperBoundOfIndexingTypeAndType(IndexingType indexingType, SpeculatedType type)
{
    if (!type)
        return indexingType;
    switch (indexingType) {
    case ALL_BLANK_INDEXING_TYPES:
    case ALL_UNDECIDED_INDEXING_TYPES:
    case ALL_INT32_INDEXING_TYPES:
        if (isInt32Speculation(type))
            return (indexingType & ~IndexingShapeMask) | Int32Shape;
        if (isNumberSpeculation(type))
            return (indexingType & ~IndexingShapeMask) | DoubleShape;
        return (indexingType & ~IndexingShapeMask) | ContiguousShape;
    case ALL_DOUBLE_INDEXING_TYPES:
        if (isNumberSpeculation(type))
            return indexingType;
        return (indexingType & ~IndexingShapeMask) | ContiguousShape;
    case ALL_CONTIGUOUS_INDEXING_TYPES:
    case ALL_ARRAY_STORAGE_INDEXING_TYPES:
        return indexingType;
    default:
        CRASH();
        return 0;
    }
}

IndexingType leastUpperBoundOfIndexingTypeAndValue(IndexingType indexingType, JSValue value)
{
    return leastUpperBoundOfIndexingTypeAndType(indexingType, speculationFromValue(value));
}

void dumpIndexingType(PrintStream& out, IndexingType indexingType)
{
    const char* basicName;
    switch (indexingType & AllArrayTypes) {
    case NonArray:
        basicName = "NonArray";
        break;
    case NonArrayWithInt32:
        basicName = "NonArrayWithInt32";
        break;
    case NonArrayWithDouble:
        basicName = "NonArrayWithDouble";
        break;
    case NonArrayWithContiguous:
        basicName = "NonArrayWithContiguous";
        break;
    case NonArrayWithArrayStorage:
        basicName = "NonArrayWithArrayStorage";
        break;
    case NonArrayWithSlowPutArrayStorage:
        basicName = "NonArrayWithSlowPutArrayStorage";
        break;
    case ArrayClass:
        basicName = "ArrayClass";
        break;
    case ArrayWithUndecided:
        basicName = "ArrayWithUndecided";
        break;
    case ArrayWithInt32:
        basicName = "ArrayWithInt32";
        break;
    case ArrayWithDouble:
        basicName = "ArrayWithDouble";
        break;
    case ArrayWithContiguous:
        basicName = "ArrayWithContiguous";
        break;
    case ArrayWithArrayStorage:
        basicName = "ArrayWithArrayStorage";
        break;
    case ArrayWithSlowPutArrayStorage:
        basicName = "ArrayWithSlowPutArrayStorage";
        break;
    default:
        basicName = "Unknown!";
        break;
    }
    
    out.printf("%s%s", basicName, (indexingType & MayHaveIndexedAccessors) ? "|MayHaveIndexedAccessors" : "");
}

} // namespace JSC

