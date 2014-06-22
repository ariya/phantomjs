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

#ifndef ArrayProfile_h
#define ArrayProfile_h

#include "JSArray.h"
#include "Structure.h"
#include <wtf/HashMap.h>
#include <wtf/SegmentedVector.h>

namespace JSC {

class CodeBlock;
class LLIntOffsetsExtractor;

// This is a bitfield where each bit represents an IndexingType that we have seen.
// There are 32 indexing types, so an unsigned is enough.
typedef unsigned ArrayModes;

#define asArrayModes(type) \
    (static_cast<unsigned>(1) << static_cast<unsigned>(type))

#define ALL_NON_ARRAY_ARRAY_MODES                       \
    (asArrayModes(NonArray)                             \
    | asArrayModes(NonArrayWithInt32)                   \
    | asArrayModes(NonArrayWithDouble)                  \
    | asArrayModes(NonArrayWithContiguous)              \
    | asArrayModes(NonArrayWithArrayStorage)            \
    | asArrayModes(NonArrayWithSlowPutArrayStorage))

#define ALL_ARRAY_ARRAY_MODES                           \
    (asArrayModes(ArrayClass)                           \
    | asArrayModes(ArrayWithUndecided)                  \
    | asArrayModes(ArrayWithInt32)                      \
    | asArrayModes(ArrayWithDouble)                     \
    | asArrayModes(ArrayWithContiguous)                 \
    | asArrayModes(ArrayWithArrayStorage)               \
    | asArrayModes(ArrayWithSlowPutArrayStorage))

#define ALL_ARRAY_MODES (ALL_NON_ARRAY_ARRAY_MODES | ALL_ARRAY_ARRAY_MODES)

inline ArrayModes arrayModeFromStructure(Structure* structure)
{
    return asArrayModes(structure->indexingType());
}

void dumpArrayModes(PrintStream&, ArrayModes);
MAKE_PRINT_ADAPTOR(ArrayModesDump, ArrayModes, dumpArrayModes);

inline bool mergeArrayModes(ArrayModes& left, ArrayModes right)
{
    ArrayModes newModes = left | right;
    if (newModes == left)
        return false;
    left = newModes;
    return true;
}

// Checks if proven is a subset of expected.
inline bool arrayModesAlreadyChecked(ArrayModes proven, ArrayModes expected)
{
    return (expected | proven) == expected;
}

inline bool arrayModesInclude(ArrayModes arrayModes, IndexingType shape)
{
    return !!(arrayModes & (asArrayModes(NonArray | shape) | asArrayModes(ArrayClass | shape)));
}

inline bool shouldUseSlowPutArrayStorage(ArrayModes arrayModes)
{
    return arrayModesInclude(arrayModes, SlowPutArrayStorageShape);
}

inline bool shouldUseFastArrayStorage(ArrayModes arrayModes)
{
    return arrayModesInclude(arrayModes, ArrayStorageShape);
}

inline bool shouldUseContiguous(ArrayModes arrayModes)
{
    return arrayModesInclude(arrayModes, ContiguousShape);
}

inline bool shouldUseDouble(ArrayModes arrayModes)
{
    return arrayModesInclude(arrayModes, DoubleShape);
}

inline bool shouldUseInt32(ArrayModes arrayModes)
{
    return arrayModesInclude(arrayModes, Int32Shape);
}

inline bool hasSeenArray(ArrayModes arrayModes)
{
    return arrayModes & ALL_ARRAY_ARRAY_MODES;
}

inline bool hasSeenNonArray(ArrayModes arrayModes)
{
    return arrayModes & ALL_NON_ARRAY_ARRAY_MODES;
}

class ArrayProfile {
public:
    ArrayProfile()
        : m_bytecodeOffset(std::numeric_limits<unsigned>::max())
        , m_lastSeenStructure(0)
        , m_expectedStructure(0)
        , m_mayStoreToHole(false)
        , m_outOfBounds(false)
        , m_mayInterceptIndexedAccesses(false)
        , m_usesOriginalArrayStructures(true)
        , m_observedArrayModes(0)
    {
    }
    
    ArrayProfile(unsigned bytecodeOffset)
        : m_bytecodeOffset(bytecodeOffset)
        , m_lastSeenStructure(0)
        , m_expectedStructure(0)
        , m_mayStoreToHole(false)
        , m_outOfBounds(false)
        , m_mayInterceptIndexedAccesses(false)
        , m_usesOriginalArrayStructures(true)
        , m_observedArrayModes(0)
    {
    }
    
    unsigned bytecodeOffset() const { return m_bytecodeOffset; }
    
    Structure** addressOfLastSeenStructure() { return &m_lastSeenStructure; }
    ArrayModes* addressOfArrayModes() { return &m_observedArrayModes; }
    bool* addressOfMayStoreToHole() { return &m_mayStoreToHole; }
    bool* addressOfOutOfBounds() { return &m_outOfBounds; }
    
    void observeStructure(Structure* structure)
    {
        m_lastSeenStructure = structure;
    }
    
    void computeUpdatedPrediction(CodeBlock*, OperationInProgress = NoOperation);
    
    Structure* expectedStructure() const
    {
        if (structureIsPolymorphic())
            return 0;
        return m_expectedStructure;
    }
    bool structureIsPolymorphic() const
    {
        return m_expectedStructure == polymorphicStructure();
    }
    bool hasDefiniteStructure() const
    {
        return !structureIsPolymorphic() && m_expectedStructure;
    }
    ArrayModes observedArrayModes() const { return m_observedArrayModes; }
    ArrayModes updatedObservedArrayModes() const; // Computes the observed array modes without updating the profile.
    bool mayInterceptIndexedAccesses() const { return m_mayInterceptIndexedAccesses; }
    
    bool mayStoreToHole() const { return m_mayStoreToHole; }
    bool outOfBounds() const { return m_outOfBounds; }
    
    bool usesOriginalArrayStructures() const { return m_usesOriginalArrayStructures; }
    
    CString briefDescription(CodeBlock*);
    
private:
    friend class LLIntOffsetsExtractor;
    
    static Structure* polymorphicStructure() { return static_cast<Structure*>(reinterpret_cast<void*>(1)); }
    
    unsigned m_bytecodeOffset;
    Structure* m_lastSeenStructure;
    Structure* m_expectedStructure;
    bool m_mayStoreToHole; // This flag may become overloaded to indicate other special cases that were encountered during array access, as it depends on indexing type. Since we currently have basically just one indexing type (two variants of ArrayStorage), this flag for now just means exactly what its name implies.
    bool m_outOfBounds;
    bool m_mayInterceptIndexedAccesses;
    bool m_usesOriginalArrayStructures;
    ArrayModes m_observedArrayModes;
};

typedef SegmentedVector<ArrayProfile, 4, 0> ArrayProfileVector;

} // namespace JSC

#endif // ArrayProfile_h

