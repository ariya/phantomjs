/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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
#include "DFGCodeBlocks.h"

#include "CodeBlock.h"
#include "SlotVisitor.h"
#include <wtf/Vector.h>

namespace JSC {

#if ENABLE(DFG_JIT)

DFGCodeBlocks::DFGCodeBlocks() { }

DFGCodeBlocks::~DFGCodeBlocks()
{
    Vector<OwnPtr<CodeBlock>, 16> toRemove;
    
    for (HashSet<CodeBlock*>::iterator iter = m_set.begin(); iter != m_set.end(); ++iter) {
        if ((*iter)->m_dfgData->isJettisoned)
            toRemove.append(adoptPtr(*iter));
    }
}

void DFGCodeBlocks::jettison(PassOwnPtr<CodeBlock> codeBlockPtr)
{
    // We don't want to delete it now; we just want its pointer.
    CodeBlock* codeBlock = codeBlockPtr.leakPtr();
    
    ASSERT(codeBlock);
    ASSERT(codeBlock->getJITType() == JITCode::DFGJIT);
    
    // It should not have already been jettisoned.
    ASSERT(!codeBlock->m_dfgData->isJettisoned);

    // We should have this block already.
    ASSERT(m_set.find(codeBlock) != m_set.end());
    
    codeBlock->m_dfgData->isJettisoned = true;
}

void DFGCodeBlocks::clearMarks()
{
    for (HashSet<CodeBlock*>::iterator iter = m_set.begin(); iter != m_set.end(); ++iter) {
        (*iter)->m_dfgData->mayBeExecuting = false;
        (*iter)->m_dfgData->visitAggregateHasBeenCalled = false;
    }
}

void DFGCodeBlocks::deleteUnmarkedJettisonedCodeBlocks()
{
    Vector<OwnPtr<CodeBlock>, 16> toRemove;
    
    for (HashSet<CodeBlock*>::iterator iter = m_set.begin(); iter != m_set.end(); ++iter) {
        if ((*iter)->m_dfgData->isJettisoned && !(*iter)->m_dfgData->mayBeExecuting)
            toRemove.append(adoptPtr(*iter));
    }
}

void DFGCodeBlocks::traceMarkedCodeBlocks(SlotVisitor& visitor)
{
    for (HashSet<CodeBlock*>::iterator iter = m_set.begin(); iter != m_set.end(); ++iter) {
        if ((*iter)->m_dfgData->mayBeExecuting)
            (*iter)->visitAggregate(visitor);
    }
}

#else // ENABLE(DFG_JIT)

void DFGCodeBlocks::jettison(PassOwnPtr<CodeBlock>)
{
}

#endif // ENABLE(DFG_JIT)

} // namespace JSC


