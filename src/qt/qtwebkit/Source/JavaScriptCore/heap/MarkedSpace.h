/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2001 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2011 Apple Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef MarkedSpace_h
#define MarkedSpace_h

#include "MachineStackMarker.h"
#include "MarkedAllocator.h"
#include "MarkedBlock.h"
#include "MarkedBlockSet.h"
#include <wtf/PageAllocationAligned.h>
#include <wtf/Bitmap.h>
#include <wtf/DoublyLinkedList.h>
#include <wtf/FixedArray.h>
#include <wtf/HashSet.h>
#include <wtf/Noncopyable.h>
#include <wtf/Vector.h>

namespace JSC {

class Heap;
class JSCell;
class LiveObjectIterator;
class LLIntOffsetsExtractor;
class WeakGCHandle;
class SlotVisitor;

struct ClearMarks : MarkedBlock::VoidFunctor {
    void operator()(MarkedBlock* block) { block->clearMarks(); }
};

struct Sweep : MarkedBlock::VoidFunctor {
    void operator()(MarkedBlock* block) { block->sweep(); }
};

struct MarkCount : MarkedBlock::CountFunctor {
    void operator()(MarkedBlock* block) { count(block->markCount()); }
};

struct Size : MarkedBlock::CountFunctor {
    void operator()(MarkedBlock* block) { count(block->markCount() * block->cellSize()); }
};

struct Capacity : MarkedBlock::CountFunctor {
    void operator()(MarkedBlock* block) { count(block->capacity()); }
};

class MarkedSpace {
    WTF_MAKE_NONCOPYABLE(MarkedSpace);
public:
    MarkedSpace(Heap*);
    ~MarkedSpace();
    void lastChanceToFinalize();

    MarkedAllocator& firstAllocator();
    MarkedAllocator& allocatorFor(size_t);
    MarkedAllocator& immortalStructureDestructorAllocatorFor(size_t);
    MarkedAllocator& normalDestructorAllocatorFor(size_t);
    void* allocateWithNormalDestructor(size_t);
    void* allocateWithImmortalStructureDestructor(size_t);
    void* allocateWithoutDestructor(size_t);
 
    void resetAllocators();

    void visitWeakSets(HeapRootVisitor&);
    void reapWeakSets();

    MarkedBlockSet& blocks() { return m_blocks; }
    
    void canonicalizeCellLivenessData();

    typedef HashSet<MarkedBlock*>::iterator BlockIterator;
    
    template<typename Functor> typename Functor::ReturnType forEachLiveCell(Functor&);
    template<typename Functor> typename Functor::ReturnType forEachLiveCell();
    template<typename Functor> typename Functor::ReturnType forEachDeadCell(Functor&);
    template<typename Functor> typename Functor::ReturnType forEachDeadCell();
    template<typename Functor> typename Functor::ReturnType forEachBlock(Functor&);
    template<typename Functor> typename Functor::ReturnType forEachBlock();
    
    void shrink();
    void freeBlock(MarkedBlock*);
    void freeOrShrinkBlock(MarkedBlock*);

    void didAddBlock(MarkedBlock*);
    void didConsumeFreeList(MarkedBlock*);

    void clearMarks();
    void sweep();
    size_t objectCount();
    size_t size();
    size_t capacity();

    bool isPagedOut(double deadline);

private:
    friend class LLIntOffsetsExtractor;

    // [ 32... 128 ]
    static const size_t preciseStep = MarkedBlock::atomSize;
    static const size_t preciseCutoff = 128;
    static const size_t preciseCount = preciseCutoff / preciseStep;

    // [ 1024... blockSize ]
    static const size_t impreciseStep = 2 * preciseCutoff;
    static const size_t impreciseCutoff = MarkedBlock::blockSize / 2;
    static const size_t impreciseCount = impreciseCutoff / impreciseStep;

    struct Subspace {
        FixedArray<MarkedAllocator, preciseCount> preciseAllocators;
        FixedArray<MarkedAllocator, impreciseCount> impreciseAllocators;
        MarkedAllocator largeAllocator;
    };

    Subspace m_normalDestructorSpace;
    Subspace m_immortalStructureDestructorSpace;
    Subspace m_normalSpace;

    Heap* m_heap;
    MarkedBlockSet m_blocks;
};

template<typename Functor> inline typename Functor::ReturnType MarkedSpace::forEachLiveCell(Functor& functor)
{
    canonicalizeCellLivenessData();

    BlockIterator end = m_blocks.set().end();
    for (BlockIterator it = m_blocks.set().begin(); it != end; ++it)
        (*it)->forEachLiveCell(functor);
    return functor.returnValue();
}

template<typename Functor> inline typename Functor::ReturnType MarkedSpace::forEachLiveCell()
{
    Functor functor;
    return forEachLiveCell(functor);
}

template<typename Functor> inline typename Functor::ReturnType MarkedSpace::forEachDeadCell(Functor& functor)
{
    canonicalizeCellLivenessData();

    BlockIterator end = m_blocks.set().end();
    for (BlockIterator it = m_blocks.set().begin(); it != end; ++it)
        (*it)->forEachDeadCell(functor);
    return functor.returnValue();
}

template<typename Functor> inline typename Functor::ReturnType MarkedSpace::forEachDeadCell()
{
    Functor functor;
    return forEachDeadCell(functor);
}

inline MarkedAllocator& MarkedSpace::allocatorFor(size_t bytes)
{
    ASSERT(bytes);
    if (bytes <= preciseCutoff)
        return m_normalSpace.preciseAllocators[(bytes - 1) / preciseStep];
    if (bytes <= impreciseCutoff)
        return m_normalSpace.impreciseAllocators[(bytes - 1) / impreciseStep];
    return m_normalSpace.largeAllocator;
}

inline MarkedAllocator& MarkedSpace::immortalStructureDestructorAllocatorFor(size_t bytes)
{
    ASSERT(bytes);
    if (bytes <= preciseCutoff)
        return m_immortalStructureDestructorSpace.preciseAllocators[(bytes - 1) / preciseStep];
    if (bytes <= impreciseCutoff)
        return m_immortalStructureDestructorSpace.impreciseAllocators[(bytes - 1) / impreciseStep];
    return m_immortalStructureDestructorSpace.largeAllocator;
}

inline MarkedAllocator& MarkedSpace::normalDestructorAllocatorFor(size_t bytes)
{
    ASSERT(bytes);
    if (bytes <= preciseCutoff)
        return m_normalDestructorSpace.preciseAllocators[(bytes - 1) / preciseStep];
    if (bytes <= impreciseCutoff)
        return m_normalDestructorSpace.impreciseAllocators[(bytes - 1) / impreciseStep];
    return m_normalDestructorSpace.largeAllocator;
}

inline void* MarkedSpace::allocateWithoutDestructor(size_t bytes)
{
    return allocatorFor(bytes).allocate(bytes);
}

inline void* MarkedSpace::allocateWithImmortalStructureDestructor(size_t bytes)
{
    return immortalStructureDestructorAllocatorFor(bytes).allocate(bytes);
}

inline void* MarkedSpace::allocateWithNormalDestructor(size_t bytes)
{
    return normalDestructorAllocatorFor(bytes).allocate(bytes);
}

template <typename Functor> inline typename Functor::ReturnType MarkedSpace::forEachBlock(Functor& functor)
{
    for (size_t i = 0; i < preciseCount; ++i) {
        m_normalSpace.preciseAllocators[i].forEachBlock(functor);
        m_normalDestructorSpace.preciseAllocators[i].forEachBlock(functor);
        m_immortalStructureDestructorSpace.preciseAllocators[i].forEachBlock(functor);
    }

    for (size_t i = 0; i < impreciseCount; ++i) {
        m_normalSpace.impreciseAllocators[i].forEachBlock(functor);
        m_normalDestructorSpace.impreciseAllocators[i].forEachBlock(functor);
        m_immortalStructureDestructorSpace.impreciseAllocators[i].forEachBlock(functor);
    }

    m_normalSpace.largeAllocator.forEachBlock(functor);
    m_normalDestructorSpace.largeAllocator.forEachBlock(functor);
    m_immortalStructureDestructorSpace.largeAllocator.forEachBlock(functor);

    return functor.returnValue();
}

template <typename Functor> inline typename Functor::ReturnType MarkedSpace::forEachBlock()
{
    Functor functor;
    return forEachBlock(functor);
}

inline void MarkedSpace::didAddBlock(MarkedBlock* block)
{
    m_blocks.add(block);
}

inline void MarkedSpace::clearMarks()
{
    forEachBlock<ClearMarks>();
}

inline size_t MarkedSpace::objectCount()
{
    return forEachBlock<MarkCount>();
}

inline size_t MarkedSpace::size()
{
    return forEachBlock<Size>();
}

inline size_t MarkedSpace::capacity()
{
    return forEachBlock<Capacity>();
}

} // namespace JSC

#endif // MarkedSpace_h
