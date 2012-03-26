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

#ifndef JSStaticScopeObject_h
#define JSStaticScopeObject_h

#include "JSVariableObject.h"

namespace JSC{
    
    class JSStaticScopeObject : public JSVariableObject {
    public:
        JSStaticScopeObject(ExecState* exec, const Identifier& ident, JSValue value, unsigned attributes)
            : JSVariableObject(exec->globalData(), exec->globalData().staticScopeStructure.get(), &m_symbolTable, reinterpret_cast<Register*>(&m_registerStore + 1))
        {
            m_registerStore.set(exec->globalData(), this, value);
            symbolTable().add(ident.impl(), SymbolTableEntry(-1, attributes));
        }

        virtual void visitChildren(SlotVisitor&);
        bool isDynamicScope(bool& requiresDynamicChecks) const;
        virtual JSObject* toThisObject(ExecState*) const;
        virtual JSValue toStrictThisObject(ExecState*) const;
        virtual bool getOwnPropertySlot(ExecState*, const Identifier&, PropertySlot&);
        virtual void put(ExecState*, const Identifier&, JSValue, PutPropertySlot&);
        void putWithAttributes(ExecState*, const Identifier&, JSValue, unsigned attributes);

        static Structure* createStructure(JSGlobalData& globalData, JSValue proto) { return Structure::create(globalData, proto, TypeInfo(ObjectType, StructureFlags), AnonymousSlotCount, &s_info); }

    protected:
        static const unsigned StructureFlags = OverridesGetOwnPropertySlot | NeedsThisConversion | OverridesVisitChildren | OverridesGetPropertyNames | JSVariableObject::StructureFlags;

    private:
        SymbolTable m_symbolTable;
        WriteBarrier<Unknown> m_registerStore;
    };

}

#endif // JSStaticScopeObject_h
