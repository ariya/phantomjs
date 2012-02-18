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
#include "RenderText.h"

namespace WebCore {
    
using namespace MathMLNames;

static const float gHorizontalPad = 0.2f;
static const float gLineThin = 0.33f;
static const float gLineMedium = 1.f;
static const float gLineThick = 3.f;
static const float gFractionBarWidth = 0.05f;
static const float gDenominatorPad = 0.1f;

RenderMathMLFraction::RenderMathMLFraction(Element* fraction) 
    : RenderMathMLBlock(fraction)
    , m_lineThickness(gLineMedium)
{
    setChildrenInline(false);
}

void RenderMathMLFraction::updateFromElement()
{
    // FIXME: mfrac where bevelled=true will need to reorganize the descendants
    if (isEmpty()) 
        return;
    
    Element* fraction = static_cast<Element*>(node());
    
    RenderObject* numerator = firstChild();
    String nalign = fraction->getAttribute(MathMLNames::numalignAttr);
    if (equalIgnoringCase(nalign, "left"))
        numerator->style()->setTextAlign(LEFT);
    else if (equalIgnoringCase(nalign, "right"))
        numerator->style()->setTextAlign(RIGHT);
    else
        numerator->style()->setTextAlign(CENTER);
    
    RenderObject* denominator = numerator->nextSibling();
    if (!denominator)
        return;
    
    String dalign = fraction->getAttribute(MathMLNames::denomalignAttr);
    if (equalIgnoringCase(dalign, "left"))
        denominator->style()->setTextAlign(LEFT);
    else if (equalIgnoringCase(dalign, "right"))
        denominator->style()->setTextAlign(RIGHT);
    else
        denominator->style()->setTextAlign(CENTER);
    
    // FIXME: parse units
    String thickness = fraction->getAttribute(MathMLNames::linethicknessAttr);
    m_lineThickness = gLineMedium;
    if (equalIgnoringCase(thickness, "thin"))
        m_lineThickness = gLineThin;
    else if (equalIgnoringCase(thickness, "medium"))
        m_lineThickness = gLineMedium;
    else if (equalIgnoringCase(thickness, "thick"))
        m_lineThickness = gLineThick;
    else if (equalIgnoringCase(thickness, "0"))
        m_lineThickness = 0;

    // Update the style for the padding of the denominator for the line thickness
    lastChild()->style()->setPaddingTop(Length(static_cast<int>(m_lineThickness + style()->fontSize() * gDenominatorPad), Fixed));
}

void RenderMathMLFraction::addChild(RenderObject* child, RenderObject* beforeChild)
{
    RenderBlock* row = new (renderArena()) RenderMathMLBlock(node());
    RefPtr<RenderStyle> rowStyle = makeBlockStyle();
    
    rowStyle->setTextAlign(CENTER);
    Length pad(static_cast<int>(rowStyle->fontSize() * gHorizontalPad), Fixed);
    rowStyle->setPaddingLeft(pad);
    rowStyle->setPaddingRight(pad);
    
    // Only add padding for rows as denominators
    bool isNumerator = isEmpty();
    if (!isNumerator) 
        rowStyle->setPaddingTop(Length(2, Fixed));
    
    row->setStyle(rowStyle.release());
    RenderBlock::addChild(row, beforeChild);
    row->addChild(child);
    updateFromElement();
}

void RenderMathMLFraction::layout()
{
    updateFromElement();

    // Adjust the fraction line thickness for the zoom
    if (lastChild() && lastChild()->isRenderBlock())
        m_lineThickness *= ceilf(gFractionBarWidth * style()->fontSize());

    RenderBlock::layout();

}

void RenderMathMLFraction::paint(PaintInfo& info, int tx, int ty)
{
    RenderMathMLBlock::paint(info, tx, ty);
    if (info.context->paintingDisabled() || info.phase != PaintPhaseForeground)
        return;
    
    if (!firstChild() ||!m_lineThickness)
        return;

    int verticalOffset = 0;
    // The children are always RenderMathMLBlock instances
    if (firstChild()->isRenderMathMLBlock()) {
        int adjustForThickness = m_lineThickness > 1 ? int(m_lineThickness / 2) : 1;
        if (int(m_lineThickness) % 2 == 1)
            adjustForThickness++;
        RenderMathMLBlock* numerator = toRenderMathMLBlock(firstChild());
        if (numerator->isRenderMathMLRow())
            verticalOffset = numerator->offsetHeight() + adjustForThickness;
        else 
            verticalOffset = numerator->offsetHeight();        
    }
    
    tx += x();
    ty += y() + verticalOffset;
    
    GraphicsContextStateSaver stateSaver(*info.context);
    
    info.context->setStrokeThickness(m_lineThickness);
    info.context->setStrokeStyle(SolidStroke);
    info.context->setStrokeColor(style()->visitedDependentColor(CSSPropertyColor), ColorSpaceSRGB);
    
    info.context->drawLine(IntPoint(tx, ty), IntPoint(tx + offsetWidth(), ty));
}

int RenderMathMLFraction::baselinePosition(FontBaseline, bool firstLine, LineDirectionMode lineDirection, LinePositionMode linePositionMode) const
{
    if (firstChild() && firstChild()->isRenderMathMLBlock()) {
        RenderMathMLBlock* numerator = toRenderMathMLBlock(firstChild());
        RenderStyle* refStyle = style();
        if (previousSibling())
            refStyle = previousSibling()->style();
        else if (nextSibling())
            refStyle = nextSibling()->style();
        int shift = int(ceil((refStyle->fontMetrics().xHeight() + 1) / 2));
        return numerator->offsetHeight() + shift;
    }
    return RenderBlock::baselinePosition(AlphabeticBaseline, firstLine, lineDirection, linePositionMode);
}

}


#endif // ENABLE(MATHML)
