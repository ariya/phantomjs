/*
 * Copyright (C) 2009, 2011 Apple Inc. All rights reserved.
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

#ifndef MarkStackInlines_h
#define MarkStackInlines_h

#include "GCThreadSharedData.h"
#include "MarkStack.h"

namespace JSC {

inline MarkStackSegment* MarkStackSegment::create(DeadBlock* block)
{
    return new (NotNull, block) MarkStackSegment(block->region());
}

inline size_t MarkStackArray::postIncTop()
{
    size_t result = m_top++;
    ASSERT(result == m_segments.head()->m_top++);
    return result;
}

inline size_t MarkStackArray::preDecTop()
{
    size_t result = --m_top;
    ASSERT(result == --m_segments.head()->m_top);
    return result;
}

inline void MarkStackArray::setTopForFullSegment()
{
    ASSERT(m_segments.head()->m_top == s_segmentCapacity);
    m_top = s_segmentCapacity;
}

inline void MarkStackArray::setTopForEmptySegment()
{
    ASSERT(!m_segments.head()->m_top);
    m_top = 0;
}

inline size_t MarkStackArray::top()
{
    ASSERT(m_top == m_segments.head()->m_top);
    return m_top;
}

#if ASSERT_DISABLED
inline void MarkStackArray::validatePrevious() { }
#else
inline void MarkStackArray::validatePrevious()
{
    unsigned count = 0;
    for (MarkStackSegment* current = m_segments.head(); current; current = current->next())
        count++;
    ASSERT(m_segments.size() == m_numberOfSegments);
}
#endif

inline void MarkStackArray::append(const JSCell* cell)
{
    if (m_top == s_segmentCapacity)
        expand();
    m_segments.head()->data()[postIncTop()] = cell;
}

inline bool MarkStackArray::canRemoveLast()
{
    return !!m_top;
}

inline const JSCell* MarkStackArray::removeLast()
{
    return m_segments.head()->data()[preDecTop()];
}

inline bool MarkStackArray::isEmpty()
{
    if (m_top)
        return false;
    if (m_segments.head()->next()) {
        ASSERT(m_segments.head()->next()->m_top == s_segmentCapacity);
        return false;
    }
    return true;
}

inline size_t MarkStackArray::size()
{
    return m_top + s_segmentCapacity * (m_numberOfSegments - 1);
}

} // namespace JSC

#endif // MarkStackInlines_h

