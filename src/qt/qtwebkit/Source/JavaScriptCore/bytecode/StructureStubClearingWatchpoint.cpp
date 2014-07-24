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
#include "StructureStubClearingWatchpoint.h"

#if ENABLE(JIT)

#include "CodeBlock.h"
#include "StructureStubInfo.h"

namespace JSC {

StructureStubClearingWatchpoint::~StructureStubClearingWatchpoint() { }

StructureStubClearingWatchpoint* StructureStubClearingWatchpoint::push(
    WatchpointsOnStructureStubInfo& holder,
    OwnPtr<StructureStubClearingWatchpoint>& head)
{
    head = adoptPtr(new StructureStubClearingWatchpoint(holder, head.release()));
    return head.get();
}

void StructureStubClearingWatchpoint::fireInternal()
{
    // This will implicitly cause my own demise: stub reset removes all watchpoints.
    // That works, because deleting a watchpoint removes it from the set's list, and
    // the set's list traversal for firing is robust against the set changing.
    m_holder.codeBlock()->resetStub(*m_holder.stubInfo());
}

WatchpointsOnStructureStubInfo::~WatchpointsOnStructureStubInfo()
{
}

StructureStubClearingWatchpoint* WatchpointsOnStructureStubInfo::addWatchpoint()
{
    return StructureStubClearingWatchpoint::push(*this, m_head);
}

StructureStubClearingWatchpoint* WatchpointsOnStructureStubInfo::ensureReferenceAndAddWatchpoint(
    RefPtr<WatchpointsOnStructureStubInfo>& holderRef, CodeBlock* codeBlock,
    StructureStubInfo* stubInfo)
{
    if (!holderRef)
        holderRef = adoptRef(new WatchpointsOnStructureStubInfo(codeBlock, stubInfo));
    else {
        ASSERT(holderRef->m_codeBlock == codeBlock);
        ASSERT(holderRef->m_stubInfo == stubInfo);
    }
    
    return holderRef->addWatchpoint();
}

} // namespace JSC

#endif // ENABLE(JIT)

