/*
 * Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef AreaAllocator_h
#define AreaAllocator_h

#include "IntPoint.h"
#include "IntRect.h"
#include "IntSize.h"

#if USE(COORDINATED_GRAPHICS)

namespace WebCore {

inline int nextPowerOfTwo(int number)
{
    // This is a fast trick to get nextPowerOfTwo for an integer.
    --number;
    number |= number >> 1;
    number |= number >> 2;
    number |= number >> 4;
    number |= number >> 8;
    number |= number >> 16;
    number++;
    return number;
}

inline IntSize nextPowerOfTwo(const IntSize& size)
{
    return IntSize(nextPowerOfTwo(size.width()), nextPowerOfTwo(size.height()));
}

class AreaAllocator {
public:
    explicit AreaAllocator(const IntSize&);
    virtual ~AreaAllocator();

    IntSize size() const { return m_size; }

    IntSize minimumAllocation() const { return m_minAlloc; }
    void setMinimumAllocation(const IntSize& size) { m_minAlloc = size; }

    IntSize margin() const { return m_margin; }
    void setMargin(const IntSize &margin) { m_margin = margin; }

    virtual void expand(const IntSize&);
    void expandBy(const IntSize&);

    virtual IntRect allocate(const IntSize&) = 0;
    virtual void release(const IntRect&);

    virtual int overhead() const;

protected:
    IntSize m_size;
    IntSize m_minAlloc;
    IntSize m_margin;

    IntSize roundAllocation(const IntSize&) const;
};

class GeneralAreaAllocator : public AreaAllocator {
public:
    explicit GeneralAreaAllocator(const IntSize&);
    virtual ~GeneralAreaAllocator();

    void expand(const IntSize&);
    IntRect allocate(const IntSize&);
    void release(const IntRect&);
    int overhead() const;

private:
    enum Split { SplitOnX, SplitOnY };

    struct Node {
        IntRect rect;
        IntSize largestFree;
        Node* parent;
        Node* left;
        Node* right;
    };

    Node* m_root;
    int m_nodeCount;

    static void freeNode(Node*);
    IntPoint allocateFromNode(const IntSize&, Node*);
    Node* splitNode(Node*, Split);
    static void updateLargestFree(Node*);
};

} // namespace WebCore

#endif // USE(COORDINATED_GRAPHICS)

#endif // AreaAllocator_h
