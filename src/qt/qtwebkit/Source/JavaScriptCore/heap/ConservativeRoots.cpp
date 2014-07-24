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
#include "ConservativeRoots.h"

#include "CodeBlock.h"
#include "CopiedSpace.h"
#include "CopiedSpaceInlines.h"
#include "DFGCodeBlocks.h"
#include "JSCell.h"
#include "JSObject.h"
#include "Structure.h"

namespace JSC {

ConservativeRoots::ConservativeRoots(const MarkedBlockSet* blocks, CopiedSpace* copiedSpace)
    : m_roots(m_inlineRoots)
    , m_size(0)
    , m_capacity(inlineCapacity)
    , m_blocks(blocks)
    , m_copiedSpace(copiedSpace)
{
}

ConservativeRoots::~ConservativeRoots()
{
    if (m_roots != m_inlineRoots)
        OSAllocator::decommitAndRelease(m_roots, m_capacity * sizeof(JSCell*));
}

void ConservativeRoots::grow()
{
    size_t newCapacity = m_capacity == inlineCapacity ? nonInlineCapacity : m_capacity * 2;
    JSCell** newRoots = static_cast<JSCell**>(OSAllocator::reserveAndCommit(newCapacity * sizeof(JSCell*)));
    memcpy(newRoots, m_roots, m_size * sizeof(JSCell*));
    if (m_roots != m_inlineRoots)
        OSAllocator::decommitAndRelease(m_roots, m_capacity * sizeof(JSCell*));
    m_capacity = newCapacity;
    m_roots = newRoots;
}

template<typename MarkHook>
inline void ConservativeRoots::genericAddPointer(void* p, TinyBloomFilter filter, MarkHook& markHook)
{
    markHook.mark(p);

    m_copiedSpace->pinIfNecessary(p);
    
    MarkedBlock* candidate = MarkedBlock::blockFor(p);
    if (filter.ruleOut(reinterpret_cast<Bits>(candidate))) {
        ASSERT(!candidate || !m_blocks->set().contains(candidate));
        return;
    }

    if (!MarkedBlock::isAtomAligned(p))
        return;

    if (!m_blocks->set().contains(candidate))
        return;

    if (!candidate->isLiveCell(p))
        return;

    if (m_size == m_capacity)
        grow();

    m_roots[m_size++] = static_cast<JSCell*>(p);
}

template<typename MarkHook>
void ConservativeRoots::genericAddSpan(void* begin, void* end, MarkHook& markHook)
{
    ASSERT(begin <= end);
    ASSERT((static_cast<char*>(end) - static_cast<char*>(begin)) < 0x1000000);
    ASSERT(isPointerAligned(begin));
    ASSERT(isPointerAligned(end));

    TinyBloomFilter filter = m_blocks->filter(); // Make a local copy of filter to show the compiler it won't alias, and can be register-allocated.
    for (char** it = static_cast<char**>(begin); it != static_cast<char**>(end); ++it)
        genericAddPointer(*it, filter, markHook);
}

class DummyMarkHook {
public:
    void mark(void*) { }
};

void ConservativeRoots::add(void* begin, void* end)
{
    DummyMarkHook dummy;
    genericAddSpan(begin, end, dummy);
}

void ConservativeRoots::add(void* begin, void* end, JITStubRoutineSet& jitStubRoutines)
{
    genericAddSpan(begin, end, jitStubRoutines);
}

template<typename T, typename U>
class CompositeMarkHook {
public:
    CompositeMarkHook(T& first, U& second)
        : m_first(first)
        , m_second(second)
    {
    }
    
    void mark(void* address)
    {
        m_first.mark(address);
        m_second.mark(address);
    }

private:
    T& m_first;
    U& m_second;
};

void ConservativeRoots::add(
    void* begin, void* end, JITStubRoutineSet& jitStubRoutines, DFGCodeBlocks& dfgCodeBlocks)
{
    CompositeMarkHook<JITStubRoutineSet, DFGCodeBlocks> markHook(
        jitStubRoutines, dfgCodeBlocks);
    genericAddSpan(begin, end, markHook);
}

} // namespace JSC
