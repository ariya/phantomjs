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

#ifndef ResolveOperation_h
#define ResolveOperation_h

#include "PropertyOffset.h"
#include "WriteBarrier.h"

#include <wtf/Vector.h>

namespace JSC {

class Structure;

struct ResolveOperation {
    typedef enum {
        Fail,
        SetBaseToUndefined,
        ReturnScopeAsBase,
        SetBaseToScope,
        SetBaseToGlobal,
        GetAndReturnScopedVar,
        GetAndReturnGlobalVar,
        GetAndReturnGlobalVarWatchable,
        SkipTopScopeNode,
        SkipScopes,
        ReturnGlobalObjectAsBase,
        GetAndReturnGlobalProperty,
        CheckForDynamicEntriesBeforeGlobalScope
    } ResolveOperationType;

    ResolveOperationType m_operation;
    WriteBarrier<Structure> m_structure;
    union {
        PropertyOffset m_offset;
        WriteBarrier<Unknown>* m_registerAddress;
        int m_scopesToSkip;
        int m_activationRegister;
    };
    static ResolveOperation getAndReturnScopedVar(PropertyOffset offset)
    {
        ResolveOperation op;
        op.m_operation = GetAndReturnScopedVar;
        op.m_offset = offset;
        return op;
    }
    static ResolveOperation checkForDynamicEntriesBeforeGlobalScope()
    {
        ResolveOperation op;
        op.m_operation = CheckForDynamicEntriesBeforeGlobalScope;
        return op;
    }

    static ResolveOperation getAndReturnGlobalVar(WriteBarrier<Unknown>* registerAddress, bool couldBeWatched)
    {
        ResolveOperation op;
        op.m_operation = couldBeWatched ? GetAndReturnGlobalVarWatchable : GetAndReturnGlobalVar;
        op.m_registerAddress = registerAddress;
        return op;
    }
    static ResolveOperation getAndReturnGlobalProperty()
    {
        ResolveOperation op;
        op.m_operation = GetAndReturnGlobalProperty;
        return op;
    }
    static ResolveOperation resolveFail()
    {
        ResolveOperation op;
        op.m_operation = Fail;
        return op;
    }
    static ResolveOperation skipTopScopeNode(int activationRegister)
    {
        ResolveOperation op;
        op.m_operation = SkipTopScopeNode;
        op.m_activationRegister = activationRegister;
        return op;
    }
    static ResolveOperation skipScopes(int scopesToSkip)
    {
        ResolveOperation op;
        op.m_operation = SkipScopes;
        op.m_scopesToSkip = scopesToSkip;
        return op;
    }
    static ResolveOperation returnGlobalObjectAsBase()
    {
        ResolveOperation op;
        op.m_operation = ReturnGlobalObjectAsBase;
        return op;
    }
    static ResolveOperation setBaseToGlobal()
    {
        ResolveOperation op;
        op.m_operation = SetBaseToGlobal;
        return op;
    }
    static ResolveOperation setBaseToUndefined()
    {
        ResolveOperation op;
        op.m_operation = SetBaseToUndefined;
        return op;
    }
    static ResolveOperation setBaseToScope()
    {
        ResolveOperation op;
        op.m_operation = SetBaseToScope;
        return op;
    }
    static ResolveOperation returnScopeAsBase()
    {
        ResolveOperation op;
        op.m_operation = ReturnScopeAsBase;
        return op;
    }
};

typedef Vector<ResolveOperation> ResolveOperations;

struct PutToBaseOperation {
    PutToBaseOperation(bool isStrict)
        : m_kind(Uninitialised)
        , m_isDynamic(false)
        , m_isStrict(isStrict)
        , m_predicatePointer(0)
    {

    }
    enum Kind { Uninitialised, Generic, Readonly, GlobalVariablePut, GlobalVariablePutChecked, GlobalPropertyPut, VariablePut };
    union {
        Kind m_kind : 8;
        uint8_t m_kindAsUint8;
    };
    bool m_isDynamic : 8;
    bool m_isStrict : 8;
    union {
        bool* m_predicatePointer;
        unsigned m_scopeDepth;
    };
    WriteBarrier<Structure> m_structure;
    union {
        // Used for GlobalVariablePut
        WriteBarrier<Unknown>* m_registerAddress;

        // Used for GlobalPropertyPut and VariablePut
        struct {
            PropertyOffset m_offset;
            int32_t m_offsetInButterfly;
        };
    };
};
}

#endif // ResolveOperation_h
