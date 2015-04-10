/*
 * Copyright (C) 2008, 2009 Apple Inc. All Rights Reserved.
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

#ifndef JSNameScope_h
#define JSNameScope_h

#include "JSGlobalObject.h"
#include "JSVariableObject.h"

namespace JSC {

// Used for scopes with a single named variable: catch and named function expression.
class JSNameScope : public JSVariableObject {
public:
    typedef JSVariableObject Base;

    static JSNameScope* create(ExecState* exec, const Identifier& identifier, JSValue value, unsigned attributes)
    {
        JSNameScope* scopeObject = new (NotNull, allocateCell<JSNameScope>(*exec->heap())) JSNameScope(exec, exec->scope());
        scopeObject->finishCreation(exec, identifier, value, attributes);
        return scopeObject;
    }

    static JSNameScope* create(ExecState* exec, const Identifier& identifier, JSValue value, unsigned attributes, JSScope* next)
    {
        JSNameScope* scopeObject = new (NotNull, allocateCell<JSNameScope>(*exec->heap())) JSNameScope(exec, next);
        scopeObject->finishCreation(exec, identifier, value, attributes);
        return scopeObject;
    }

    static void visitChildren(JSCell*, SlotVisitor&);
    bool isDynamicScope(bool& requiresDynamicChecks) const;
    static JSObject* toThisObject(JSCell*, ExecState*);
    static bool getOwnPropertySlot(JSCell*, ExecState*, PropertyName, PropertySlot&);
    static void put(JSCell*, ExecState*, PropertyName, JSValue, PutPropertySlot&);

    static Structure* createStructure(VM& vm, JSGlobalObject* globalObject, JSValue proto) { return Structure::create(vm, globalObject, proto, TypeInfo(NameScopeObjectType, StructureFlags), &s_info); }

    static const ClassInfo s_info;

protected:
    void finishCreation(ExecState* exec, const Identifier& identifier, JSValue value, unsigned attributes)
    {
        Base::finishCreation(exec->vm());
        m_registerStore.set(exec->vm(), this, value);
        symbolTable()->add(identifier.impl(), SymbolTableEntry(-1, attributes));
    }

    static const unsigned StructureFlags = OverridesGetOwnPropertySlot | OverridesVisitChildren | Base::StructureFlags;

private:
    JSNameScope(ExecState* exec, JSScope* next)
        : Base(
            exec->vm(),
            exec->lexicalGlobalObject()->nameScopeStructure(),
            reinterpret_cast<Register*>(&m_registerStore + 1),
            next
        )
    {
    }

    WriteBarrier<Unknown> m_registerStore;
};

inline bool JSNameScope::isDynamicScope(bool&) const
{
    return false;
}

}

#endif // JSNameScope_h
