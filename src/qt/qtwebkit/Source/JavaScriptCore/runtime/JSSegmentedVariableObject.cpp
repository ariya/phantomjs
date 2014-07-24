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
#include "JSSegmentedVariableObject.h"

#include "Operations.h"

namespace JSC {

int JSSegmentedVariableObject::findRegisterIndex(void* registerAddress)
{
    for (int i = m_registers.size(); i--;) {
        if (&m_registers[i] != registerAddress)
            continue;
        return i;
    }
    CRASH();
    return -1;
}

int JSSegmentedVariableObject::addRegisters(int numberOfRegistersToAdd)
{
    ASSERT(numberOfRegistersToAdd >= 0);
    
    size_t oldSize = m_registers.size();
    m_registers.grow(oldSize + numberOfRegistersToAdd);
    
    for (size_t i = numberOfRegistersToAdd; i--;)
        m_registers[oldSize + i].setWithoutWriteBarrier(jsUndefined());
    
    return static_cast<int>(oldSize);
}

void JSSegmentedVariableObject::visitChildren(JSCell* cell, SlotVisitor& slotVisitor)
{
    JSSegmentedVariableObject* thisObject = jsCast<JSSegmentedVariableObject*>(cell);
    ASSERT_GC_OBJECT_INHERITS(thisObject, &s_info);
    COMPILE_ASSERT(StructureFlags & OverridesVisitChildren, OverridesVisitChildrenWithoutSettingFlag);
    ASSERT(thisObject->structure()->typeInfo().overridesVisitChildren());
    JSSymbolTableObject::visitChildren(thisObject, slotVisitor);
    
    for (unsigned i = thisObject->m_registers.size(); i--;)
        slotVisitor.append(&thisObject->m_registers[i]);
}

} // namespace JSC

