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
#include "DFGArrayMode.h"

#if ENABLE(DFG_JIT)

#include "DFGAbstractValue.h"
#include "DFGGraph.h"
#include "Operations.h"

namespace JSC { namespace DFG {

ArrayMode ArrayMode::fromObserved(ArrayProfile* profile, Array::Action action, bool makeSafe)
{
    ArrayModes observed = profile->observedArrayModes();
    switch (observed) {
    case 0:
        return ArrayMode(Array::Unprofiled);
    case asArrayModes(NonArray):
        if (action == Array::Write && !profile->mayInterceptIndexedAccesses())
            return ArrayMode(Array::Undecided, Array::NonArray, Array::OutOfBounds, Array::Convert);
        return ArrayMode(Array::SelectUsingPredictions);

    case asArrayModes(ArrayWithUndecided):
        if (action == Array::Write)
            return ArrayMode(Array::Undecided, Array::Array, Array::OutOfBounds, Array::Convert);
        return ArrayMode(Array::Generic);
        
    case asArrayModes(NonArray) | asArrayModes(ArrayWithUndecided):
        if (action == Array::Write && !profile->mayInterceptIndexedAccesses())
            return ArrayMode(Array::Undecided, Array::PossiblyArray, Array::OutOfBounds, Array::Convert);
        return ArrayMode(Array::SelectUsingPredictions);

    case asArrayModes(NonArrayWithInt32):
        return ArrayMode(Array::Int32, Array::NonArray, Array::AsIs).withProfile(profile, makeSafe);
    case asArrayModes(ArrayWithInt32):
        return ArrayMode(Array::Int32, Array::Array, Array::AsIs).withProfile(profile, makeSafe);
    case asArrayModes(NonArrayWithInt32) | asArrayModes(ArrayWithInt32):
        return ArrayMode(Array::Int32, Array::PossiblyArray, Array::AsIs).withProfile(profile, makeSafe);

    case asArrayModes(NonArrayWithDouble):
        return ArrayMode(Array::Double, Array::NonArray, Array::AsIs).withProfile(profile, makeSafe);
    case asArrayModes(ArrayWithDouble):
        return ArrayMode(Array::Double, Array::Array, Array::AsIs).withProfile(profile, makeSafe);
    case asArrayModes(NonArrayWithDouble) | asArrayModes(ArrayWithDouble):
        return ArrayMode(Array::Double, Array::PossiblyArray, Array::AsIs).withProfile(profile, makeSafe);

    case asArrayModes(NonArrayWithContiguous):
        return ArrayMode(Array::Contiguous, Array::NonArray, Array::AsIs).withProfile(profile, makeSafe);
    case asArrayModes(ArrayWithContiguous):
        return ArrayMode(Array::Contiguous, Array::Array, Array::AsIs).withProfile(profile, makeSafe);
    case asArrayModes(NonArrayWithContiguous) | asArrayModes(ArrayWithContiguous):
        return ArrayMode(Array::Contiguous, Array::PossiblyArray, Array::AsIs).withProfile(profile, makeSafe);

    case asArrayModes(NonArrayWithArrayStorage):
        return ArrayMode(Array::ArrayStorage, Array::NonArray, Array::AsIs).withProfile(profile, makeSafe);
    case asArrayModes(NonArrayWithSlowPutArrayStorage):
    case asArrayModes(NonArrayWithArrayStorage) | asArrayModes(NonArrayWithSlowPutArrayStorage):
        return ArrayMode(Array::SlowPutArrayStorage, Array::NonArray, Array::AsIs).withProfile(profile, makeSafe);
    case asArrayModes(ArrayWithArrayStorage):
        return ArrayMode(Array::ArrayStorage, Array::Array, Array::AsIs).withProfile(profile, makeSafe);
    case asArrayModes(ArrayWithSlowPutArrayStorage):
    case asArrayModes(ArrayWithArrayStorage) | asArrayModes(ArrayWithSlowPutArrayStorage):
        return ArrayMode(Array::SlowPutArrayStorage, Array::Array, Array::AsIs).withProfile(profile, makeSafe);
    case asArrayModes(NonArrayWithArrayStorage) | asArrayModes(ArrayWithArrayStorage):
        return ArrayMode(Array::ArrayStorage, Array::PossiblyArray, Array::AsIs).withProfile(profile, makeSafe);
    case asArrayModes(NonArrayWithSlowPutArrayStorage) | asArrayModes(ArrayWithSlowPutArrayStorage):
    case asArrayModes(NonArrayWithArrayStorage) | asArrayModes(ArrayWithArrayStorage) | asArrayModes(NonArrayWithSlowPutArrayStorage) | asArrayModes(ArrayWithSlowPutArrayStorage):
        return ArrayMode(Array::SlowPutArrayStorage, Array::PossiblyArray, Array::AsIs).withProfile(profile, makeSafe);

    default:
        if ((observed & asArrayModes(NonArray)) && profile->mayInterceptIndexedAccesses())
            return ArrayMode(Array::SelectUsingPredictions);
        
        Array::Type type;
        Array::Class arrayClass;
        
        if (shouldUseSlowPutArrayStorage(observed))
            type = Array::SlowPutArrayStorage;
        else if (shouldUseFastArrayStorage(observed))
            type = Array::ArrayStorage;
        else if (shouldUseContiguous(observed))
            type = Array::Contiguous;
        else if (shouldUseDouble(observed))
            type = Array::Double;
        else if (shouldUseInt32(observed))
            type = Array::Int32;
        else
            type = Array::Undecided;
        
        if (hasSeenArray(observed) && hasSeenNonArray(observed))
            arrayClass = Array::PossiblyArray;
        else if (hasSeenArray(observed))
            arrayClass = Array::Array;
        else if (hasSeenNonArray(observed))
            arrayClass = Array::NonArray;
        else
            arrayClass = Array::PossiblyArray;
        
        return ArrayMode(type, arrayClass, Array::Convert).withProfile(profile, makeSafe);
    }
}

ArrayMode ArrayMode::refine(SpeculatedType base, SpeculatedType index, SpeculatedType value, NodeFlags flags) const
{
    if (!base || !index) {
        // It can be that we had a legitimate arrayMode but no incoming predictions. That'll
        // happen if we inlined code based on, say, a global variable watchpoint, but later
        // realized that the callsite could not have possibly executed. It may be worthwhile
        // to fix that, but for now I'm leaving it as-is.
        return ArrayMode(Array::ForceExit);
    }
    
    if (!isInt32Speculation(index))
        return ArrayMode(Array::Generic);
    
    // Note: our profiling currently doesn't give us good information in case we have
    // an unlikely control flow path that sets the base to a non-cell value. Value
    // profiling and prediction propagation will probably tell us that the value is
    // either a cell or not, but that doesn't tell us which is more likely: that this
    // is an array access on a cell (what we want and can optimize) or that the user is
    // doing a crazy by-val access on a primitive (we can't easily optimize this and
    // don't want to). So, for now, we assume that if the base is not a cell according
    // to value profiling, but the array profile tells us something else, then we
    // should just trust the array profile.
    
    switch (type()) {
    case Array::Unprofiled:
        return ArrayMode(Array::ForceExit);
        
    case Array::Undecided:
        if (!value)
            return withType(Array::ForceExit);
        if (isInt32Speculation(value))
            return withTypeAndConversion(Array::Int32, Array::Convert);
        if (isNumberSpeculation(value))
            return withTypeAndConversion(Array::Double, Array::Convert);
        return withTypeAndConversion(Array::Contiguous, Array::Convert);
        
    case Array::Int32:
        if (!value || isInt32Speculation(value))
            return *this;
        if (isNumberSpeculation(value))
            return withTypeAndConversion(Array::Double, Array::Convert);
        return withTypeAndConversion(Array::Contiguous, Array::Convert);
        
    case Array::Double:
        if (flags & NodeUsedAsInt)
            return withTypeAndConversion(Array::Contiguous, Array::RageConvert);
        if (!value || isNumberSpeculation(value))
            return *this;
        return withTypeAndConversion(Array::Contiguous, Array::Convert);
        
    case Array::Contiguous:
        if (doesConversion() && (flags & NodeUsedAsInt))
            return withConversion(Array::RageConvert);
        return *this;
        
    case Array::SelectUsingPredictions:
        base &= ~SpecOther;
        
        if (isStringSpeculation(base))
            return ArrayMode(Array::String);
        
        if (isArgumentsSpeculation(base))
            return ArrayMode(Array::Arguments);
        
        if (isInt8ArraySpeculation(base))
            return ArrayMode(Array::Int8Array);
        
        if (isInt16ArraySpeculation(base))
            return ArrayMode(Array::Int16Array);
        
        if (isInt32ArraySpeculation(base))
            return ArrayMode(Array::Int32Array);
        
        if (isUint8ArraySpeculation(base))
            return ArrayMode(Array::Uint8Array);
        
        if (isUint8ClampedArraySpeculation(base))
            return ArrayMode(Array::Uint8ClampedArray);
        
        if (isUint16ArraySpeculation(base))
            return ArrayMode(Array::Uint16Array);
        
        if (isUint32ArraySpeculation(base))
            return ArrayMode(Array::Uint32Array);
        
        if (isFloat32ArraySpeculation(base))
            return ArrayMode(Array::Float32Array);
        
        if (isFloat64ArraySpeculation(base))
            return ArrayMode(Array::Float64Array);

        return ArrayMode(Array::Generic);

    default:
        return *this;
    }
}

Structure* ArrayMode::originalArrayStructure(Graph& graph, const CodeOrigin& codeOrigin) const
{
    if (!isJSArrayWithOriginalStructure())
        return 0;
    
    JSGlobalObject* globalObject = graph.globalObjectFor(codeOrigin);
    
    switch (type()) {
    case Array::Int32:
        return globalObject->originalArrayStructureForIndexingType(ArrayWithInt32);
    case Array::Double:
        return globalObject->originalArrayStructureForIndexingType(ArrayWithDouble);
    case Array::Contiguous:
        return globalObject->originalArrayStructureForIndexingType(ArrayWithContiguous);
    case Array::ArrayStorage:
        return globalObject->originalArrayStructureForIndexingType(ArrayWithArrayStorage);
    default:
        CRASH();
        return 0;
    }
}

Structure* ArrayMode::originalArrayStructure(Graph& graph, Node* node) const
{
    return originalArrayStructure(graph, node->codeOrigin);
}

bool ArrayMode::alreadyChecked(Graph& graph, Node* node, AbstractValue& value, IndexingType shape) const
{
    switch (arrayClass()) {
    case Array::OriginalArray:
        return value.m_currentKnownStructure.hasSingleton()
            && (value.m_currentKnownStructure.singleton()->indexingType() & IndexingShapeMask) == shape
            && (value.m_currentKnownStructure.singleton()->indexingType() & IsArray)
            && graph.globalObjectFor(node->codeOrigin)->isOriginalArrayStructure(value.m_currentKnownStructure.singleton());
        
    case Array::Array:
        if (arrayModesAlreadyChecked(value.m_arrayModes, asArrayModes(shape | IsArray)))
            return true;
        return value.m_currentKnownStructure.hasSingleton()
            && (value.m_currentKnownStructure.singleton()->indexingType() & IndexingShapeMask) == shape
            && (value.m_currentKnownStructure.singleton()->indexingType() & IsArray);
        
    default:
        if (arrayModesAlreadyChecked(value.m_arrayModes, asArrayModes(shape) | asArrayModes(shape | IsArray)))
            return true;
        return value.m_currentKnownStructure.hasSingleton()
            && (value.m_currentKnownStructure.singleton()->indexingType() & IndexingShapeMask) == shape;
    }
}

bool ArrayMode::alreadyChecked(Graph& graph, Node* node, AbstractValue& value) const
{
    switch (type()) {
    case Array::Generic:
        return true;
        
    case Array::ForceExit:
        return false;
        
    case Array::String:
        return speculationChecked(value.m_type, SpecString);
        
    case Array::Int32:
        return alreadyChecked(graph, node, value, Int32Shape);
        
    case Array::Double:
        return alreadyChecked(graph, node, value, DoubleShape);
        
    case Array::Contiguous:
        return alreadyChecked(graph, node, value, ContiguousShape);
        
    case Array::ArrayStorage:
        return alreadyChecked(graph, node, value, ArrayStorageShape);
        
    case Array::SlowPutArrayStorage:
        switch (arrayClass()) {
        case Array::OriginalArray:
            CRASH();
            return false;
        
        case Array::Array:
            if (arrayModesAlreadyChecked(value.m_arrayModes, asArrayModes(ArrayWithArrayStorage) | asArrayModes(ArrayWithSlowPutArrayStorage)))
                return true;
            return value.m_currentKnownStructure.hasSingleton()
                && hasArrayStorage(value.m_currentKnownStructure.singleton()->indexingType())
                && (value.m_currentKnownStructure.singleton()->indexingType() & IsArray);
        
        default:
            if (arrayModesAlreadyChecked(value.m_arrayModes, asArrayModes(NonArrayWithArrayStorage) | asArrayModes(ArrayWithArrayStorage) | asArrayModes(NonArrayWithSlowPutArrayStorage) | asArrayModes(ArrayWithSlowPutArrayStorage)))
                return true;
            return value.m_currentKnownStructure.hasSingleton()
                && hasArrayStorage(value.m_currentKnownStructure.singleton()->indexingType());
        }
        
    case Array::Arguments:
        return speculationChecked(value.m_type, SpecArguments);
        
    case Array::Int8Array:
        return speculationChecked(value.m_type, SpecInt8Array);
        
    case Array::Int16Array:
        return speculationChecked(value.m_type, SpecInt16Array);
        
    case Array::Int32Array:
        return speculationChecked(value.m_type, SpecInt32Array);
        
    case Array::Uint8Array:
        return speculationChecked(value.m_type, SpecUint8Array);
        
    case Array::Uint8ClampedArray:
        return speculationChecked(value.m_type, SpecUint8ClampedArray);
        
    case Array::Uint16Array:
        return speculationChecked(value.m_type, SpecUint16Array);
        
    case Array::Uint32Array:
        return speculationChecked(value.m_type, SpecUint32Array);

    case Array::Float32Array:
        return speculationChecked(value.m_type, SpecFloat32Array);

    case Array::Float64Array:
        return speculationChecked(value.m_type, SpecFloat64Array);
        
    case Array::SelectUsingPredictions:
    case Array::Unprofiled:
    case Array::Undecided:
        break;
    }
    
    CRASH();
    return false;
}

const char* arrayTypeToString(Array::Type type)
{
    switch (type) {
    case Array::SelectUsingPredictions:
        return "SelectUsingPredictions";
    case Array::Unprofiled:
        return "Unprofiled";
    case Array::Generic:
        return "Generic";
    case Array::ForceExit:
        return "ForceExit";
    case Array::String:
        return "String";
    case Array::Undecided:
        return "Undecided";
    case Array::Int32:
        return "Int32";
    case Array::Double:
        return "Double";
    case Array::Contiguous:
        return "Contiguous";
    case Array::ArrayStorage:
        return "ArrayStorage";
    case Array::SlowPutArrayStorage:
        return "SlowPutArrayStorage";
    case Array::Arguments:
        return "Arguments";
    case Array::Int8Array:
        return "Int8Array";
    case Array::Int16Array:
        return "Int16Array";
    case Array::Int32Array:
        return "Int32Array";
    case Array::Uint8Array:
        return "Uint8Array";
    case Array::Uint8ClampedArray:
        return "Uint8ClampedArray";
    case Array::Uint16Array:
        return "Uint16Array";
    case Array::Uint32Array:
        return "Uint32Array";
    case Array::Float32Array:
        return "Float32Array";
    case Array::Float64Array:
        return "Float64Array";
    default:
        // Better to return something then it is to crash. Remember, this method
        // is being called from our main diagnostic tool, the IR dumper. It's like
        // a stack trace. So if we get here then probably something has already
        // gone wrong.
        return "Unknown!";
    }
}

const char* arrayClassToString(Array::Class arrayClass)
{
    switch (arrayClass) {
    case Array::Array:
        return "Array";
    case Array::OriginalArray:
        return "OriginalArray";
    case Array::NonArray:
        return "NonArray";
    case Array::PossiblyArray:
        return "PossiblyArray";
    default:
        return "Unknown!";
    }
}

const char* arraySpeculationToString(Array::Speculation speculation)
{
    switch (speculation) {
    case Array::SaneChain:
        return "SaneChain";
    case Array::InBounds:
        return "InBounds";
    case Array::ToHole:
        return "ToHole";
    case Array::OutOfBounds:
        return "OutOfBounds";
    default:
        return "Unknown!";
    }
}

const char* arrayConversionToString(Array::Conversion conversion)
{
    switch (conversion) {
    case Array::AsIs:
        return "AsIs";
    case Array::Convert:
        return "Convert";
    case Array::RageConvert:
        return "RageConvert";
    default:
        return "Unknown!";
    }
}

void ArrayMode::dump(PrintStream& out) const
{
    out.print(type(), arrayClass(), speculation(), conversion());
}

} } // namespace JSC::DFG

namespace WTF {

void printInternal(PrintStream& out, JSC::DFG::Array::Type type)
{
    out.print(JSC::DFG::arrayTypeToString(type));
}

void printInternal(PrintStream& out, JSC::DFG::Array::Class arrayClass)
{
    out.print(JSC::DFG::arrayClassToString(arrayClass));
}

void printInternal(PrintStream& out, JSC::DFG::Array::Speculation speculation)
{
    out.print(JSC::DFG::arraySpeculationToString(speculation));
}

void printInternal(PrintStream& out, JSC::DFG::Array::Conversion conversion)
{
    out.print(JSC::DFG::arrayConversionToString(conversion));
}

} // namespace WTF

#endif // ENABLE(DFG_JIT)

