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

#ifndef CopyWorkList_h
#define CopyWorkList_h

#include <wtf/Vector.h>

namespace JSC {

class JSCell;

class CopyWorkListSegment : public HeapBlock<CopyWorkListSegment> {
public:
    static CopyWorkListSegment* create(DeadBlock* block)
    {
        return new (NotNull, block) CopyWorkListSegment(block->region());
    }

    size_t size() { return m_size; }
    bool isFull() { return reinterpret_cast<char*>(&data()[size()]) >= endOfBlock(); }
    JSCell* get(size_t index) { return data()[index]; }

    void append(JSCell* cell)
    {
        ASSERT(!isFull());
        data()[m_size] = cell;
        m_size += 1;
    }

    static const size_t blockSize = 512;

private:
    CopyWorkListSegment(Region* region)
        : HeapBlock<CopyWorkListSegment>(region)
        , m_size(0)
    {
    }

    JSCell** data() { return reinterpret_cast<JSCell**>(this + 1); }
    char* endOfBlock() { return reinterpret_cast<char*>(this) + blockSize; }

    size_t m_size;
};

class CopyWorkListIterator {
    friend class CopyWorkList;
public:
    JSCell* get() { return m_currentSegment->get(m_currentIndex); }
    JSCell* operator*() { return get(); }
    JSCell* operator->() { return get(); }

    CopyWorkListIterator& operator++()
    {
        m_currentIndex++;

        if (m_currentIndex >= m_currentSegment->size()) {
            m_currentIndex = 0;
            m_currentSegment = m_currentSegment->next();
        
            ASSERT(!m_currentSegment || m_currentSegment->size());
        }

        return *this;
    }

    bool operator==(const CopyWorkListIterator& other) const
    {
        return m_currentSegment == other.m_currentSegment && m_currentIndex == other.m_currentIndex;
    }
    
    bool operator!=(const CopyWorkListIterator& other) const
    {
        return !(*this == other);
    }

    CopyWorkListIterator()
        : m_currentSegment(0)
        , m_currentIndex(0)
    {
    }

private:
    CopyWorkListIterator(CopyWorkListSegment* startSegment, size_t startIndex)
        : m_currentSegment(startSegment)
        , m_currentIndex(startIndex)
    {
    }
    
    CopyWorkListSegment* m_currentSegment;
    size_t m_currentIndex;
};

class CopyWorkList {
public:
    typedef CopyWorkListIterator iterator;

    CopyWorkList(BlockAllocator&);
    ~CopyWorkList();

    void append(JSCell*);
    iterator begin();
    iterator end();

private:
    DoublyLinkedList<CopyWorkListSegment> m_segments;
    BlockAllocator& m_blockAllocator;
};

inline CopyWorkList::CopyWorkList(BlockAllocator& blockAllocator)
    : m_blockAllocator(blockAllocator)
{
}

inline CopyWorkList::~CopyWorkList()
{
    while (!m_segments.isEmpty())
        m_blockAllocator.deallocate(CopyWorkListSegment::destroy(m_segments.removeHead()));
}

inline void CopyWorkList::append(JSCell* cell)
{
    if (m_segments.isEmpty() || m_segments.tail()->isFull())
        m_segments.append(CopyWorkListSegment::create(m_blockAllocator.allocate<CopyWorkListSegment>()));

    ASSERT(!m_segments.tail()->isFull());

    m_segments.tail()->append(cell);
}

inline CopyWorkList::iterator CopyWorkList::begin()
{
    return CopyWorkListIterator(m_segments.head(), 0);
}

inline CopyWorkList::iterator CopyWorkList::end()
{
    return CopyWorkListIterator();
}

} // namespace JSC

#endif // CopyWorkList_h
