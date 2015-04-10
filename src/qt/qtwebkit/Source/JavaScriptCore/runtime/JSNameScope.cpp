/*
 * Copyright (C) 2008, 2009, 2012 Apple Inc. All Rights Reserved.
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
#include "JSNameScope.h"

#include "Error.h"
#include "Operations.h"

namespace JSC {

const ClassInfo JSNameScope::s_info = { "NameScope", &Base::s_info, 0, 0, CREATE_METHOD_TABLE(JSNameScope) };

void JSNameScope::visitChildren(JSCell* cell, SlotVisitor& visitor)
{
    JSNameScope* thisObject = jsCast<JSNameScope*>(cell);
    ASSERT_GC_OBJECT_INHERITS(thisObject, &s_info);
    COMPILE_ASSERT(StructureFlags & OverridesVisitChildren, OverridesVisitChildrenWithoutSettingFlag);
    ASSERT(thisObject->structure()->typeInfo().overridesVisitChildren());

    Base::visitChildren(thisObject, visitor);
    visitor.append(&thisObject->m_registerStore);
}

JSObject* JSNameScope::toThisObject(JSCell*, ExecState* exec)
{
    return exec->globalThisValue();
}

void JSNameScope::put(JSCell* cell, ExecState* exec, PropertyName propertyName, JSValue value, PutPropertySlot& slot)
{
    JSNameScope* thisObject = jsCast<JSNameScope*>(cell);
    if (slot.isStrictMode()) {
        // Double lookup in strict mode, but this only occurs when
        // a) indirectly writing to an exception slot
        // b) writing to a function expression name
        // (a) is unlikely, and (b) is an error.
        // Also with a single entry the symbol table lookup should simply be
        // a pointer compare.
        PropertySlot slot;
        bool isWritable = true;
        symbolTableGet(thisObject, propertyName, slot, isWritable);
        if (!isWritable) {
            throwError(exec, createTypeError(exec, StrictModeReadonlyPropertyWriteError));
            return;
        }
    }
    if (symbolTablePut(thisObject, exec, propertyName, value, slot.isStrictMode()))
        return;
    
    RELEASE_ASSERT_NOT_REACHED();
}

bool JSNameScope::getOwnPropertySlot(JSCell* cell, ExecState*, PropertyName propertyName, PropertySlot& slot)
{
    return symbolTableGet(jsCast<JSNameScope*>(cell), propertyName, slot);
}

} // namespace JSC
