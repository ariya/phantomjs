/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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

#include "config.h"
#include "RenderScrollbarPart.h"

#include "PaintInfo.h"
#include "RenderScrollbar.h"
#include "RenderScrollbarTheme.h"
#include "RenderView.h"
#include <wtf/StackStats.h>

using namespace std;

namespace WebCore {

RenderScrollbarPart::RenderScrollbarPart(RenderScrollbar* scrollbar, ScrollbarPart part)
    : RenderBlock(0)
    , m_scrollbar(scrollbar)
    , m_part(part)
{
}

RenderScrollbarPart::~RenderScrollbarPart()
{
}

RenderScrollbarPart* RenderScrollbarPart::createAnonymous(Document* document, RenderScrollbar* scrollbar, ScrollbarPart part)
{
    RenderScrollbarPart* renderer = new (document->renderArena()) RenderScrollbarPart(scrollbar, part);
    renderer->setDocumentForAnonymous(document);
    return renderer;
}

void RenderScrollbarPart::layout()
{
    StackStats::LayoutCheckPoint layoutCheckPoint;
    setLocation(LayoutPoint()); // We don't worry about positioning ourselves. We're just determining our minimum width/height.
    if (m_scrollbar->orientation() == HorizontalScrollbar)
        layoutHorizontalPart();
    else
        layoutVerticalPart();

    setNeedsLayout(false);
}

void RenderScrollbarPart::layoutHorizontalPart()
{
    if (m_part == ScrollbarBGPart) {
        setWidth(m_scrollbar->width());
        computeScrollbarHeight();
    } else {
        computeScrollbarWidth();
        setHeight(m_scrollbar->height());
    }
}

void RenderScrollbarPart::layoutVerticalPart()
{
    if (m_part == ScrollbarBGPart) {
        computeScrollbarWidth();
        setHeight(m_scrollbar->height());
    } else {
        setWidth(m_scrollbar->width());
        computeScrollbarHeight();
    } 
}

static int calcScrollbarThicknessUsing(SizeType sizeType, const Length& length, int containingLength, RenderView* renderView)
{
    if (!length.isIntrinsicOrAuto() || (sizeType == MinSize && length.isAuto()))
        return minimumValueForLength(length, containingLength, renderView);
    return ScrollbarTheme::theme()->scrollbarThickness();
}

void RenderScrollbarPart::computeScrollbarWidth()
{
    if (!m_scrollbar->owningRenderer())
        return;
    RenderView* renderView = view();
    // FIXME: We are querying layout information but nothing guarantees that it's up-to-date, especially since we are called at style change.
    // FIXME: Querying the style's border information doesn't work on table cells with collapsing borders.
    int visibleSize = m_scrollbar->owningRenderer()->width() - m_scrollbar->owningRenderer()->style()->borderLeftWidth() - m_scrollbar->owningRenderer()->style()->borderRightWidth();
    int w = calcScrollbarThicknessUsing(MainOrPreferredSize, style()->width(), visibleSize, renderView);
    int minWidth = calcScrollbarThicknessUsing(MinSize, style()->minWidth(), visibleSize, renderView);
    int maxWidth = style()->maxWidth().isUndefined() ? w : calcScrollbarThicknessUsing(MaxSize, style()->maxWidth(), visibleSize, renderView);
    setWidth(max(minWidth, min(maxWidth, w)));
    
    // Buttons and track pieces can all have margins along the axis of the scrollbar. 
    m_marginBox.setLeft(minimumValueForLength(style()->marginLeft(), visibleSize, renderView));
    m_marginBox.setRight(minimumValueForLength(style()->marginRight(), visibleSize, renderView));
}

void RenderScrollbarPart::computeScrollbarHeight()
{
    if (!m_scrollbar->owningRenderer())
        return;
    RenderView* renderView = view();
    // FIXME: We are querying layout information but nothing guarantees that it's up-to-date, especially since we are called at style change.
    // FIXME: Querying the style's border information doesn't work on table cells with collapsing borders.
    int visibleSize = m_scrollbar->owningRenderer()->height() -  m_scrollbar->owningRenderer()->style()->borderTopWidth() - m_scrollbar->owningRenderer()->style()->borderBottomWidth();
    int h = calcScrollbarThicknessUsing(MainOrPreferredSize, style()->height(), visibleSize, renderView);
    int minHeight = calcScrollbarThicknessUsing(MinSize, style()->minHeight(), visibleSize, renderView);
    int maxHeight = style()->maxHeight().isUndefined() ? h : calcScrollbarThicknessUsing(MaxSize, style()->maxHeight(), visibleSize, renderView);
    setHeight(max(minHeight, min(maxHeight, h)));

    // Buttons and track pieces can all have margins along the axis of the scrollbar. 
    m_marginBox.setTop(minimumValueForLength(style()->marginTop(), visibleSize, renderView));
    m_marginBox.setBottom(minimumValueForLength(style()->marginBottom(), visibleSize, renderView));
}

void RenderScrollbarPart::computePreferredLogicalWidths()
{
    if (!preferredLogicalWidthsDirty())
        return;
    
    m_minPreferredLogicalWidth = m_maxPreferredLogicalWidth = 0;

    setPreferredLogicalWidthsDirty(false);
}

void RenderScrollbarPart::styleWillChange(StyleDifference diff, const RenderStyle* newStyle)
{
    RenderBlock::styleWillChange(diff, newStyle);
    setInline(false);
}

void RenderScrollbarPart::styleDidChange(StyleDifference diff, const RenderStyle* oldStyle)
{
    RenderBlock::styleDidChange(diff, oldStyle);
    setInline(false);
    clearPositionedState();
    setFloating(false);
    setHasOverflowClip(false);
    if (oldStyle && m_scrollbar && m_part != NoPart && diff >= StyleDifferenceRepaint)
        m_scrollbar->theme()->invalidatePart(m_scrollbar, m_part);
}

void RenderScrollbarPart::imageChanged(WrappedImagePtr image, const IntRect* rect)
{
    if (m_scrollbar && m_part != NoPart)
        m_scrollbar->theme()->invalidatePart(m_scrollbar, m_part);
    else {
        if (FrameView* frameView = view()->frameView()) {
            if (frameView->isFrameViewScrollCorner(this)) {
                frameView->invalidateScrollCorner(frameView->scrollCornerRect());
                return;
            }
        }
        
        RenderBlock::imageChanged(image, rect);
    }
}

void RenderScrollbarPart::paintIntoRect(GraphicsContext* graphicsContext, const LayoutPoint& paintOffset, const LayoutRect& rect)
{
    // Make sure our dimensions match the rect.
    setLocation(rect.location() - toSize(paintOffset));
    setWidth(rect.width());
    setHeight(rect.height());

    if (graphicsContext->paintingDisabled())
        return;

    // Now do the paint.
    PaintInfo paintInfo(graphicsContext, pixelSnappedIntRect(rect), PaintPhaseBlockBackground, PaintBehaviorNormal);
    paint(paintInfo, paintOffset);
    paintInfo.phase = PaintPhaseChildBlockBackgrounds;
    paint(paintInfo, paintOffset);
    paintInfo.phase = PaintPhaseFloat;
    paint(paintInfo, paintOffset);
    paintInfo.phase = PaintPhaseForeground;
    paint(paintInfo, paintOffset);
    paintInfo.phase = PaintPhaseOutline;
    paint(paintInfo, paintOffset);
}

RenderObject* RenderScrollbarPart::rendererOwningScrollbar() const
{
    if (!m_scrollbar)
        return 0;
    return m_scrollbar->owningRenderer();
}

}
