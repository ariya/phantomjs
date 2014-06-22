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
#include "JSCellInlines.h"
#include "ArrayProfile.h"

#include "CodeBlock.h"
#include <wtf/StringExtras.h>
#include <wtf/StringPrintStream.h>

namespace JSC {

void dumpArrayModes(PrintStream& out, ArrayModes arrayModes)
{
    if (!arrayModes) {
        out.print("0:<empty>");
        return;
    }
    
    if (arrayModes == ALL_ARRAY_MODES) {
        out.print("TOP");
        return;
    }
    
    out.print(arrayModes, ":");
    
    if (arrayModes & asArrayModes(NonArray))
        out.print("NonArray");
    if (arrayModes & asArrayModes(NonArrayWithInt32))
        out.print("NonArrayWithInt32");
    if (arrayModes & asArrayModes(NonArrayWithDouble))
        out.print("NonArrayWithDouble");
    if (arrayModes & asArrayModes(NonArrayWithContiguous))
        out.print("NonArrayWithContiguous");
    if (arrayModes & asArrayModes(NonArrayWithArrayStorage))
        out.print("NonArrayWithArrayStorage");
    if (arrayModes & asArrayModes(NonArrayWithSlowPutArrayStorage))
        out.print("NonArrayWithSlowPutArrayStorage");
    if (arrayModes & asArrayModes(ArrayClass))
        out.print("ArrayClass");
    if (arrayModes & asArrayModes(ArrayWithUndecided))
        out.print("ArrayWithUndecided");
    if (arrayModes & asArrayModes(ArrayWithInt32))
        out.print("ArrayWithInt32");
    if (arrayModes & asArrayModes(ArrayWithDouble))
        out.print("ArrayWithDouble");
    if (arrayModes & asArrayModes(ArrayWithContiguous))
        out.print("ArrayWithContiguous");
    if (arrayModes & asArrayModes(ArrayWithArrayStorage))
        out.print("ArrayWithArrayStorage");
    if (arrayModes & asArrayModes(ArrayWithSlowPutArrayStorage))
        out.print("ArrayWithSlowPutArrayStorage");
}

ArrayModes ArrayProfile::updatedObservedArrayModes() const
{
    if (m_lastSeenStructure)
        return m_observedArrayModes | arrayModeFromStructure(m_lastSeenStructure);
    return m_observedArrayModes;
}

void ArrayProfile::computeUpdatedPrediction(CodeBlock* codeBlock, OperationInProgress operation)
{
    const bool verbose = false;
    
    if (m_lastSeenStructure) {
        m_observedArrayModes |= arrayModeFromStructure(m_lastSeenStructure);
        m_mayInterceptIndexedAccesses |=
            m_lastSeenStructure->typeInfo().interceptsGetOwnPropertySlotByIndexEvenWhenLengthIsNotZero();
        if (!codeBlock->globalObject()->isOriginalArrayStructure(m_lastSeenStructure))
            m_usesOriginalArrayStructures = false;
        if (!structureIsPolymorphic()) {
            if (!m_expectedStructure)
                m_expectedStructure = m_lastSeenStructure;
            else if (m_expectedStructure != m_lastSeenStructure) {
                if (verbose)
                    dataLog(*codeBlock, " bc#", m_bytecodeOffset, ": making structure polymorphic because ", RawPointer(m_expectedStructure), " (", m_expectedStructure->classInfo()->className, ") != ", RawPointer(m_lastSeenStructure), " (", m_lastSeenStructure->classInfo()->className, ")\n");
                m_expectedStructure = polymorphicStructure();
            }
        }
        m_lastSeenStructure = 0;
    }
    
    if (hasTwoOrMoreBitsSet(m_observedArrayModes)) {
        if (verbose)
            dataLog(*codeBlock, " bc#", m_bytecodeOffset, ": making structure polymorphic because two or more bits are set in m_observedArrayModes\n");
        m_expectedStructure = polymorphicStructure();
    }
    
    if (operation == Collection
        && expectedStructure()
        && !Heap::isMarked(m_expectedStructure)) {
        if (verbose)
            dataLog(*codeBlock, " bc#", m_bytecodeOffset, ": making structure during GC\n");
        m_expectedStructure = polymorphicStructure();
    }
}

CString ArrayProfile::briefDescription(CodeBlock* codeBlock)
{
    computeUpdatedPrediction(codeBlock);
    
    StringPrintStream out;
    
    bool hasPrinted = false;
    
    if (m_observedArrayModes) {
        if (hasPrinted)
            out.print(", ");
        out.print(ArrayModesDump(m_observedArrayModes));
        hasPrinted = true;
    }
    
    if (structureIsPolymorphic()) {
        if (hasPrinted)
            out.print(", ");
        out.print("struct = TOP");
        hasPrinted = true;
    } else if (m_expectedStructure) {
        if (hasPrinted)
            out.print(", ");
        out.print("struct = ", RawPointer(m_expectedStructure));
        hasPrinted = true;
    }
    
    if (m_mayStoreToHole) {
        if (hasPrinted)
            out.print(", ");
        out.print("Hole");
        hasPrinted = true;
    }
    
    if (m_outOfBounds) {
        if (hasPrinted)
            out.print(", ");
        out.print("OutOfBounds");
        hasPrinted = true;
    }
    
    if (m_mayInterceptIndexedAccesses) {
        if (hasPrinted)
            out.print(", ");
        out.print("Intercept");
        hasPrinted = true;
    }
    
    if (m_usesOriginalArrayStructures) {
        if (hasPrinted)
            out.print(", ");
        out.print("Original");
        hasPrinted = true;
    }
    
    UNUSED_PARAM(hasPrinted);
    
    return out.toCString();
}

} // namespace JSC

