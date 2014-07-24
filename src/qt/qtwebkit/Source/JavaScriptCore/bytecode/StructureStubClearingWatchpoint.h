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

#ifndef StructureStubClearingWatchpoint_h
#define StructureStubClearingWatchpoint_h

#include "Watchpoint.h"
#include <wtf/Platform.h>

#if ENABLE(JIT)

#include <wtf/FastAllocBase.h>
#include <wtf/Noncopyable.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>

namespace JSC {

class CodeBlock;
class WatchpointsOnStructureStubInfo;
struct StructureStubInfo;

class StructureStubClearingWatchpoint : public Watchpoint {
    WTF_MAKE_NONCOPYABLE(StructureStubClearingWatchpoint);
    WTF_MAKE_FAST_ALLOCATED;
public:
    StructureStubClearingWatchpoint(
        WatchpointsOnStructureStubInfo& holder)
        : m_holder(holder)
    {
    }
    
    StructureStubClearingWatchpoint(
        WatchpointsOnStructureStubInfo& holder,
        PassOwnPtr<StructureStubClearingWatchpoint> next)
        : m_holder(holder)
        , m_next(next)
    {
    }
    
    virtual ~StructureStubClearingWatchpoint();
    
    static StructureStubClearingWatchpoint* push(
        WatchpointsOnStructureStubInfo& holder,
        OwnPtr<StructureStubClearingWatchpoint>& head);

protected:
    void fireInternal();

private:
    WatchpointsOnStructureStubInfo& m_holder;
    OwnPtr<StructureStubClearingWatchpoint> m_next;
};

class WatchpointsOnStructureStubInfo : public RefCounted<WatchpointsOnStructureStubInfo> {
public:
    WatchpointsOnStructureStubInfo(CodeBlock* codeBlock, StructureStubInfo* stubInfo)
        : m_codeBlock(codeBlock)
        , m_stubInfo(stubInfo)
    {
    }
    
    ~WatchpointsOnStructureStubInfo();
    
    StructureStubClearingWatchpoint* addWatchpoint();
    
    static StructureStubClearingWatchpoint* ensureReferenceAndAddWatchpoint(
        RefPtr<WatchpointsOnStructureStubInfo>& holderRef,
        CodeBlock*, StructureStubInfo*);
    
    CodeBlock* codeBlock() const { return m_codeBlock; }
    StructureStubInfo* stubInfo() const { return m_stubInfo; }
    
private:
    CodeBlock* m_codeBlock;
    StructureStubInfo* m_stubInfo;
    OwnPtr<StructureStubClearingWatchpoint> m_head;
};

} // namespace JSC

#endif // ENABLE(JIT)

#endif // StructureStubClearingWatchpoint_h

