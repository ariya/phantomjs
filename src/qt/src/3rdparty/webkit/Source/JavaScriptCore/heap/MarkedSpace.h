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
#include "MarkedBlock.h"
#include "PageAllocationAligned.h"
#include <wtf/Bitmap.h>
#include <wtf/DoublyLinkedList.h>
#include <wtf/FixedArray.h>
#include <wtf/HashSet.h>
#include <wtf/Noncopyable.h>
#include <wtf/Vector.h>

#define ASSERT_CLASS_FITS_IN_CELL(class) COMPILE_ASSERT(sizeof(class) < MarkedSpace::maxCellSize, class_fits_in_cell)

namespace JSC {

    class Heap;
    class JSCell;
    class JSGlobalData;
    class LiveObjectIterator;
    class MarkStack;
    class WeakGCHandle;
    typedef MarkStack SlotVisitor;

    class MarkedSpace {
        WTF_MAKE_NONCOPYABLE(MarkedSpace);
    public:
        // Currently public for use in assertions.
        static const size_t maxCellSize = 1024;

        static Heap* heap(JSCell*);

        static bool isMarked(const JSCell*);
        static bool testAndSetMarked(const JSCell*);
        static void setMarked(const JSCell*);

        MarkedSpace(JSGlobalData*);
        void destroy();

        JSGlobalData* globalData();

        size_t highWaterMark();
        void setHighWaterMark(size_t);

        void* allocate(size_t);

        void clearMarks();
        void markRoots();
        void reset();
        void sweep();
        void shrink();

        size_t size() const;
        size_t capacity() const;
        size_t objectCount() const;

        bool contains(const void*);

        template<typename Functor> void forEach(Functor&);

    private:
        // [ 8, 16... 128 )
        static const size_t preciseStep = MarkedBlock::atomSize;
        static const size_t preciseCutoff = 128;
        static const size_t preciseCount = preciseCutoff / preciseStep - 1;

        // [ 128, 256... 1024 )
        static const size_t impreciseStep = preciseCutoff;
        static const size_t impreciseCutoff = maxCellSize;
        static const size_t impreciseCount = impreciseCutoff / impreciseStep - 1;

        typedef HashSet<MarkedBlock*>::iterator BlockIterator;

        struct SizeClass {
            SizeClass();
            void reset();

            MarkedBlock* nextBlock;
            DoublyLinkedList<MarkedBlock> blockList;
            size_t cellSize;
        };

        MarkedBlock* allocateBlock(SizeClass&);
        void freeBlocks(DoublyLinkedList<MarkedBlock>&);

        SizeClass& sizeClassFor(size_t);
        void* allocateFromSizeClass(SizeClass&);

        void clearMarks(MarkedBlock*);

        SizeClass m_preciseSizeClasses[preciseCount];
        SizeClass m_impreciseSizeClasses[impreciseCount];
        HashSet<MarkedBlock*> m_blocks;
        size_t m_waterMark;
        size_t m_highWaterMark;
        JSGlobalData* m_globalData;
    };

    inline Heap* MarkedSpace::heap(JSCell* cell)
    {
        return MarkedBlock::blockFor(cell)->heap();
    }

    inline bool MarkedSpace::isMarked(const JSCell* cell)
    {
        return MarkedBlock::blockFor(cell)->isMarked(cell);
    }

    inline bool MarkedSpace::testAndSetMarked(const JSCell* cell)
    {
        return MarkedBlock::blockFor(cell)->testAndSetMarked(cell);
    }

    inline void MarkedSpace::setMarked(const JSCell* cell)
    {
        MarkedBlock::blockFor(cell)->setMarked(cell);
    }

    inline bool MarkedSpace::contains(const void* x)
    {
        if (!MarkedBlock::isAtomAligned(x))
            return false;

        MarkedBlock* block = MarkedBlock::blockFor(x);
        if (!block || !m_blocks.contains(block))
            return false;

        return block->contains(x);
    }

    template <typename Functor> inline void MarkedSpace::forEach(Functor& functor)
    {
        BlockIterator end = m_blocks.end();
        for (BlockIterator it = m_blocks.begin(); it != end; ++it)
            (*it)->forEach(functor);
    }

    inline JSGlobalData* MarkedSpace::globalData()
    {
        return m_globalData;
    }

    inline size_t MarkedSpace::highWaterMark()
    {
        return m_highWaterMark;
    }

    inline void MarkedSpace::setHighWaterMark(size_t highWaterMark)
    {
        m_highWaterMark = highWaterMark;
    }

    inline MarkedSpace::SizeClass::SizeClass()
        : nextBlock(0)
        , cellSize(0)
    {
    }

    inline void MarkedSpace::SizeClass::reset()
    {
        nextBlock = blockList.head();
    }

} // namespace JSC

#endif // MarkedSpace_h
