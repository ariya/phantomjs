/*
 * Copyright (C) 2009 Alex Milowski (alex@milowski.com). All rights reserved.
 * Copyright (C) 2010 François Sausset (sausset@gmail.com). All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(MATHML)

#include "RenderMathMLRoot.h"

#include "GraphicsContext.h"
#include "PaintInfo.h"
#include "RenderMathMLRow.h"

using namespace std;

namespace WebCore {
    
// FIXME: This whole file should be changed to work with various writing modes. See https://bugs.webkit.org/show_bug.cgi?id=48951.

// Threshold above which the radical shape is modified to look nice with big bases (em)
const float gThresholdBaseHeightEms = 1.5f;
// Normal width of the front of the radical sign, before the base & overbar (em)
const float gFrontWidthEms = 0.75f;
// Gap between the base and overbar (em)
const float gSpaceAboveEms = 0.2f;
// Horizontal position of the bottom point of the radical (* frontWidth)
const float gRadicalBottomPointXFront = 0.5f;
// Lower the radical sign's bottom point (px)
const int gRadicalBottomPointLower = 3;
// Horizontal position of the top left point of the radical "dip" (* frontWidth)
const float gRadicalDipLeftPointXFront = 0.8f;
// Vertical position of the top left point of a sqrt radical "dip" (* baseHeight)
const float gSqrtRadicalDipLeftPointYPos = 0.5f;
// Vertical position of the top left point of an nth root radical "dip" (* baseHeight)
const float gRootRadicalDipLeftPointYPos = 0.625f;
// Vertical shift of the left end point of the radical (em)
const float gRadicalLeftEndYShiftEms = 0.05f;
// Additional bottom root padding if baseHeight > threshold (em)
const float gBigRootBottomPaddingEms = 0.2f;

// Radical line thickness (em)
const float gRadicalLineThicknessEms = 0.02f;
// Radical thick line thickness (em)
const float gRadicalThickLineThicknessEms = 0.1f;
    
RenderMathMLRoot::RenderMathMLRoot(Element* element)
    : RenderMathMLBlock(element)
    , m_intrinsicPaddingBefore(0)
    , m_intrinsicPaddingAfter(0)
    , m_intrinsicPaddingStart(0)
    , m_intrinsicPaddingEnd(0)
{
}

LayoutUnit RenderMathMLRoot::paddingTop() const
{
    LayoutUnit result = computedCSSPaddingTop();
    switch (style()->writingMode()) {
    case TopToBottomWritingMode:
        return result + m_intrinsicPaddingBefore;
    case BottomToTopWritingMode:
        return result + m_intrinsicPaddingAfter;
    case LeftToRightWritingMode:
    case RightToLeftWritingMode:
        return result + (style()->isLeftToRightDirection() ? m_intrinsicPaddingStart : m_intrinsicPaddingEnd);
    }
    ASSERT_NOT_REACHED();
    return result;
}

LayoutUnit RenderMathMLRoot::paddingBottom() const
{
    LayoutUnit result = computedCSSPaddingBottom();
    switch (style()->writingMode()) {
    case TopToBottomWritingMode:
        return result + m_intrinsicPaddingAfter;
    case BottomToTopWritingMode:
        return result + m_intrinsicPaddingBefore;
    case LeftToRightWritingMode:
    case RightToLeftWritingMode:
        return result + (style()->isLeftToRightDirection() ? m_intrinsicPaddingEnd : m_intrinsicPaddingStart);
    }
    ASSERT_NOT_REACHED();
    return result;
}

LayoutUnit RenderMathMLRoot::paddingLeft() const
{
    LayoutUnit result = computedCSSPaddingLeft();
    switch (style()->writingMode()) {
    case LeftToRightWritingMode:
        return result + m_intrinsicPaddingBefore;
    case RightToLeftWritingMode:
        return result + m_intrinsicPaddingAfter;
    case TopToBottomWritingMode:
    case BottomToTopWritingMode:
        return result + (style()->isLeftToRightDirection() ? m_intrinsicPaddingStart : m_intrinsicPaddingEnd);
    }
    ASSERT_NOT_REACHED();
    return result;
}

LayoutUnit RenderMathMLRoot::paddingRight() const
{
    LayoutUnit result = computedCSSPaddingRight();
    switch (style()->writingMode()) {
    case RightToLeftWritingMode:
        return result + m_intrinsicPaddingBefore;
    case LeftToRightWritingMode:
        return result + m_intrinsicPaddingAfter;
    case TopToBottomWritingMode:
    case BottomToTopWritingMode:
        return result + (style()->isLeftToRightDirection() ? m_intrinsicPaddingEnd : m_intrinsicPaddingStart);
    }
    ASSERT_NOT_REACHED();
    return result;
}

LayoutUnit RenderMathMLRoot::paddingBefore() const
{
    return computedCSSPaddingBefore() + m_intrinsicPaddingBefore;
}

LayoutUnit RenderMathMLRoot::paddingAfter() const
{
    return computedCSSPaddingAfter() + m_intrinsicPaddingAfter;
}

LayoutUnit RenderMathMLRoot::paddingStart() const
{
    return computedCSSPaddingStart() + m_intrinsicPaddingStart;
}

LayoutUnit RenderMathMLRoot::paddingEnd() const
{
    return computedCSSPaddingEnd() + m_intrinsicPaddingEnd;
}

void RenderMathMLRoot::addChild(RenderObject* newChild, RenderObject* beforeChild)
{
    // Insert an implicit <mrow> for <mroot> as well as <msqrt>, to ensure firstChild() will have a box
    // to measure and store a glyph-based height for preferredLogicalHeightAfterSizing.
    if (!firstChild())
        RenderMathMLBlock::addChild(RenderMathMLRow::createAnonymousWithParentRenderer(this));
    
    // An <mroot>'s index has { position: absolute }.
    if (newChild->style()->position() == AbsolutePosition)
        RenderMathMLBlock::addChild(newChild);
    else
        firstChild()->addChild(newChild, beforeChild && beforeChild->parent() == firstChild() ? beforeChild : 0);
}

RenderBoxModelObject* RenderMathMLRoot::index() const
{
    if (!firstChild())
        return 0;
    RenderObject* index = firstChild()->nextSibling();
    if (!index || !index->isBoxModelObject())
        return 0;
    return toRenderBoxModelObject(index);
}

void RenderMathMLRoot::computePreferredLogicalWidths()
{
    ASSERT(preferredLogicalWidthsDirty() && needsLayout());
    
#ifndef NDEBUG
    // FIXME: Remove this once mathml stops modifying the render tree here.
    SetLayoutNeededForbiddenScope layoutForbiddenScope(this, false);
#endif
    
    computeChildrenPreferredLogicalHeights();
    
    int baseHeight = firstChild() ? roundToInt(preferredLogicalHeightAfterSizing(firstChild())) : style()->fontSize();
    
    int frontWidth = lroundf(gFrontWidthEms * style()->fontSize());
    
    // Base height above which the shape of the root changes
    float thresholdHeight = gThresholdBaseHeightEms * style()->fontSize();
    if (baseHeight > thresholdHeight && thresholdHeight) {
        float shift = min<float>((baseHeight - thresholdHeight) / thresholdHeight, 1.0f);
        m_overbarLeftPointShift = static_cast<int>(shift * gRadicalBottomPointXFront * frontWidth);
        m_intrinsicPaddingAfter = lroundf(gBigRootBottomPaddingEms * style()->fontSize());
    } else {
        m_overbarLeftPointShift = 0;
        m_intrinsicPaddingAfter = 0;
    }
    
    int rootPad = lroundf(gSpaceAboveEms * style()->fontSize());
    m_intrinsicPaddingBefore = rootPad;
    m_indexTop = 0;
    if (RenderBoxModelObject* index = this->index()) {
        m_intrinsicPaddingStart = roundToInt(index->maxPreferredLogicalWidth()) + m_overbarLeftPointShift;
        
        int indexHeight = roundToInt(preferredLogicalHeightAfterSizing(index));
        int partDipHeight = lroundf((1 - gRootRadicalDipLeftPointYPos) * baseHeight);
        int rootExtraTop = partDipHeight + indexHeight - (baseHeight + rootPad);
        if (rootExtraTop > 0)
            m_intrinsicPaddingBefore += rootExtraTop;
        else
            m_indexTop = - rootExtraTop;
    } else
        m_intrinsicPaddingStart = frontWidth;

    RenderMathMLBlock::computePreferredLogicalWidths();
    
    // Shrink our logical width to its probable value now without triggering unnecessary relayout of our children.
    ASSERT(needsLayout() && logicalWidth() >= maxPreferredLogicalWidth());
    setLogicalWidth(maxPreferredLogicalWidth());
}

void RenderMathMLRoot::layout()
{
    // Our computePreferredLogicalWidths() may change our logical width and then layout our children, which
    // RenderBlock::layout()'s relayoutChildren logic isn't expecting.
    if (preferredLogicalWidthsDirty())
        computePreferredLogicalWidths();
    
    RenderMathMLBlock::layout();
    
    RenderBoxModelObject* index = this->index();
    // If |index|, it should be a RenderBlock here, unless the user has overriden its { position: absolute }.
    if (index && index->isBox())
        toRenderBox(index)->setLogicalTop(m_indexTop);
}

void RenderMathMLRoot::paint(PaintInfo& info, const LayoutPoint& paintOffset)
{
    RenderMathMLBlock::paint(info, paintOffset);
    
    if (info.context->paintingDisabled() || style()->visibility() != VISIBLE)
        return;
    
    IntPoint adjustedPaintOffset = roundedIntPoint(paintOffset + location() + contentBoxRect().location());
    
    int startX = adjustedPaintOffset.x();
    int frontWidth = lroundf(gFrontWidthEms * style()->fontSize());
    int overbarWidth = roundToInt(contentLogicalWidth()) + m_overbarLeftPointShift;
    
    int baseHeight = roundToInt(contentLogicalHeight());
    int rootPad = lroundf(gSpaceAboveEms * style()->fontSize());
    adjustedPaintOffset.setY(adjustedPaintOffset.y() - rootPad);
    
    float radicalDipLeftPointYPos = (index() ? gRootRadicalDipLeftPointYPos : gSqrtRadicalDipLeftPointYPos) * baseHeight;
    
    FloatPoint overbarLeftPoint(startX - m_overbarLeftPointShift, adjustedPaintOffset.y());
    FloatPoint bottomPoint(startX - gRadicalBottomPointXFront * frontWidth, adjustedPaintOffset.y() + baseHeight + gRadicalBottomPointLower);
    FloatPoint dipLeftPoint(startX - gRadicalDipLeftPointXFront * frontWidth, adjustedPaintOffset.y() + radicalDipLeftPointYPos);
    FloatPoint leftEnd(startX - frontWidth, dipLeftPoint.y() + gRadicalLeftEndYShiftEms * style()->fontSize());
    
    GraphicsContextStateSaver stateSaver(*info.context);
    
    info.context->setStrokeThickness(gRadicalLineThicknessEms * style()->fontSize());
    info.context->setStrokeStyle(SolidStroke);
    info.context->setStrokeColor(style()->visitedDependentColor(CSSPropertyColor), ColorSpaceDeviceRGB);
    info.context->setLineJoin(MiterJoin);
    info.context->setMiterLimit(style()->fontSize());
    
    Path root;
    
    root.moveTo(FloatPoint(overbarLeftPoint.x() + overbarWidth, adjustedPaintOffset.y()));
    // draw top
    root.addLineTo(overbarLeftPoint);
    // draw from top left corner to bottom point of radical
    root.addLineTo(bottomPoint);
    // draw from bottom point to top of left part of radical base "dip"
    root.addLineTo(dipLeftPoint);
    // draw to end
    root.addLineTo(leftEnd);
    
    info.context->strokePath(root);
    
    GraphicsContextStateSaver maskStateSaver(*info.context);
    
    // Build a mask to draw the thick part of the root.
    Path mask;
    
    mask.moveTo(overbarLeftPoint);
    mask.addLineTo(bottomPoint);
    mask.addLineTo(dipLeftPoint);
    mask.addLineTo(FloatPoint(2 * dipLeftPoint.x() - leftEnd.x(), 2 * dipLeftPoint.y() - leftEnd.y()));
    
    info.context->clip(mask);
    
    // Draw the thick part of the root.
    info.context->setStrokeThickness(gRadicalThickLineThicknessEms * style()->fontSize());
    info.context->setLineCap(SquareCap);
    
    Path line;
    line.moveTo(bottomPoint);
    line.addLineTo(dipLeftPoint);
    
    info.context->strokePath(line);
}

}

#endif // ENABLE(MATHML)
