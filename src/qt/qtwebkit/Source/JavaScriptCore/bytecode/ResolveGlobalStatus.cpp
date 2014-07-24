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
#include "ResolveGlobalStatus.h"

#include "CodeBlock.h"
#include "JSCJSValue.h"
#include "Operations.h"
#include "Structure.h"

namespace JSC {

static ResolveGlobalStatus computeForStructure(CodeBlock* codeBlock, Structure* structure, Identifier& identifier)
{
    unsigned attributesIgnored;
    JSCell* specificValue;
    PropertyOffset offset = structure->get(*codeBlock->vm(), identifier, attributesIgnored, specificValue);
    if (structure->isDictionary())
        specificValue = 0;
    if (!isValidOffset(offset))
        return ResolveGlobalStatus();
    
    return ResolveGlobalStatus(ResolveGlobalStatus::Simple, structure, offset, specificValue);
}

ResolveGlobalStatus ResolveGlobalStatus::computeFor(CodeBlock* codeBlock, int, ResolveOperation* operation, Identifier& identifier)
{
    ASSERT(operation->m_operation == ResolveOperation::GetAndReturnGlobalProperty);
    if (!operation->m_structure)
        return ResolveGlobalStatus();
    
    return computeForStructure(codeBlock, operation->m_structure.get(), identifier);
}

} // namespace JSC

