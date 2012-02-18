/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

#include "config.h"

#include "HandleStack.h"

#include "MarkStack.h"

namespace JSC {

HandleStack::HandleStack()
#ifndef NDEBUG
    : m_scopeDepth(0)
#endif
{
    grow();
}

void HandleStack::mark(HeapRootVisitor& heapRootMarker)
{
    const Vector<HandleSlot>& blocks = m_blockStack.blocks();
    size_t blockLength = m_blockStack.blockLength;

    int end = blocks.size() - 1;
    for (int i = 0; i < end; ++i) {
        HandleSlot block = blocks[i];
        heapRootMarker.mark(block, blockLength);
    }
    HandleSlot block = blocks[end];
    heapRootMarker.mark(block, m_frame.m_next - block);
}

void HandleStack::grow()
{
    HandleSlot block = m_blockStack.grow();
    m_frame.m_next = block;
    m_frame.m_end = block + m_blockStack.blockLength;
}

}
