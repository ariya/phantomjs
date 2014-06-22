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

#ifndef MarkStack_h
#define MarkStack_h

#if ENABLE(OBJECT_MARK_LOGGING)
#define MARK_LOG_MESSAGE0(message) dataLogF(message)
#define MARK_LOG_MESSAGE1(message, arg1) dataLogF(message, arg1)
#define MARK_LOG_MESSAGE2(message, arg1, arg2) dataLogF(message, arg1, arg2)
#define MARK_LOG_ROOT(visitor, rootName) \
    dataLogF("\n%s: ", rootName); \
    (visitor).resetChildCount()
#define MARK_LOG_PARENT(visitor, parent) \
    dataLogF("\n%p (%s): ", parent, parent->className() ? parent->className() : "unknown"); \
    (visitor).resetChildCount()
#define MARK_LOG_CHILD(visitor, child) \
    if ((visitor).childCount()) \
    dataLogFString(", "); \
    dataLogF("%p", child); \
    (visitor).incrementChildCount()
#else
#define MARK_LOG_MESSAGE0(message) do { } while (false)
#define MARK_LOG_MESSAGE1(message, arg1) do { } while (false)
#define MARK_LOG_MESSAGE2(message, arg1, arg2) do { } while (false)
#define MARK_LOG_ROOT(visitor, rootName) do { } while (false)
#define MARK_LOG_PARENT(visitor, parent) do { } while (false)
#define MARK_LOG_CHILD(visitor, child) do { } while (false)
#endif

#include "HeapBlock.h"
#include <wtf/StdLibExtras.h>

namespace JSC {

class BlockAllocator;
class DeadBlock;
class JSCell;

class MarkStackSegment : public HeapBlock<MarkStackSegment> {
public:
    MarkStackSegment(Region* region)
        : HeapBlock<MarkStackSegment>(region)
#if !ASSERT_DISABLED
        , m_top(0)
#endif
    {
    }

    static MarkStackSegment* create(DeadBlock*);

    const JSCell** data()
    {
        return bitwise_cast<const JSCell**>(this + 1);
    }

    static const size_t blockSize = 4 * KB;

#if !ASSERT_DISABLED
    size_t m_top;
#endif
};

class MarkStackArray {
public:
    MarkStackArray(BlockAllocator&);
    ~MarkStackArray();

    void append(const JSCell*);

    bool canRemoveLast();
    const JSCell* removeLast();
    bool refill();
    
    void donateSomeCellsTo(MarkStackArray& other);
    void stealSomeCellsFrom(MarkStackArray& other, size_t idleThreadCount);

    size_t size();
    bool isEmpty();

private:
    template <size_t size> struct CapacityFromSize {
        static const size_t value = (size - sizeof(MarkStackSegment)) / sizeof(const JSCell*);
    };

    JS_EXPORT_PRIVATE void expand();
    
    size_t postIncTop();
    size_t preDecTop();
    void setTopForFullSegment();
    void setTopForEmptySegment();
    size_t top();
    
    void validatePrevious();

    DoublyLinkedList<MarkStackSegment> m_segments;
    BlockAllocator& m_blockAllocator;

    JS_EXPORT_PRIVATE static const size_t s_segmentCapacity = CapacityFromSize<MarkStackSegment::blockSize>::value;
    size_t m_top;
    size_t m_numberOfSegments;
   
};

} // namespace JSC

#endif
