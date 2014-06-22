/*
 * Copyright (C) 2008 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "RenderLineBoxList.h"

#include "HitTestResult.h"
#include "InlineTextBox.h"
#include "PaintInfo.h"
#include "RenderArena.h"
#include "RenderInline.h"
#include "RenderView.h"
#include "RootInlineBox.h"

using namespace std;

namespace WebCore {

#ifndef NDEBUG
RenderLineBoxList::~RenderLineBoxList()
{
    ASSERT(!m_firstLineBox);
    ASSERT(!m_lastLineBox);
}
#endif

void RenderLineBoxList::appendLineBox(InlineFlowBox* box)
{
    checkConsistency();
    
    if (!m_firstLineBox)
        m_firstLineBox = m_lastLineBox = box;
    else {
        m_lastLineBox->setNextLineBox(box);
        box->setPreviousLineBox(m_lastLineBox);
        m_lastLineBox = box;
    }

    checkConsistency();
}

void RenderLineBoxList::deleteLineBoxTree(RenderArena* arena)
{
    InlineFlowBox* line = m_firstLineBox;
    InlineFlowBox* nextLine;
    while (line) {
        nextLine = line->nextLineBox();
        line->deleteLine(arena);
        line = nextLine;
    }
    m_firstLineBox = m_lastLineBox = 0;
}

void RenderLineBoxList::extractLineBox(InlineFlowBox* box)
{
    checkConsistency();
    
    m_lastLineBox = box->prevLineBox();
    if (box == m_firstLineBox)
        m_firstLineBox = 0;
    if (box->prevLineBox())
        box->prevLineBox()->setNextLineBox(0);
    box->setPreviousLineBox(0);
    for (InlineFlowBox* curr = box; curr; curr = curr->nextLineBox())
        curr->setExtracted();

    checkConsistency();
}

void RenderLineBoxList::attachLineBox(InlineFlowBox* box)
{
    checkConsistency();

    if (m_lastLineBox) {
        m_lastLineBox->setNextLineBox(box);
        box->setPreviousLineBox(m_lastLineBox);
    } else
        m_firstLineBox = box;
    InlineFlowBox* last = box;
    for (InlineFlowBox* curr = box; curr; curr = curr->nextLineBox()) {
        curr->setExtracted(false);
        last = curr;
    }
    m_lastLineBox = last;

    checkConsistency();
}

void RenderLineBoxList::removeLineBox(InlineFlowBox* box)
{
    checkConsistency();

    if (box == m_firstLineBox)
        m_firstLineBox = box->nextLineBox();
    if (box == m_lastLineBox)
        m_lastLineBox = box->prevLineBox();
    if (box->nextLineBox())
        box->nextLineBox()->setPreviousLineBox(box->prevLineBox());
    if (box->prevLineBox())
        box->prevLineBox()->setNextLineBox(box->nextLineBox());

    checkConsistency();
}

void RenderLineBoxList::deleteLineBoxes(RenderArena* arena)
{
    if (m_firstLineBox) {
        InlineFlowBox* next;
        for (InlineFlowBox* curr = m_firstLineBox; curr; curr = next) {
            next = curr->nextLineBox();
            curr->destroy(arena);
        }
        m_firstLineBox = 0;
        m_lastLineBox = 0;
    }
}

void RenderLineBoxList::dirtyLineBoxes()
{
    for (InlineFlowBox* curr = firstLineBox(); curr; curr = curr->nextLineBox())
        curr->dirtyLineBoxes();
}

bool RenderLineBoxList::rangeIntersectsRect(RenderBoxModelObject* renderer, LayoutUnit logicalTop, LayoutUnit logicalBottom, const LayoutRect& rect, const LayoutPoint& offset) const
{
    RenderBox* block;
    if (renderer->isBox())
        block = toRenderBox(renderer);
    else
        block = renderer->containingBlock();
    LayoutUnit physicalStart = block->flipForWritingMode(logicalTop);
    LayoutUnit physicalEnd = block->flipForWritingMode(logicalBottom);
    LayoutUnit physicalExtent = absoluteValue(physicalEnd - physicalStart);
    physicalStart = min(physicalStart, physicalEnd);
    
    if (renderer->style()->isHorizontalWritingMode()) {
        physicalStart += offset.y();
        if (physicalStart >= rect.maxY() || physicalStart + physicalExtent <= rect.y())
            return false;
    } else {
        physicalStart += offset.x();
        if (physicalStart >= rect.maxX() || physicalStart + physicalExtent <= rect.x())
            return false;
    }
    
    return true;
}

bool RenderLineBoxList::anyLineIntersectsRect(RenderBoxModelObject* renderer, const LayoutRect& rect, const LayoutPoint& offset, bool usePrintRect, LayoutUnit outlineSize) const
{
    // We can check the first box and last box and avoid painting/hit testing if we don't
    // intersect.  This is a quick short-circuit that we can take to avoid walking any lines.
    // FIXME: This check is flawed in the following extremely obscure way:
    // if some line in the middle has a huge overflow, it might actually extend below the last line.
    RootInlineBox* firstRootBox = firstLineBox()->root();
    RootInlineBox* lastRootBox = lastLineBox()->root();
    LayoutUnit firstLineTop = firstLineBox()->logicalTopVisualOverflow(firstRootBox->lineTop());
    if (usePrintRect && !firstLineBox()->parent())
        firstLineTop = min(firstLineTop, firstLineBox()->root()->lineTop());
    LayoutUnit lastLineBottom = lastLineBox()->logicalBottomVisualOverflow(lastRootBox->lineBottom());
    if (usePrintRect && !lastLineBox()->parent())
        lastLineBottom = max(lastLineBottom, lastLineBox()->root()->lineBottom());
    LayoutUnit logicalTop = firstLineTop - outlineSize;
    LayoutUnit logicalBottom = outlineSize + lastLineBottom;
    
    return rangeIntersectsRect(renderer, logicalTop, logicalBottom, rect, offset);
}

bool RenderLineBoxList::lineIntersectsDirtyRect(RenderBoxModelObject* renderer, InlineFlowBox* box, const PaintInfo& paintInfo, const LayoutPoint& offset) const
{
    RootInlineBox* root = box->root();
    LayoutUnit logicalTop = min<LayoutUnit>(box->logicalTopVisualOverflow(root->lineTop()), root->selectionTop()) - renderer->maximalOutlineSize(paintInfo.phase);
    LayoutUnit logicalBottom = box->logicalBottomVisualOverflow(root->lineBottom()) + renderer->maximalOutlineSize(paintInfo.phase);
    
    return rangeIntersectsRect(renderer, logicalTop, logicalBottom, paintInfo.rect, offset);
}

void RenderLineBoxList::paint(RenderBoxModelObject* renderer, PaintInfo& paintInfo, const LayoutPoint& paintOffset) const
{
    // Only paint during the foreground/selection phases.
    if (paintInfo.phase != PaintPhaseForeground && paintInfo.phase != PaintPhaseSelection && paintInfo.phase != PaintPhaseOutline 
        && paintInfo.phase != PaintPhaseSelfOutline && paintInfo.phase != PaintPhaseChildOutlines && paintInfo.phase != PaintPhaseTextClip
        && paintInfo.phase != PaintPhaseMask)
        return;

    ASSERT(renderer->isRenderBlock() || (renderer->isRenderInline() && renderer->hasLayer())); // The only way an inline could paint like this is if it has a layer.

    // If we have no lines then we have no work to do.
    if (!firstLineBox())
        return;

    // FIXME: Paint-time pagination is obsolete and is now only used by embedded WebViews inside AppKit
    // NSViews.  Do not add any more code for this.
    RenderView* v = renderer->view();
    bool usePrintRect = !v->printRect().isEmpty();
    LayoutUnit outlineSize = renderer->maximalOutlineSize(paintInfo.phase);
    if (!anyLineIntersectsRect(renderer, paintInfo.rect, paintOffset, usePrintRect, outlineSize))
        return;

    PaintInfo info(paintInfo);
    ListHashSet<RenderInline*> outlineObjects;
    info.outlineObjects = &outlineObjects;

    // See if our root lines intersect with the dirty rect.  If so, then we paint
    // them.  Note that boxes can easily overlap, so we can't make any assumptions
    // based off positions of our first line box or our last line box.
    for (InlineFlowBox* curr = firstLineBox(); curr; curr = curr->nextLineBox()) {
        if (usePrintRect) {
            // FIXME: This is the deprecated pagination model that is still needed
            // for embedded views inside AppKit.  AppKit is incapable of paginating vertical
            // text pages, so we don't have to deal with vertical lines at all here.
            RootInlineBox* root = curr->root();
            LayoutUnit topForPaginationCheck = curr->logicalTopVisualOverflow(root->lineTop());
            LayoutUnit bottomForPaginationCheck = curr->logicalLeftVisualOverflow();
            if (!curr->parent()) {
                // We're a root box.  Use lineTop and lineBottom as well here.
                topForPaginationCheck = min(topForPaginationCheck, root->lineTop());
                bottomForPaginationCheck = max(bottomForPaginationCheck, root->lineBottom());
            }
            if (bottomForPaginationCheck - topForPaginationCheck <= v->printRect().height()) {
                if (paintOffset.y() + bottomForPaginationCheck > v->printRect().maxY()) {
                    if (RootInlineBox* nextRootBox = curr->root()->nextRootBox())
                        bottomForPaginationCheck = min(bottomForPaginationCheck, min<LayoutUnit>(nextRootBox->logicalTopVisualOverflow(), nextRootBox->lineTop()));
                }
                if (paintOffset.y() + bottomForPaginationCheck > v->printRect().maxY()) {
                    if (paintOffset.y() + topForPaginationCheck < v->truncatedAt())
                        v->setBestTruncatedAt(paintOffset.y() + topForPaginationCheck, renderer);
                    // If we were able to truncate, don't paint.
                    if (paintOffset.y() + topForPaginationCheck >= v->truncatedAt())
                        break;
                }
            }
        }

        if (lineIntersectsDirtyRect(renderer, curr, info, paintOffset)) {
            RootInlineBox* root = curr->root();
            curr->paint(info, paintOffset, root->lineTop(), root->lineBottom());
        }
    }

    if (info.phase == PaintPhaseOutline || info.phase == PaintPhaseSelfOutline || info.phase == PaintPhaseChildOutlines) {
        ListHashSet<RenderInline*>::iterator end = info.outlineObjects->end();
        for (ListHashSet<RenderInline*>::iterator it = info.outlineObjects->begin(); it != end; ++it) {
            RenderInline* flow = *it;
            flow->paintOutline(info, paintOffset);
        }
        info.outlineObjects->clear();
    }
}

bool RenderLineBoxList::hitTest(RenderBoxModelObject* renderer, const HitTestRequest& request, HitTestResult& result, const HitTestLocation& locationInContainer, const LayoutPoint& accumulatedOffset, HitTestAction hitTestAction) const
{
    if (hitTestAction != HitTestForeground)
        return false;

    ASSERT(renderer->isRenderBlock() || (renderer->isRenderInline() && renderer->hasLayer())); // The only way an inline could hit test like this is if it has a layer.

    // If we have no lines then we have no work to do.
    if (!firstLineBox())
        return false;

    LayoutPoint point = locationInContainer.point();
    LayoutRect rect = firstLineBox()->isHorizontal() ?
        IntRect(point.x(), point.y() - locationInContainer.topPadding(), 1, locationInContainer.topPadding() + locationInContainer.bottomPadding() + 1) :
        IntRect(point.x() - locationInContainer.leftPadding(), point.y(), locationInContainer.rightPadding() + locationInContainer.leftPadding() + 1, 1);

    if (!anyLineIntersectsRect(renderer, rect, accumulatedOffset))
        return false;

    // See if our root lines contain the point.  If so, then we hit test
    // them further.  Note that boxes can easily overlap, so we can't make any assumptions
    // based off positions of our first line box or our last line box.
    for (InlineFlowBox* curr = lastLineBox(); curr; curr = curr->prevLineBox()) {
        RootInlineBox* root = curr->root();
        if (rangeIntersectsRect(renderer, curr->logicalTopVisualOverflow(root->lineTop()), curr->logicalBottomVisualOverflow(root->lineBottom()), rect, accumulatedOffset)) {
            bool inside = curr->nodeAtPoint(request, result, locationInContainer, accumulatedOffset, root->lineTop(), root->lineBottom());
            if (inside) {
                renderer->updateHitTestResult(result, locationInContainer.point() - toLayoutSize(accumulatedOffset));
                return true;
            }
        }
    }

    return false;
}

void RenderLineBoxList::dirtyLinesFromChangedChild(RenderObject* container, RenderObject* child)
{
    if (!container->parent() || (container->isRenderBlock() && (container->selfNeedsLayout() || !container->isBlockFlow())))
        return;

    RenderInline* inlineContainer = container->isRenderInline() ? toRenderInline(container) : 0;
    InlineBox* firstBox = inlineContainer ? inlineContainer->firstLineBoxIncludingCulling() : firstLineBox();

    // If we have no first line box, then just bail early.
    if (!firstBox) {
        // For an empty inline, go ahead and propagate the check up to our parent, unless the parent
        // is already dirty.
        if (container->isInline() && !container->ancestorLineBoxDirty()) {
            container->parent()->dirtyLinesFromChangedChild(container);
            container->setAncestorLineBoxDirty(); // Mark the container to avoid dirtying the same lines again across multiple destroy() calls of the same subtree.
        }
        return;
    }

    // Try to figure out which line box we belong in.  First try to find a previous
    // line box by examining our siblings.  If we didn't find a line box, then use our 
    // parent's first line box.
    RootInlineBox* box = 0;
    RenderObject* curr = 0;
    for (curr = child->previousSibling(); curr; curr = curr->previousSibling()) {
        if (curr->isFloatingOrOutOfFlowPositioned())
            continue;

        if (curr->isReplaced()) {
            InlineBox* wrapper = toRenderBox(curr)->inlineBoxWrapper();
            if (wrapper)
                box = wrapper->root();
        } else if (curr->isText()) {
            InlineTextBox* textBox = toRenderText(curr)->lastTextBox();
            if (textBox)
                box = textBox->root();
        } else if (curr->isRenderInline()) {
            InlineBox* lastSiblingBox = toRenderInline(curr)->lastLineBoxIncludingCulling();
            if (lastSiblingBox)
                box = lastSiblingBox->root();
        }

        if (box)
            break;
    }
    if (!box) {
        if (inlineContainer && !inlineContainer->alwaysCreateLineBoxes()) {
            // https://bugs.webkit.org/show_bug.cgi?id=60778
            // We may have just removed a <br> with no line box that was our first child. In this case
            // we won't find a previous sibling, but firstBox can be pointing to a following sibling.
            // This isn't good enough, since we won't locate the root line box that encloses the removed
            // <br>. We have to just over-invalidate a bit and go up to our parent.
            if (!inlineContainer->ancestorLineBoxDirty()) {
                inlineContainer->parent()->dirtyLinesFromChangedChild(inlineContainer);
                inlineContainer->setAncestorLineBoxDirty(); // Mark the container to avoid dirtying the same lines again across multiple destroy() calls of the same subtree.
            }
            return;
        }
        box = firstBox->root();
    }

    // If we found a line box, then dirty it.
    if (box) {
        RootInlineBox* adjacentBox;
        box->markDirty();

        // dirty the adjacent lines that might be affected
        // NOTE: we dirty the previous line because RootInlineBox objects cache
        // the address of the first object on the next line after a BR, which we may be
        // invalidating here.  For more info, see how RenderBlock::layoutInlineChildren
        // calls setLineBreakInfo with the result of findNextLineBreak.  findNextLineBreak,
        // despite the name, actually returns the first RenderObject after the BR.
        // <rdar://problem/3849947> "Typing after pasting line does not appear until after window resize."
        adjacentBox = box->prevRootBox();
        if (adjacentBox)
            adjacentBox->markDirty();
        adjacentBox = box->nextRootBox();
        // If |child| has been inserted before the first element in the linebox, but after collapsed leading
        // space, the search for |child|'s linebox will go past the leading space to the previous linebox and select that
        // one as |box|. If we hit that situation here, dirty the |box| actually containing the child too. 
        bool insertedAfterLeadingSpace = box->lineBreakObj() == child->previousSibling();
        if (adjacentBox && (adjacentBox->lineBreakObj() == child || child->isBR() || (curr && curr->isBR()) || insertedAfterLeadingSpace))
            adjacentBox->markDirty();
    }
}

#ifndef NDEBUG

void RenderLineBoxList::checkConsistency() const
{
#ifdef CHECK_CONSISTENCY
    const InlineFlowBox* prev = 0;
    for (const InlineFlowBox* child = m_firstLineBox; child != 0; child = child->nextLineBox()) {
        ASSERT(child->prevLineBox() == prev);
        prev = child;
    }
    ASSERT(prev == m_lastLineBox);
#endif
}

#endif

}
