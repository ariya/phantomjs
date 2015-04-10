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

#include "RenderMathMLFraction.h"

#include "GraphicsContext.h"
#include "MathMLNames.h"
#include "PaintInfo.h"

namespace WebCore {
    
using namespace MathMLNames;

static const float gLineThin = 0.33f;
static const float gLineMedium = 1.f;
static const float gLineThick = 3.f;
static const float gFractionBarWidth = 0.05f;

RenderMathMLFraction::RenderMathMLFraction(Element* element)
    : RenderMathMLBlock(element)
    , m_lineThickness(gLineMedium)
{
}

void RenderMathMLFraction::fixChildStyle(RenderObject* child)
{
    ASSERT(child->isAnonymous() && child->style()->refCount() == 1);
    child->style()->setFlexDirection(FlowColumn);
}

// FIXME: It's cleaner to only call updateFromElement when an attribute has changed. Move parts
// of this to fixChildStyle or other methods, and call them when needed.
void RenderMathMLFraction::updateFromElement()
{
    // FIXME: mfrac where bevelled=true will need to reorganize the descendants
    if (isEmpty()) 
        return;
    
    Element* fraction = toElement(node());
    
    RenderObject* numeratorWrapper = firstChild();
    RenderObject* denominatorWrapper = numeratorWrapper->nextSibling();
    if (!denominatorWrapper)
        return;
    
    String thickness = fraction->getAttribute(MathMLNames::linethicknessAttr);
    m_lineThickness = gLineMedium;
    if (equalIgnoringCase(thickness, "thin"))
        m_lineThickness = gLineThin;
    else if (equalIgnoringCase(thickness, "medium"))
        m_lineThickness = gLineMedium;
    else if (equalIgnoringCase(thickness, "thick"))
        m_lineThickness = gLineThick;
    else {
        // This function parses the thickness attribute using gLineMedium as
        // the default value. If the parsing fails, m_lineThickness will not be
        // modified i.e. the default value will be used.
        parseMathMLLength(thickness, m_lineThickness, style(), false);
    }

    // Update the style for the padding of the denominator for the line thickness
    lastChild()->style()->setPaddingTop(Length(static_cast<int>(m_lineThickness), Fixed));
}

void RenderMathMLFraction::addChild(RenderObject* child, RenderObject* /* beforeChild */)
{
    if (isEmpty()) {
        RenderMathMLBlock* numeratorWrapper = createAnonymousMathMLBlock();
        RenderMathMLBlock::addChild(numeratorWrapper);
        fixChildStyle(numeratorWrapper);
        
        RenderMathMLBlock* denominatorWrapper = createAnonymousMathMLBlock();
        RenderMathMLBlock::addChild(denominatorWrapper);
        fixChildStyle(denominatorWrapper);
    }
    
    if (firstChild()->isEmpty())
        firstChild()->addChild(child);
    else
        lastChild()->addChild(child);
    
    updateFromElement();
}

void RenderMathMLFraction::styleDidChange(StyleDifference diff, const RenderStyle* oldStyle)
{
    RenderMathMLBlock::styleDidChange(diff, oldStyle);
    
    for (RenderObject* child = firstChild(); child; child = child->nextSibling())
        fixChildStyle(child);
    updateFromElement();
}

RenderMathMLOperator* RenderMathMLFraction::unembellishedOperator()
{
    RenderObject* numeratorWrapper = firstChild();
    if (!numeratorWrapper)
        return 0;
    RenderObject* numerator = numeratorWrapper->firstChild();
    if (!numerator || !numerator->isRenderMathMLBlock())
        return 0;
    return toRenderMathMLBlock(numerator)->unembellishedOperator();
}

void RenderMathMLFraction::layout()
{
    updateFromElement();

    // Adjust the fraction line thickness for the zoom
    if (lastChild() && lastChild()->isRenderBlock())
        m_lineThickness *= ceilf(gFractionBarWidth * style()->fontSize());

    RenderMathMLBlock::layout();
}

void RenderMathMLFraction::paint(PaintInfo& info, const LayoutPoint& paintOffset)
{
    RenderMathMLBlock::paint(info, paintOffset);
    if (info.context->paintingDisabled() || info.phase != PaintPhaseForeground || style()->visibility() != VISIBLE)
        return;
    
    RenderBox* denominatorWrapper = lastChildBox();
    if (!denominatorWrapper || !m_lineThickness)
        return;

    IntPoint adjustedPaintOffset = roundedIntPoint(paintOffset + location() + denominatorWrapper->location() + LayoutPoint(0, m_lineThickness / 2));
    
    GraphicsContextStateSaver stateSaver(*info.context);
    
    info.context->setStrokeThickness(m_lineThickness);
    info.context->setStrokeStyle(SolidStroke);
    info.context->setStrokeColor(style()->visitedDependentColor(CSSPropertyColor), ColorSpaceSRGB);
    
    info.context->drawLine(adjustedPaintOffset, IntPoint(adjustedPaintOffset.x() + denominatorWrapper->pixelSnappedOffsetWidth(), adjustedPaintOffset.y()));
}

int RenderMathMLFraction::firstLineBoxBaseline() const
{
    if (RenderBox* denominatorWrapper = lastChildBox())
        return denominatorWrapper->logicalTop() + static_cast<int>(lroundf((m_lineThickness + style()->fontMetrics().xHeight()) / 2));
    return RenderMathMLBlock::firstLineBoxBaseline();
}

}

#endif // ENABLE(MATHML)
