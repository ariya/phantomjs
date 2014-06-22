/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "JSSymbolTableObject.h"

#include "JSActivation.h"
#include "JSGlobalObject.h"
#include "JSNameScope.h"
#include "Operations.h"
#include "PropertyNameArray.h"

namespace JSC {

void JSSymbolTableObject::visitChildren(JSCell* cell, SlotVisitor& visitor)
{
    JSSymbolTableObject* thisObject = jsCast<JSSymbolTableObject*>(cell);
    ASSERT_GC_OBJECT_INHERITS(thisObject, &s_info);
    COMPILE_ASSERT(StructureFlags & OverridesVisitChildren, OverridesVisitChildrenWithoutSettingFlag);
    ASSERT(thisObject->structure()->typeInfo().overridesVisitChildren());

    Base::visitChildren(thisObject, visitor);
    visitor.append(&thisObject->m_symbolTable);
}

bool JSSymbolTableObject::deleteProperty(JSCell* cell, ExecState* exec, PropertyName propertyName)
{
    JSSymbolTableObject* thisObject = jsCast<JSSymbolTableObject*>(cell);
    if (thisObject->symbolTable()->contains(propertyName.publicName()))
        return false;

    return JSObject::deleteProperty(thisObject, exec, propertyName);
}

void JSSymbolTableObject::getOwnNonIndexPropertyNames(JSObject* object, ExecState* exec, PropertyNameArray& propertyNames, EnumerationMode mode)
{
    JSSymbolTableObject* thisObject = jsCast<JSSymbolTableObject*>(object);
    SymbolTable::const_iterator end = thisObject->symbolTable()->end();
    for (SymbolTable::const_iterator it = thisObject->symbolTable()->begin(); it != end; ++it) {
        if (!(it->value.getAttributes() & DontEnum) || (mode == IncludeDontEnumProperties))
            propertyNames.add(Identifier(exec, it->key.get()));
    }
    
    JSObject::getOwnNonIndexPropertyNames(thisObject, exec, propertyNames, mode);
}

void JSSymbolTableObject::putDirectVirtual(JSObject*, ExecState*, PropertyName, JSValue, unsigned)
{
    RELEASE_ASSERT_NOT_REACHED();
}

} // namespace JSC
