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

#ifndef MarkedBlock_h
#define MarkedBlock_h

#include <wtf/Bitmap.h>
#include <wtf/PageAllocationAligned.h>
#include <wtf/StdLibExtras.h>

namespace JSC {

    class Heap;
    class JSCell;
    class JSGlobalData;

    typedef uintptr_t Bits;

    static const size_t KB = 1024;

    class MarkedBlock {
    public:
        static const size_t atomSize = sizeof(double); // Ensures natural alignment for all built-in types.

        static MarkedBlock* create(JSGlobalData*, size_t cellSize);
        static void destroy(MarkedBlock*);

        static bool isAtomAligned(const void*);
        static MarkedBlock* blockFor(const void*);
        static size_t firstAtom();
        
        Heap* heap() const;

        void setPrev(MarkedBlock*);
        void setNext(MarkedBlock*);
        MarkedBlock* prev() const;
        MarkedBlock* next() const;
        
        void* allocate();
        void reset();
        void sweep();
        
        bool isEmpty();

        void clearMarks();
        size_t markCount();

        size_t cellSize();

        size_t size();
        size_t capacity();

        bool contains(const void*);
        size_t atomNumber(const void*);
        bool isMarked(const void*);
        bool testAndSetMarked(const void*);
        void setMarked(const void*);
        
        template <typename Functor> void forEach(Functor&);

    private:
        static const size_t blockSize = 16 * KB;
        static const size_t blockMask = ~(blockSize - 1); // blockSize must be a power of two.

        static const size_t atomMask = ~(atomSize - 1); // atomSize must be a power of two.
        
        static const size_t atomsPerBlock = blockSize / atomSize;

        typedef char Atom[atomSize];

        MarkedBlock(const PageAllocationAligned&, JSGlobalData*, size_t cellSize);
        Atom* atoms();

        size_t m_nextAtom;
        size_t m_endAtom; // This is a fuzzy end. Always test for < m_endAtom.
        size_t m_atomsPerCell;
        WTF::Bitmap<blockSize / atomSize> m_marks;
        PageAllocationAligned m_allocation;
        Heap* m_heap;
        MarkedBlock* m_prev;
        MarkedBlock* m_next;
    };

    inline size_t MarkedBlock::firstAtom()
    {
        return WTF::roundUpToMultipleOf<atomSize>(sizeof(MarkedBlock)) / atomSize;
    }

    inline MarkedBlock::Atom* MarkedBlock::atoms()
    {
        return reinterpret_cast<Atom*>(this);
    }

    inline bool MarkedBlock::isAtomAligned(const void* p)
    {
        return !((intptr_t)(p) & ~atomMask);
    }

    inline MarkedBlock* MarkedBlock::blockFor(const void* p)
    {
        return reinterpret_cast<MarkedBlock*>(reinterpret_cast<uintptr_t>(p) & blockMask);
    }

    inline Heap* MarkedBlock::heap() const
    {
        return m_heap;
    }

    inline void MarkedBlock::setPrev(MarkedBlock* prev)
    {
        m_prev = prev;
    }

    inline void MarkedBlock::setNext(MarkedBlock* next)
    {
        m_next = next;
    }

    inline MarkedBlock* MarkedBlock::prev() const
    {
        return m_prev;
    }

    inline MarkedBlock* MarkedBlock::next() const
    {
        return m_next;
    }

    inline void MarkedBlock::reset()
    {
        m_nextAtom = firstAtom();
    }

    inline bool MarkedBlock::isEmpty()
    {
        return m_marks.isEmpty();
    }

    inline void MarkedBlock::clearMarks()
    {
        m_marks.clearAll();
    }
    
    inline size_t MarkedBlock::markCount()
    {
        return m_marks.count();
    }

    inline size_t MarkedBlock::cellSize()
    {
        return m_atomsPerCell * atomSize;
    }

    inline size_t MarkedBlock::size()
    {
        return markCount() * cellSize();
    }

    inline size_t MarkedBlock::capacity()
    {
        return m_allocation.size();
    }

    inline bool MarkedBlock::contains(const void* p)
    {
        ASSERT(p && isAtomAligned(p) && atomNumber(p) < atomsPerBlock);

        // Even though we physically contain p, we only logically contain p if p
        // points to a live cell. (Claiming to contain a dead cell would trick the
        // conservative garbage collector into resurrecting the cell in a zombie state.)
        return isMarked(p);
    }

    inline size_t MarkedBlock::atomNumber(const void* p)
    {
        return (reinterpret_cast<uintptr_t>(p) - reinterpret_cast<uintptr_t>(this)) / atomSize;
    }

    inline bool MarkedBlock::isMarked(const void* p)
    {
        return m_marks.get(atomNumber(p));
    }

    inline bool MarkedBlock::testAndSetMarked(const void* p)
    {
        return m_marks.testAndSet(atomNumber(p));
    }

    inline void MarkedBlock::setMarked(const void* p)
    {
        m_marks.set(atomNumber(p));
    }

    template <typename Functor> inline void MarkedBlock::forEach(Functor& functor)
    {
        for (size_t i = firstAtom(); i < m_endAtom; i += m_atomsPerCell) {
            if (!m_marks.get(i))
                continue;
            functor(reinterpret_cast<JSCell*>(&atoms()[i]));
        }
    }

} // namespace JSC

#endif // MarkedSpace_h
