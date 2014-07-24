/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef HandleBlockInlines_h
#define HandleBlockInlines_h

#include "BlockAllocator.h"
#include "HandleBlock.h"

namespace JSC {

inline HandleBlock* HandleBlock::create(DeadBlock* block, HandleSet* handleSet)
{
    Region* region = block->region();
    return new (NotNull, block) HandleBlock(region, handleSet);
}

inline HandleBlock::HandleBlock(Region* region, HandleSet* handleSet)
    : HeapBlock<HandleBlock>(region)
    , m_handleSet(handleSet)
{
}

inline char* HandleBlock::payloadEnd()
{
    return reinterpret_cast<char*>(this) + region()->blockSize();
}

inline char* HandleBlock::payload()
{
    return reinterpret_cast<char*>(this) + WTF::roundUpToMultipleOf<sizeof(double)>(sizeof(HandleBlock));
}

inline HandleNode* HandleBlock::nodes()
{
    return reinterpret_cast_ptr<HandleNode*>(payload());
}

inline HandleNode* HandleBlock::nodeAtIndex(unsigned i)
{
    ASSERT(i < nodeCapacity());
    return &nodes()[i];
}

inline unsigned HandleBlock::nodeCapacity()
{
    return (payloadEnd() - payload()) / sizeof(HandleNode);
}

} // namespace JSC
    
#endif // HandleBlockInlines_h
