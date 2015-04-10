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

#ifndef ByValInfo_h
#define ByValInfo_h

#include <wtf/Platform.h>

#if ENABLE(JIT)

#include "ClassInfo.h"
#include "CodeLocation.h"
#include "IndexingType.h"
#include "JITStubRoutine.h"
#include "Structure.h"

namespace JSC {

enum JITArrayMode {
    JITInt32,
    JITDouble,
    JITContiguous,
    JITArrayStorage,
    JITInt8Array,
    JITInt16Array,
    JITInt32Array,
    JITUint8Array,
    JITUint8ClampedArray,
    JITUint16Array,
    JITUint32Array,
    JITFloat32Array,
    JITFloat64Array
};

inline bool isOptimizableIndexingType(IndexingType indexingType)
{
    switch (indexingType) {
    case ALL_INT32_INDEXING_TYPES:
    case ALL_DOUBLE_INDEXING_TYPES:
    case ALL_CONTIGUOUS_INDEXING_TYPES:
    case ARRAY_WITH_ARRAY_STORAGE_INDEXING_TYPES:
        return true;
    default:
        return false;
    }
}

inline bool hasOptimizableIndexingForClassInfo(const ClassInfo* classInfo)
{
    return classInfo->typedArrayStorageType != TypedArrayNone;
}

inline bool hasOptimizableIndexing(Structure* structure)
{
    return isOptimizableIndexingType(structure->indexingType())
        || hasOptimizableIndexingForClassInfo(structure->classInfo());
}

inline JITArrayMode jitArrayModeForIndexingType(IndexingType indexingType)
{
    switch (indexingType) {
    case ALL_INT32_INDEXING_TYPES:
        return JITInt32;
    case ALL_DOUBLE_INDEXING_TYPES:
        return JITDouble;
    case ALL_CONTIGUOUS_INDEXING_TYPES:
        return JITContiguous;
    case ARRAY_WITH_ARRAY_STORAGE_INDEXING_TYPES:
        return JITArrayStorage;
    default:
        CRASH();
        return JITContiguous;
    }
}

inline JITArrayMode jitArrayModeForClassInfo(const ClassInfo* classInfo)
{
    switch (classInfo->typedArrayStorageType) {
    case TypedArrayInt8:
        return JITInt8Array;
    case TypedArrayInt16:
        return JITInt16Array;
    case TypedArrayInt32:
        return JITInt32Array;
    case TypedArrayUint8:
        return JITUint8Array;
    case TypedArrayUint8Clamped:
        return JITUint8ClampedArray;
    case TypedArrayUint16:
        return JITUint16Array;
    case TypedArrayUint32:
        return JITUint32Array;
    case TypedArrayFloat32:
        return JITFloat32Array;
    case TypedArrayFloat64:
        return JITFloat64Array;
    default:
        CRASH();
        return JITContiguous;
    }
}

inline JITArrayMode jitArrayModeForStructure(Structure* structure)
{
    if (isOptimizableIndexingType(structure->indexingType()))
        return jitArrayModeForIndexingType(structure->indexingType());
    
    ASSERT(hasOptimizableIndexingForClassInfo(structure->classInfo()));
    return jitArrayModeForClassInfo(structure->classInfo());
}

struct ByValInfo {
    ByValInfo() { }
    
    ByValInfo(unsigned bytecodeIndex, CodeLocationJump badTypeJump, JITArrayMode arrayMode, int16_t badTypeJumpToDone, int16_t returnAddressToSlowPath)
        : bytecodeIndex(bytecodeIndex)
        , badTypeJump(badTypeJump)
        , arrayMode(arrayMode)
        , badTypeJumpToDone(badTypeJumpToDone)
        , returnAddressToSlowPath(returnAddressToSlowPath)
        , slowPathCount(0)
    {
    }
    
    unsigned bytecodeIndex;
    CodeLocationJump badTypeJump;
    JITArrayMode arrayMode; // The array mode that was baked into the inline JIT code.
    int16_t badTypeJumpToDone;
    int16_t returnAddressToSlowPath;
    unsigned slowPathCount;
    RefPtr<JITStubRoutine> stubRoutine;
};

inline unsigned getByValInfoBytecodeIndex(ByValInfo* info)
{
    return info->bytecodeIndex;
}

} // namespace JSC

#endif // ENABLE(JIT)

#endif // ByValInfo_h

