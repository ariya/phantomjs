/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2000 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2006, 2007 Apple Inc. All rights reserved.
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
 *
 */

#include "config.h"
#include "RenderReplaced.h"

#include "GraphicsContext.h"
#include "RenderBlock.h"
#include "RenderLayer.h"
#include "RenderTheme.h"
#include "RenderView.h"
#include "VisiblePosition.h"

using namespace std;

namespace WebCore {

const int cDefaultWidth = 300;
const int cDefaultHeight = 150;

RenderReplaced::RenderReplaced(Node* node)
    : RenderBox(node)
    , m_intrinsicSize(cDefaultWidth, cDefaultHeight)
    , m_hasIntrinsicSize(false)
{
    setReplaced(true);
}

RenderReplaced::RenderReplaced(Node* node, const IntSize& intrinsicSize)
    : RenderBox(node)
    , m_intrinsicSize(intrinsicSize)
    , m_hasIntrinsicSize(true)
{
    setReplaced(true);
}

RenderReplaced::~RenderReplaced()
{
}

void RenderReplaced::destroy()
{
    if (!documentBeingDestroyed() && parent())
        parent()->dirtyLinesFromChangedChild(this);

    RenderBox::destroy();
}

void RenderReplaced::styleDidChange(StyleDifference diff, const RenderStyle* oldStyle)
{
    RenderBox::styleDidChange(diff, oldStyle);

    bool hadStyle = (oldStyle != 0);
    float oldZoom = hadStyle ? oldStyle->effectiveZoom() : RenderStyle::initialZoom();
    if (style() && style()->effectiveZoom() != oldZoom)
        intrinsicSizeChanged();
}

void RenderReplaced::layout()
{
    ASSERT(needsLayout());
    
    LayoutRepainter repainter(*this, checkForRepaintDuringLayout());
    
    setHeight(minimumReplacedHeight());

    computeLogicalWidth();
    computeLogicalHeight();

    m_overflow.clear();
    addShadowOverflow();
    updateLayerTransform();
    
    repainter.repaintAfterLayout();
    setNeedsLayout(false);
}
 
void RenderReplaced::intrinsicSizeChanged()
{
    int scaledWidth = static_cast<int>(cDefaultWidth * style()->effectiveZoom());
    int scaledHeight = static_cast<int>(cDefaultHeight * style()->effectiveZoom());
    m_intrinsicSize = IntSize(scaledWidth, scaledHeight);
    setNeedsLayoutAndPrefWidthsRecalc();
}

void RenderReplaced::paint(PaintInfo& paintInfo, int tx, int ty)
{
    if (!shouldPaint(paintInfo, tx, ty))
        return;
    
    tx += x();
    ty += y();
    
    if (hasBoxDecorations() && (paintInfo.phase == PaintPhaseForeground || paintInfo.phase == PaintPhaseSelection)) 
        paintBoxDecorations(paintInfo, tx, ty);
    
    if (paintInfo.phase == PaintPhaseMask) {
        paintMask(paintInfo, tx, ty);
        return;
    }

    if ((paintInfo.phase == PaintPhaseOutline || paintInfo.phase == PaintPhaseSelfOutline) && style()->outlineWidth())
        paintOutline(paintInfo.context, tx, ty, width(), height());
    
    if (paintInfo.phase != PaintPhaseForeground && paintInfo.phase != PaintPhaseSelection)
        return;
    
    if (!paintInfo.shouldPaintWithinRoot(this))
        return;
    
    bool drawSelectionTint = selectionState() != SelectionNone && !document()->printing();
    if (paintInfo.phase == PaintPhaseSelection) {
        if (selectionState() == SelectionNone)
            return;
        drawSelectionTint = false;
    }

    bool completelyClippedOut = false;
    if (style()->hasBorderRadius()) {
        IntRect borderRect = IntRect(tx, ty, width(), height());

        if (borderRect.isEmpty())
            completelyClippedOut = true;
        else {
            // Push a clip if we have a border radius, since we want to round the foreground content that gets painted.
            paintInfo.context->save();
            paintInfo.context->addRoundedRectClip(style()->getRoundedBorderFor(borderRect));
        }
    }

    if (!completelyClippedOut) {
        paintReplaced(paintInfo, tx, ty);

        if (style()->hasBorderRadius())
            paintInfo.context->restore();
    }
        
    // The selection tint never gets clipped by border-radius rounding, since we want it to run right up to the edges of
    // surrounding content.
    if (drawSelectionTint) {
        IntRect selectionPaintingRect = localSelectionRect();
        selectionPaintingRect.move(tx, ty);
        paintInfo.context->fillRect(selectionPaintingRect, selectionBackgroundColor(), style()->colorSpace());
    }
}

bool RenderReplaced::shouldPaint(PaintInfo& paintInfo, int& tx, int& ty)
{
    if (paintInfo.phase != PaintPhaseForeground && paintInfo.phase != PaintPhaseOutline && paintInfo.phase != PaintPhaseSelfOutline 
            && paintInfo.phase != PaintPhaseSelection && paintInfo.phase != PaintPhaseMask)
        return false;

    if (!paintInfo.shouldPaintWithinRoot(this))
        return false;
        
    // if we're invisible or haven't received a layout yet, then just bail.
    if (style()->visibility() != VISIBLE)
        return false;

    int currentTX = tx + x();
    int currentTY = ty + y();

    // Early exit if the element touches the edges.
    int top = currentTY + minYVisualOverflow();
    int bottom = currentTY + maxYVisualOverflow();
    if (isSelected() && m_inlineBoxWrapper) {
        int selTop = ty + m_inlineBoxWrapper->root()->selectionTop();
        int selBottom = ty + selTop + m_inlineBoxWrapper->root()->selectionHeight();
        top = min(selTop, top);
        bottom = max(selBottom, bottom);
    }
    
    int os = 2 * maximalOutlineSize(paintInfo.phase);
    if (currentTX + minXVisualOverflow() >= paintInfo.rect.maxX() + os || currentTX + maxXVisualOverflow() <= paintInfo.rect.x() - os)
        return false;
    if (top >= paintInfo.rect.maxY() + os || bottom <= paintInfo.rect.y() - os)
        return false;

    return true;
}

static inline bool lengthIsSpecified(Length length)
{
    LengthType lengthType = length.type();
    return lengthType == Fixed || lengthType == Percent;
}

int RenderReplaced::computeReplacedLogicalWidth(bool includeMaxWidth) const
{
    int logicalWidth;
    if (lengthIsSpecified(style()->width()))
        logicalWidth = computeReplacedLogicalWidthUsing(style()->logicalWidth());
    else if (m_hasIntrinsicSize)
        logicalWidth = calcAspectRatioLogicalWidth();
    else
        logicalWidth = intrinsicLogicalWidth();

    int minLogicalWidth = computeReplacedLogicalWidthUsing(style()->logicalMinWidth());
    int maxLogicalWidth = !includeMaxWidth || style()->logicalMaxWidth().isUndefined() ? logicalWidth : computeReplacedLogicalWidthUsing(style()->logicalMaxWidth());

    return max(minLogicalWidth, min(logicalWidth, maxLogicalWidth));
}

int RenderReplaced::computeReplacedLogicalHeight() const
{
    int logicalHeight;
    if (lengthIsSpecified(style()->logicalHeight()))
        logicalHeight = computeReplacedLogicalHeightUsing(style()->logicalHeight());
    else if (m_hasIntrinsicSize)
        logicalHeight = calcAspectRatioLogicalHeight();
    else
        logicalHeight = intrinsicLogicalHeight();

    int minLogicalHeight = computeReplacedLogicalHeightUsing(style()->logicalMinHeight());
    int maxLogicalHeight = style()->logicalMaxHeight().isUndefined() ? logicalHeight : computeReplacedLogicalHeightUsing(style()->logicalMaxHeight());

    return max(minLogicalHeight, min(logicalHeight, maxLogicalHeight));
}

int RenderReplaced::calcAspectRatioLogicalWidth() const
{
    int intrinsicWidth = intrinsicLogicalWidth();
    int intrinsicHeight = intrinsicLogicalHeight();
    if (!intrinsicHeight)
        return 0;
    return RenderBox::computeReplacedLogicalHeight() * intrinsicWidth / intrinsicHeight;
}

int RenderReplaced::calcAspectRatioLogicalHeight() const
{
    int intrinsicWidth = intrinsicLogicalWidth();
    int intrinsicHeight = intrinsicLogicalHeight();
    if (!intrinsicWidth)
        return 0;
    return RenderBox::computeReplacedLogicalWidth() * intrinsicHeight / intrinsicWidth;
}

void RenderReplaced::computePreferredLogicalWidths()
{
    ASSERT(preferredLogicalWidthsDirty());

    int borderAndPadding = borderAndPaddingWidth();
    m_maxPreferredLogicalWidth = computeReplacedLogicalWidth(false) + borderAndPadding;

    if (style()->maxWidth().isFixed() && style()->maxWidth().value() != undefinedLength)
        m_maxPreferredLogicalWidth = min(m_maxPreferredLogicalWidth, style()->maxWidth().value() + (style()->boxSizing() == CONTENT_BOX ? borderAndPadding : 0));

    if (style()->width().isPercent() || style()->height().isPercent()
        || style()->maxWidth().isPercent() || style()->maxHeight().isPercent()
        || style()->minWidth().isPercent() || style()->minHeight().isPercent())
        m_minPreferredLogicalWidth = 0;
    else
        m_minPreferredLogicalWidth = m_maxPreferredLogicalWidth;

    setPreferredLogicalWidthsDirty(false);
}

unsigned RenderReplaced::caretMaxRenderedOffset() const
{
    return 1; 
}

VisiblePosition RenderReplaced::positionForPoint(const IntPoint& point)
{
    InlineBox* box = inlineBoxWrapper();
    if (!box)
        return createVisiblePosition(0, DOWNSTREAM);

    // FIXME: This code is buggy if the replaced element is relative positioned.

    RootInlineBox* root = box->root();

    int top = root->selectionTop();
    int bottom = root->selectionBottom();

    int blockDirectionPosition = box->isHorizontal() ? point.y() + y() : point.x() + x();
    int lineDirectionPosition = box->isHorizontal() ? point.x() + x() : point.y() + y();

    if (blockDirectionPosition < top)
        return createVisiblePosition(caretMinOffset(), DOWNSTREAM); // coordinates are above
    
    if (blockDirectionPosition >= bottom)
        return createVisiblePosition(caretMaxOffset(), DOWNSTREAM); // coordinates are below
    
    if (node()) {
        if (lineDirectionPosition <= box->logicalLeft() + (box->logicalWidth() / 2))
            return createVisiblePosition(0, DOWNSTREAM);
        return createVisiblePosition(1, DOWNSTREAM);
    }

    return RenderBox::positionForPoint(point);
}

IntRect RenderReplaced::selectionRectForRepaint(RenderBoxModelObject* repaintContainer, bool clipToVisibleContent)
{
    ASSERT(!needsLayout());

    if (!isSelected())
        return IntRect();
    
    IntRect rect = localSelectionRect();
    if (clipToVisibleContent)
        computeRectForRepaint(repaintContainer, rect);
    else
        rect = localToContainerQuad(FloatRect(rect), repaintContainer).enclosingBoundingBox();
    
    return rect;
}

IntRect RenderReplaced::localSelectionRect(bool checkWhetherSelected) const
{
    if (checkWhetherSelected && !isSelected())
        return IntRect();

    if (!m_inlineBoxWrapper)
        // We're a block-level replaced element.  Just return our own dimensions.
        return IntRect(0, 0, width(), height());
    
    RootInlineBox* root = m_inlineBoxWrapper->root();
    int newLogicalTop = root->block()->style()->isFlippedBlocksWritingMode() ? m_inlineBoxWrapper->logicalBottom() - root->selectionBottom() : root->selectionTop() - m_inlineBoxWrapper->logicalTop();
    if (root->block()->style()->isHorizontalWritingMode())
        return IntRect(0, newLogicalTop, width(), root->selectionHeight());
    return IntRect(newLogicalTop, 0, root->selectionHeight(), height());
}

void RenderReplaced::setSelectionState(SelectionState s)
{
    RenderBox::setSelectionState(s); // The selection state for our containing block hierarchy is updated by the base class call.
    if (m_inlineBoxWrapper) {
        RootInlineBox* line = m_inlineBoxWrapper->root();
        if (line)
            line->setHasSelectedChildren(isSelected());
    }
}

bool RenderReplaced::isSelected() const
{
    SelectionState s = selectionState();
    if (s == SelectionNone)
        return false;
    if (s == SelectionInside)
        return true;

    int selectionStart, selectionEnd;
    selectionStartEnd(selectionStart, selectionEnd);
    if (s == SelectionStart)
        return selectionStart == 0;
        
    int end = node()->hasChildNodes() ? node()->childNodeCount() : 1;
    if (s == SelectionEnd)
        return selectionEnd == end;
    if (s == SelectionBoth)
        return selectionStart == 0 && selectionEnd == end;
        
    ASSERT(0);
    return false;
}

IntSize RenderReplaced::intrinsicSize() const
{
    return m_intrinsicSize;
}

void RenderReplaced::setIntrinsicSize(const IntSize& size)
{
    ASSERT(m_hasIntrinsicSize);
    m_intrinsicSize = size;
}

IntRect RenderReplaced::clippedOverflowRectForRepaint(RenderBoxModelObject* repaintContainer)
{
    if (style()->visibility() != VISIBLE && !enclosingLayer()->hasVisibleContent())
        return IntRect();

    // The selectionRect can project outside of the overflowRect, so take their union
    // for repainting to avoid selection painting glitches.
    IntRect r = unionRect(localSelectionRect(false), visualOverflowRect());

    RenderView* v = view();
    if (v) {
        // FIXME: layoutDelta needs to be applied in parts before/after transforms and
        // repaint containers. https://bugs.webkit.org/show_bug.cgi?id=23308
        r.move(v->layoutDelta());
    }

    if (style()) {
        if (style()->hasAppearance())
            // The theme may wish to inflate the rect used when repainting.
            theme()->adjustRepaintRect(this, r);
        if (v)
            r.inflate(style()->outlineSize());
    }
    computeRectForRepaint(repaintContainer, r);
    return r;
}

}
