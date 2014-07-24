/*
 * Copyright (C) 2011, 2013 Apple Inc. All rights reserved.
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

#ifndef DFGValueSource_h
#define DFGValueSource_h

#include <wtf/Platform.h>

#if ENABLE(DFG_JIT)

#include "DFGCommon.h"
#include "DFGMinifiedID.h"
#include "DataFormat.h"
#include "SpeculatedType.h"
#include "ValueRecovery.h"

namespace JSC { namespace DFG {

enum ValueSourceKind {
    SourceNotSet,
    ValueInJSStack,
    Int32InJSStack,
    CellInJSStack,
    BooleanInJSStack,
    DoubleInJSStack,
    ArgumentsSource,
    SourceIsDead,
    HaveNode
};

static inline ValueSourceKind dataFormatToValueSourceKind(DataFormat dataFormat)
{
    switch (dataFormat) {
    case DataFormatInteger:
        return Int32InJSStack;
    case DataFormatDouble:
        return DoubleInJSStack;
    case DataFormatBoolean:
        return BooleanInJSStack;
    case DataFormatCell:
        return CellInJSStack;
    case DataFormatDead:
        return SourceIsDead;
    case DataFormatArguments:
        return ArgumentsSource;
    default:
        RELEASE_ASSERT(dataFormat & DataFormatJS);
        return ValueInJSStack;
    }
}

static inline DataFormat valueSourceKindToDataFormat(ValueSourceKind kind)
{
    switch (kind) {
    case ValueInJSStack:
        return DataFormatJS;
    case Int32InJSStack:
        return DataFormatInteger;
    case CellInJSStack:
        return DataFormatCell;
    case BooleanInJSStack:
        return DataFormatBoolean;
    case DoubleInJSStack:
        return DataFormatDouble;
    case ArgumentsSource:
        return DataFormatArguments;
    case SourceIsDead:
        return DataFormatDead;
    default:
        return DataFormatNone;
    }
}

static inline bool isInJSStack(ValueSourceKind kind)
{
    DataFormat format = valueSourceKindToDataFormat(kind);
    return format != DataFormatNone && format < DataFormatOSRMarker;
}

// Can this value be recovered without having to look at register allocation state or
// DFG node liveness?
static inline bool isTriviallyRecoverable(ValueSourceKind kind)
{
    return valueSourceKindToDataFormat(kind) != DataFormatNone;
}

class ValueSource {
public:
    ValueSource()
        : m_value(idFromKind(SourceNotSet))
    {
    }
    
    explicit ValueSource(ValueSourceKind valueSourceKind)
        : m_value(idFromKind(valueSourceKind))
    {
        ASSERT(kind() != SourceNotSet);
        ASSERT(kind() != HaveNode);
    }
    
    explicit ValueSource(MinifiedID id)
        : m_value(id)
    {
        ASSERT(!!id);
        ASSERT(kind() == HaveNode);
    }
    
    static ValueSource forSpeculation(SpeculatedType prediction)
    {
        if (isInt32Speculation(prediction))
            return ValueSource(Int32InJSStack);
        if (isArraySpeculation(prediction) || isCellSpeculation(prediction))
            return ValueSource(CellInJSStack);
        if (isBooleanSpeculation(prediction))
            return ValueSource(BooleanInJSStack);
        return ValueSource(ValueInJSStack);
    }
    
    static ValueSource forDataFormat(DataFormat dataFormat)
    {
        return ValueSource(dataFormatToValueSourceKind(dataFormat));
    }
    
    bool isSet() const
    {
        return kindFromID(m_value) != SourceNotSet;
    }
    
    ValueSourceKind kind() const
    {
        return kindFromID(m_value);
    }
    
    bool isInJSStack() const { return JSC::DFG::isInJSStack(kind()); }
    bool isTriviallyRecoverable() const { return JSC::DFG::isTriviallyRecoverable(kind()); }
    
    DataFormat dataFormat() const
    {
        return valueSourceKindToDataFormat(kind());
    }
    
    ValueRecovery valueRecovery() const
    {
        ASSERT(isTriviallyRecoverable());
        switch (kind()) {
        case ValueInJSStack:
            return ValueRecovery::alreadyInJSStack();
            
        case Int32InJSStack:
            return ValueRecovery::alreadyInJSStackAsUnboxedInt32();
            
        case CellInJSStack:
            return ValueRecovery::alreadyInJSStackAsUnboxedCell();
            
        case BooleanInJSStack:
            return ValueRecovery::alreadyInJSStackAsUnboxedBoolean();
            
        case DoubleInJSStack:
            return ValueRecovery::alreadyInJSStackAsUnboxedDouble();
            
        case SourceIsDead:
            return ValueRecovery::constant(jsUndefined());
            
        case ArgumentsSource:
            return ValueRecovery::argumentsThatWereNotCreated();
            
        default:
            RELEASE_ASSERT_NOT_REACHED();
            return ValueRecovery();
        }
    }
    
    MinifiedID id() const
    {
        ASSERT(kind() == HaveNode);
        return m_value;
    }
    
    void dump(PrintStream&) const;
    
private:
    static MinifiedID idFromKind(ValueSourceKind kind)
    {
        ASSERT(kind >= SourceNotSet && kind < HaveNode);
        return MinifiedID::fromBits(MinifiedID::invalidID() - kind);
    }
    
    static ValueSourceKind kindFromID(MinifiedID id)
    {
        uintptr_t kind = static_cast<uintptr_t>(MinifiedID::invalidID() - id.m_id);
        if (kind >= static_cast<uintptr_t>(HaveNode))
            return HaveNode;
        return static_cast<ValueSourceKind>(kind);
    }
    
    MinifiedID m_value;
};

} } // namespace JSC::DFG

#endif // ENABLE(DFG_JIT)

#endif // DFGValueSource_h

