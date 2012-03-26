/*
 * Copyright (C) 2008 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "StructureChain.h"

#include "JSObject.h"
#include "Structure.h"
#include <wtf/RefPtr.h>

namespace JSC {
    
ClassInfo StructureChain::s_info = { "StructureChain", 0, 0, 0 };

StructureChain::StructureChain(JSGlobalData& globalData, Structure* structure, Structure* head)
    : JSCell(globalData, structure)
{
    size_t size = 0;
    for (Structure* current = head; current; current = current->storedPrototype().isNull() ? 0 : asObject(current->storedPrototype())->structure())
        ++size;
    
    m_vector = adoptArrayPtr(new WriteBarrier<Structure>[size + 1]);

    size_t i = 0;
    for (Structure* current = head; current; current = current->storedPrototype().isNull() ? 0 : asObject(current->storedPrototype())->structure())
        m_vector[i++].set(globalData, this, current);
    m_vector[i].clear();
}

StructureChain::~StructureChain()
{
}

void StructureChain::visitChildren(SlotVisitor& visitor)
{
    size_t i = 0;
    while (m_vector[i])
        visitor.append(&m_vector[i++]);
}

} // namespace JSC
