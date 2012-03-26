/*
 * Copyright (C) 2003, 2004, 2005, 2006, 2007 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "InlineBox.h"

#include "HitTestResult.h"
#include "InlineFlowBox.h"
#include "PaintInfo.h"
#include "RenderArena.h"
#include "RenderBlock.h"
#include "RootInlineBox.h"

using namespace std;

namespace WebCore {
    
#ifndef NDEBUG
static bool inInlineBoxDetach;
#endif

#ifndef NDEBUG

InlineBox::~InlineBox()
{
    if (!m_hasBadParent && m_parent)
        m_parent->setHasBadChildList();
}

#endif

void InlineBox::remove()
{ 
    if (parent())
        parent()->removeChild(this);
}

void InlineBox::destroy(RenderArena* renderArena)
{
#ifndef NDEBUG
    inInlineBoxDetach = true;
#endif
    delete this;
#ifndef NDEBUG
    inInlineBoxDetach = false;
#endif

    // Recover the size left there for us by operator delete and free the memory.
    renderArena->free(*(size_t *)this, this);
}

void* InlineBox::operator new(size_t sz, RenderArena* renderArena) throw()
{
    return renderArena->allocate(sz);
}

void InlineBox::operator delete(void* ptr, size_t sz)
{
    ASSERT(inInlineBoxDetach);

    // Stash size where destroy can find it.
    *(size_t *)ptr = sz;
}

#ifndef NDEBUG
const char* InlineBox::boxName() const
{
    return "InlineBox";
}

void InlineBox::showTreeForThis() const
{
    if (m_renderer)
        m_renderer->showTreeForThis();
}

void InlineBox::showLineTreeForThis() const
{
    if (m_renderer)
        m_renderer->containingBlock()->showLineTreeAndMark(this, "*");
}

void InlineBox::showLineTreeAndMark(const InlineBox* markedBox1, const char* markedLabel1, const InlineBox* markedBox2, const char* markedLabel2, const RenderObject* obj, int depth) const
{
    int printedCharacters = 0;
    if (this == markedBox1)
        printedCharacters += fprintf(stderr, "%s", markedLabel1);
    if (this == markedBox2)
        printedCharacters += fprintf(stderr, "%s", markedLabel2);
    if (renderer() == obj)
        printedCharacters += fprintf(stderr, "*");
    for (; printedCharacters < depth * 2; printedCharacters++)
        fputc(' ', stderr);

    showBox(printedCharacters);
}

void InlineBox::showBox(int printedCharacters) const
{
    printedCharacters += fprintf(stderr, "%s\t%p", boxName(), this);
    for (; printedCharacters < showTreeCharacterOffset; printedCharacters++)
        fputc(' ', stderr);
    fprintf(stderr, "\t%s %p\n", renderer() ? renderer()->renderName() : "No Renderer", renderer());
}
#endif

int InlineBox::logicalHeight() const
{
#if ENABLE(SVG)
    if (hasVirtualLogicalHeight())
        return virtualLogicalHeight();
#endif
    
    if (renderer()->isText())
        return m_isText ? renderer()->style(m_firstLine)->fontMetrics().height() : 0;
    if (renderer()->isBox() && parent())
        return isHorizontal() ? toRenderBox(m_renderer)->height() : toRenderBox(m_renderer)->width();

    ASSERT(isInlineFlowBox());
    RenderBoxModelObject* flowObject = boxModelObject();
    const FontMetrics& fontMetrics = renderer()->style(m_firstLine)->fontMetrics();
    int result = fontMetrics.height();
    if (parent())
        result += flowObject->borderAndPaddingLogicalHeight();
    return result;
}

int InlineBox::caretMinOffset() const 
{ 
    return m_renderer->caretMinOffset(); 
}

int InlineBox::caretMaxOffset() const 
{ 
    return m_renderer->caretMaxOffset(); 
}

unsigned InlineBox::caretMaxRenderedOffset() const 
{ 
    return 1; 
}

void InlineBox::dirtyLineBoxes()
{
    markDirty();
    for (InlineFlowBox* curr = parent(); curr && !curr->isDirty(); curr = curr->parent())
        curr->markDirty();
}

void InlineBox::deleteLine(RenderArena* arena)
{
    if (!m_extracted && m_renderer->isBox())
        toRenderBox(m_renderer)->setInlineBoxWrapper(0);
    destroy(arena);
}

void InlineBox::extractLine()
{
    m_extracted = true;
    if (m_renderer->isBox())
        toRenderBox(m_renderer)->setInlineBoxWrapper(0);
}

void InlineBox::attachLine()
{
    m_extracted = false;
    if (m_renderer->isBox())
        toRenderBox(m_renderer)->setInlineBoxWrapper(this);
}

void InlineBox::adjustPosition(float dx, float dy)
{
    m_x += dx;
    m_y += dy;

    if (m_renderer->isReplaced()) 
        toRenderBox(m_renderer)->move(dx, dy); 
}

void InlineBox::paint(PaintInfo& paintInfo, int tx, int ty, int /* lineTop */, int /*lineBottom*/)
{
    if (!paintInfo.shouldPaintWithinRoot(renderer()) || (paintInfo.phase != PaintPhaseForeground && paintInfo.phase != PaintPhaseSelection))
        return;

    IntPoint childPoint = IntPoint(tx, ty);
    if (parent()->renderer()->style()->isFlippedBlocksWritingMode()) // Faster than calling containingBlock().
        childPoint = renderer()->containingBlock()->flipForWritingMode(toRenderBox(renderer()), childPoint, RenderBox::ParentToChildFlippingAdjustment);
    
    // Paint all phases of replaced elements atomically, as though the replaced element established its
    // own stacking context.  (See Appendix E.2, section 6.4 on inline block/table elements in the CSS2.1
    // specification.)
    bool preservePhase = paintInfo.phase == PaintPhaseSelection || paintInfo.phase == PaintPhaseTextClip;
    PaintInfo info(paintInfo);
    info.phase = preservePhase ? paintInfo.phase : PaintPhaseBlockBackground;
    renderer()->paint(info, childPoint.x(), childPoint.y());
    if (!preservePhase) {
        info.phase = PaintPhaseChildBlockBackgrounds;
        renderer()->paint(info, childPoint.x(), childPoint.y());
        info.phase = PaintPhaseFloat;
        renderer()->paint(info, childPoint.x(), childPoint.y());
        info.phase = PaintPhaseForeground;
        renderer()->paint(info, childPoint.x(), childPoint.y());
        info.phase = PaintPhaseOutline;
        renderer()->paint(info, childPoint.x(), childPoint.y());
    }
}

bool InlineBox::nodeAtPoint(const HitTestRequest& request, HitTestResult& result, int x, int y, int tx, int ty, int /* lineTop */, int /*lineBottom*/)
{
    // Hit test all phases of replaced elements atomically, as though the replaced element established its
    // own stacking context.  (See Appendix E.2, section 6.4 on inline block/table elements in the CSS2.1
    // specification.)
    return renderer()->hitTest(request, result, IntPoint(x, y), tx, ty);
}

const RootInlineBox* InlineBox::root() const
{ 
    if (m_parent)
        return m_parent->root(); 
    ASSERT(isRootInlineBox());
    return static_cast<const RootInlineBox*>(this);
}

RootInlineBox* InlineBox::root()
{ 
    if (m_parent)
        return m_parent->root(); 
    ASSERT(isRootInlineBox());
    return static_cast<RootInlineBox*>(this);
}

bool InlineBox::nextOnLineExists() const
{
    if (!m_determinedIfNextOnLineExists) {
        m_determinedIfNextOnLineExists = true;

        if (!parent())
            m_nextOnLineExists = false;
        else if (nextOnLine())
            m_nextOnLineExists = true;
        else
            m_nextOnLineExists = parent()->nextOnLineExists();
    }
    return m_nextOnLineExists;
}

bool InlineBox::prevOnLineExists() const
{
    if (!m_determinedIfPrevOnLineExists) {
        m_determinedIfPrevOnLineExists = true;
        
        if (!parent())
            m_prevOnLineExists = false;
        else if (prevOnLine())
            m_prevOnLineExists = true;
        else
            m_prevOnLineExists = parent()->prevOnLineExists();
    }
    return m_prevOnLineExists;
}

InlineBox* InlineBox::nextLeafChild() const
{
    InlineBox* leaf = 0;
    for (InlineBox* box = nextOnLine(); box && !leaf; box = box->nextOnLine())
        leaf = box->isLeaf() ? box : static_cast<InlineFlowBox*>(box)->firstLeafChild();
    if (!leaf && parent())
        leaf = parent()->nextLeafChild();
    return leaf;
}
    
InlineBox* InlineBox::prevLeafChild() const
{
    InlineBox* leaf = 0;
    for (InlineBox* box = prevOnLine(); box && !leaf; box = box->prevOnLine())
        leaf = box->isLeaf() ? box : static_cast<InlineFlowBox*>(box)->lastLeafChild();
    if (!leaf && parent())
        leaf = parent()->prevLeafChild();
    return leaf;
}
    
RenderObject::SelectionState InlineBox::selectionState()
{
    return renderer()->selectionState();
}

bool InlineBox::canAccommodateEllipsis(bool ltr, int blockEdge, int ellipsisWidth)
{
    // Non-replaced elements can always accommodate an ellipsis.
    if (!m_renderer || !m_renderer->isReplaced())
        return true;
    
    IntRect boxRect(m_x, 0, m_logicalWidth, 10);
    IntRect ellipsisRect(ltr ? blockEdge - ellipsisWidth : blockEdge, 0, ellipsisWidth, 10);
    return !(boxRect.intersects(ellipsisRect));
}

float InlineBox::placeEllipsisBox(bool, float, float, float, bool&)
{
    // Use -1 to mean "we didn't set the position."
    return -1;
}

void InlineBox::clearKnownToHaveNoOverflow()
{ 
    m_knownToHaveNoOverflow = false;
    if (parent() && parent()->knownToHaveNoOverflow())
        parent()->clearKnownToHaveNoOverflow();
}

FloatPoint InlineBox::locationIncludingFlipping()
{
    if (!renderer()->style()->isFlippedBlocksWritingMode())
        return FloatPoint(x(), y());
    RenderBlock* block = root()->block();
    if (block->style()->isHorizontalWritingMode())
        return FloatPoint(x(), block->height() - height() - y());
    else
        return FloatPoint(block->width() - width() - x(), y());
}

void InlineBox::flipForWritingMode(FloatRect& rect)
{
    if (!renderer()->style()->isFlippedBlocksWritingMode())
        return;
    root()->block()->flipForWritingMode(rect);
}

FloatPoint InlineBox::flipForWritingMode(const FloatPoint& point)
{
    if (!renderer()->style()->isFlippedBlocksWritingMode())
        return point;
    return root()->block()->flipForWritingMode(point);
}

void InlineBox::flipForWritingMode(IntRect& rect)
{
    if (!renderer()->style()->isFlippedBlocksWritingMode())
        return;
    root()->block()->flipForWritingMode(rect);
}

IntPoint InlineBox::flipForWritingMode(const IntPoint& point)
{
    if (!renderer()->style()->isFlippedBlocksWritingMode())
        return point;
    return root()->block()->flipForWritingMode(point);
}

} // namespace WebCore

#ifndef NDEBUG

void showTree(const WebCore::InlineBox* b)
{
    if (b)
        b->showTreeForThis();
}

void showLineTree(const WebCore::InlineBox* b)
{
    if (b)
        b->showLineTreeForThis();
}

#endif
