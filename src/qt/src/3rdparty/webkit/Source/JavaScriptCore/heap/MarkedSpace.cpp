/*
 *  Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
 *  Copyright (C) 2007 Eric Seidel <eric@webkit.org>
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

#include "config.h"
#include "MarkedSpace.h"

#include "JSCell.h"
#include "JSGlobalData.h"
#include "JSLock.h"
#include "JSObject.h"
#include "ScopeChain.h"

namespace JSC {

class Structure;

MarkedSpace::MarkedSpace(JSGlobalData* globalData)
    : m_waterMark(0)
    , m_highWaterMark(0)
    , m_globalData(globalData)
{
    for (size_t cellSize = preciseStep; cellSize < preciseCutoff; cellSize += preciseStep)
        sizeClassFor(cellSize).cellSize = cellSize;

    for (size_t cellSize = impreciseStep; cellSize < impreciseCutoff; cellSize += impreciseStep)
        sizeClassFor(cellSize).cellSize = cellSize;
}

void MarkedSpace::destroy()
{
    clearMarks();
    shrink();
    ASSERT(!size());
}

MarkedBlock* MarkedSpace::allocateBlock(SizeClass& sizeClass)
{
    MarkedBlock* block = MarkedBlock::create(globalData(), sizeClass.cellSize);
    sizeClass.blockList.append(block);
    sizeClass.nextBlock = block;
    m_blocks.add(block);

    return block;
}

void MarkedSpace::freeBlocks(DoublyLinkedList<MarkedBlock>& blocks)
{
    MarkedBlock* next;
    for (MarkedBlock* block = blocks.head(); block; block = next) {
        next = block->next();

        blocks.remove(block);
        m_blocks.remove(block);
        MarkedBlock::destroy(block);
    }
}

void* MarkedSpace::allocateFromSizeClass(SizeClass& sizeClass)
{
    for (MarkedBlock*& block = sizeClass.nextBlock ; block; block = block->next()) {
        if (void* result = block->allocate())
            return result;

        m_waterMark += block->capacity();
    }

    if (m_waterMark < m_highWaterMark)
        return allocateBlock(sizeClass)->allocate();

    return 0;
}

void MarkedSpace::shrink()
{
    // We record a temporary list of empties to avoid modifying m_blocks while iterating it.
    DoublyLinkedList<MarkedBlock> empties;

    BlockIterator end = m_blocks.end();
    for (BlockIterator it = m_blocks.begin(); it != end; ++it) {
        MarkedBlock* block = *it;
        if (block->isEmpty()) {
            SizeClass& sizeClass = sizeClassFor(block->cellSize());
            sizeClass.blockList.remove(block);
            sizeClass.nextBlock = sizeClass.blockList.head();
            empties.append(block);
        }
    }
    
    freeBlocks(empties);
    ASSERT(empties.isEmpty());
}

void MarkedSpace::clearMarks()
{
    BlockIterator end = m_blocks.end();
    for (BlockIterator it = m_blocks.begin(); it != end; ++it)
        (*it)->clearMarks();
}

void MarkedSpace::sweep()
{
    BlockIterator end = m_blocks.end();
    for (BlockIterator it = m_blocks.begin(); it != end; ++it)
        (*it)->sweep();
}

size_t MarkedSpace::objectCount() const
{
    size_t result = 0;
    BlockIterator end = m_blocks.end();
    for (BlockIterator it = m_blocks.begin(); it != end; ++it)
        result += (*it)->markCount();
    return result;
}

size_t MarkedSpace::size() const
{
    size_t result = 0;
    BlockIterator end = m_blocks.end();
    for (BlockIterator it = m_blocks.begin(); it != end; ++it)
        result += (*it)->size();
    return result;
}

size_t MarkedSpace::capacity() const
{
    size_t result = 0;
    BlockIterator end = m_blocks.end();
    for (BlockIterator it = m_blocks.begin(); it != end; ++it)
        result += (*it)->capacity();
    return result;
}

void MarkedSpace::reset()
{
    m_waterMark = 0;

    for (size_t cellSize = preciseStep; cellSize < preciseCutoff; cellSize += preciseStep)
        sizeClassFor(cellSize).reset();

    for (size_t cellSize = impreciseStep; cellSize < impreciseCutoff; cellSize += impreciseStep)
        sizeClassFor(cellSize).reset();

    BlockIterator end = m_blocks.end();
    for (BlockIterator it = m_blocks.begin(); it != end; ++it)
        (*it)->reset();
}

} // namespace JSC
