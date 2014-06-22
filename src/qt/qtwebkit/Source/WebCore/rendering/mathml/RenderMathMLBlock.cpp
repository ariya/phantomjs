/*
 * Copyright (C) 2009 Alex Milowski (alex@milowski.com). All rights reserved.
 * Copyright (C) 2012 David Barton (dbarton@mathscribe.com). All rights reserved.
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

#include "RenderMathMLBlock.h"

#include "GraphicsContext.h"
#include "MathMLNames.h"
#include "RenderView.h"
#include <wtf/text/StringBuilder.h>

#if ENABLE(DEBUG_MATH_LAYOUT)
#include "PaintInfo.h"
#endif

namespace WebCore {
    
using namespace MathMLNames;
    
RenderMathMLBlock::RenderMathMLBlock(Element* container)
    : RenderFlexibleBox(container)
    , m_ignoreInAccessibilityTree(false)
    , m_preferredLogicalHeight(preferredLogicalHeightUnset)
{
}

bool RenderMathMLBlock::isChildAllowed(RenderObject* child, RenderStyle*) const
{
    return child->node() && child->node()->nodeType() == Node::ELEMENT_NODE;
}

void RenderMathMLBlock::computePreferredLogicalWidths()
{
    ASSERT(preferredLogicalWidthsDirty());
    m_preferredLogicalHeight = preferredLogicalHeightUnset;
    RenderFlexibleBox::computePreferredLogicalWidths();
}

RenderMathMLBlock* RenderMathMLBlock::createAnonymousMathMLBlock(EDisplay display)
{
    RefPtr<RenderStyle> newStyle = RenderStyle::createAnonymousStyleWithDisplay(style(), display);
    RenderMathMLBlock* newBlock = new (renderArena()) RenderMathMLBlock(0);
    newBlock->setDocumentForAnonymous(document());
    newBlock->setStyle(newStyle.release());
    return newBlock;
}

// An arbitrary large value, like RenderBlock.cpp BLOCK_MAX_WIDTH or FixedTableLayout.cpp TABLE_MAX_WIDTH.
static const int cLargeLogicalWidth = 15000;

void RenderMathMLBlock::computeChildrenPreferredLogicalHeights()
{
    ASSERT(needsLayout());

    // This is ugly, but disable fragmentation when computing the preferred heights.
    FragmentationDisabler fragmentationDisabler(this);

    // Ensure a full repaint will happen after layout finishes.
    setNeedsLayout(true, MarkOnlyThis);

    RenderView* renderView = view();
    bool hadLayoutState = renderView->layoutState();
    if (!hadLayoutState)
        renderView->pushLayoutState(this);
    {
        LayoutStateDisabler layoutStateDisabler(renderView);
        
        LayoutUnit oldAvailableLogicalWidth = availableLogicalWidth();
        setLogicalWidth(cLargeLogicalWidth);
        
        for (RenderObject* child = firstChild(); child; child = child->nextSibling()) {
            if (!child->isBox())
                continue;
            
            // Because our width changed, |child| may need layout.
            if (child->maxPreferredLogicalWidth() > oldAvailableLogicalWidth)
                child->setNeedsLayout(true, MarkOnlyThis);
            
            RenderMathMLBlock* childMathMLBlock = child->isRenderMathMLBlock() ? toRenderMathMLBlock(child) : 0;
            if (childMathMLBlock && !childMathMLBlock->isPreferredLogicalHeightDirty())
                continue;
            // Layout our child to compute its preferred logical height.
            child->layoutIfNeeded();
            if (childMathMLBlock)
                childMathMLBlock->setPreferredLogicalHeight(childMathMLBlock->logicalHeight());
        }
    }
    if (!hadLayoutState)
        renderView->popLayoutState(this);
}

LayoutUnit RenderMathMLBlock::preferredLogicalHeightAfterSizing(RenderObject* child)
{
    if (child->isRenderMathMLBlock())
        return toRenderMathMLBlock(child)->preferredLogicalHeight();
    if (child->isBox()) {
        ASSERT(!child->needsLayout());
        return toRenderBox(child)->logicalHeight();
    }
    // This currently ignores -webkit-line-box-contain:
    return child->style()->fontSize();
}

int RenderMathMLBlock::baselinePosition(FontBaseline baselineType, bool firstLine, LineDirectionMode direction, LinePositionMode linePositionMode) const
{
    // mathml.css sets math { -webkit-line-box-contain: glyphs replaced; line-height: 0; }, so when linePositionMode == PositionOfInteriorLineBoxes we want to
    // return 0 here to match our line-height. This matters when RootInlineBox::ascentAndDescentForBox is called on a RootInlineBox for an inline-block.
    if (linePositionMode == PositionOfInteriorLineBoxes)
        return 0;
    
    LayoutUnit baseline = firstLineBoxBaseline(); // FIXME: This may be unnecessary after flex baselines are implemented (https://bugs.webkit.org/show_bug.cgi?id=96188).
    if (baseline != -1)
        return baseline;
    
    return RenderFlexibleBox::baselinePosition(baselineType, firstLine, direction, linePositionMode);
}

const char* RenderMathMLBlock::renderName() const
{
    EDisplay display = style()->display();
    if (display == FLEX)
        return isAnonymous() ? "RenderMathMLBlock (anonymous, flex)" : "RenderMathMLBlock (flex)";
    if (display == INLINE_FLEX)
        return isAnonymous() ? "RenderMathMLBlock (anonymous, inline-flex)" : "RenderMathMLBlock (inline-flex)";
    // |display| should be one of the above.
    ASSERT_NOT_REACHED();
    return isAnonymous() ? "RenderMathMLBlock (anonymous)" : "RenderMathMLBlock";
}

#if ENABLE(DEBUG_MATH_LAYOUT)
void RenderMathMLBlock::paint(PaintInfo& info, const LayoutPoint& paintOffset)
{
    RenderFlexibleBox::paint(info, paintOffset);
    
    if (info.context->paintingDisabled() || info.phase != PaintPhaseForeground)
        return;

    IntPoint adjustedPaintOffset = roundedIntPoint(paintOffset + location());

    GraphicsContextStateSaver stateSaver(*info.context);
    
    info.context->setStrokeThickness(1.0f);
    info.context->setStrokeStyle(SolidStroke);
    info.context->setStrokeColor(Color(0, 0, 255), ColorSpaceSRGB);
    
    info.context->drawLine(adjustedPaintOffset, IntPoint(adjustedPaintOffset.x() + pixelSnappedOffsetWidth(), adjustedPaintOffset.y()));
    info.context->drawLine(IntPoint(adjustedPaintOffset.x() + pixelSnappedOffsetWidth(), adjustedPaintOffset.y()), IntPoint(adjustedPaintOffset.x() + pixelSnappedOffsetWidth(), adjustedPaintOffset.y() + pixelSnappedOffsetHeight()));
    info.context->drawLine(IntPoint(adjustedPaintOffset.x(), adjustedPaintOffset.y() + pixelSnappedOffsetHeight()), IntPoint(adjustedPaintOffset.x() + pixelSnappedOffsetWidth(), adjustedPaintOffset.y() + pixelSnappedOffsetHeight()));
    info.context->drawLine(adjustedPaintOffset, IntPoint(adjustedPaintOffset.x(), adjustedPaintOffset.y() + pixelSnappedOffsetHeight()));
    
    int topStart = paddingTop();
    
    info.context->setStrokeColor(Color(0, 255, 0), ColorSpaceSRGB);
    
    info.context->drawLine(IntPoint(adjustedPaintOffset.x(), adjustedPaintOffset.y() + topStart), IntPoint(adjustedPaintOffset.x() + pixelSnappedOffsetWidth(), adjustedPaintOffset.y() + topStart));
    
    int baseline = roundToInt(baselinePosition(AlphabeticBaseline, true, HorizontalLine));
    
    info.context->setStrokeColor(Color(255, 0, 0), ColorSpaceSRGB);
    
    info.context->drawLine(IntPoint(adjustedPaintOffset.x(), adjustedPaintOffset.y() + baseline), IntPoint(adjustedPaintOffset.x() + pixelSnappedOffsetWidth(), adjustedPaintOffset.y() + baseline));
}
#endif // ENABLE(DEBUG_MATH_LAYOUT)

//
// The MathML specification says:
// (http://www.w3.org/TR/MathML/chapter2.html#fund.units)
//
// "Most presentation elements have attributes that accept values representing
// lengths to be used for size, spacing or similar properties. The syntax of a
// length is specified as
//
// number | number unit | namedspace
//
// There should be no space between the number and the unit of a length."
// 
// "A trailing '%' represents a percent of the default value. The default
// value, or how it is obtained, is listed in the table of attributes for each
// element. [...] A number without a unit is intepreted as a multiple of the
// default value."
//
// "The possible units in MathML are:
//  
// Unit Description
// em   an em (font-relative unit traditionally used for horizontal lengths)
// ex   an ex (font-relative unit traditionally used for vertical lengths)
// px   pixels, or size of a pixel in the current display
// in   inches (1 inch = 2.54 centimeters)
// cm   centimeters
// mm   millimeters
// pt   points (1 point = 1/72 inch)
// pc   picas (1 pica = 12 points)
// %    percentage of default value"
//
// The numbers are defined that way:
// - unsigned-number: "a string of decimal digits with up to one decimal point
//   (U+002E), representing a non-negative terminating decimal number (a type of
//   rational number)"
// - number: "an optional prefix of '-' (U+002D), followed by an unsigned
//   number, representing a terminating decimal number (a type of rational
//   number)"
//
bool parseMathMLLength(const String& string, LayoutUnit& lengthValue, const RenderStyle* style, bool allowNegative)
{
    String s = string.simplifyWhiteSpace();

    int stringLength = s.length();
    if (!stringLength)
        return false;

    if (parseMathMLNamedSpace(s, lengthValue, style, allowNegative))
        return true;

    StringBuilder number;
    String unit;

    // This verifies whether the negative sign is there.
    int i = 0;
    UChar c = s[0];
    if (c == '-') {
        number.append(c);
        i++;
    }

    // This gathers up characters that make up the number.
    bool gotDot = false;
    for ( ; i < stringLength; i++) {
        c = s[i];
        // The string is invalid if it contains two dots.
        if (gotDot && c == '.')
            return false;
        if (c == '.')
            gotDot = true;
        else if (!isASCIIDigit(c)) {
            unit = s.substring(i, stringLength - i);
            // Some authors leave blanks before the unit, but that shouldn't
            // be allowed, so don't simplifyWhitespace on 'unit'.
            break;
        }
        number.append(c);
    }

    // Convert number to floating point
    bool ok;
    float floatValue = number.toString().toFloat(&ok);
    if (!ok)
        return false;
    if (floatValue < 0 && !allowNegative)
        return false;

    if (unit.isEmpty()) {
        // no explicit unit, this is a number that will act as a multiplier
        lengthValue *= floatValue;
        return true;
    }
    if (unit == "%") {
        lengthValue *= floatValue / 100;
        return true;
    }
    if (unit == "em") {
        lengthValue = floatValue * style->font().size();
        return true;
    }
    if (unit == "ex") {
        lengthValue = floatValue * style->fontMetrics().xHeight();
        return true;
    }
    if (unit == "px") {
        lengthValue = floatValue;
        return true;
    }
    if (unit == "pt") {
        lengthValue = 4 * (floatValue / 3);
        return true;
    }
    if (unit == "pc") {
        lengthValue = 16 * floatValue;
        return true;
    }
    if (unit == "in") {
        lengthValue = 96 * floatValue;
        return true;
    }
    if (unit == "cm") {
        lengthValue = 96 * (floatValue / 2.54);
        return true;
    }
    if (unit == "mm") {
        lengthValue = 96 * (floatValue / 25.4);
        return true;
    }

    // unexpected unit
    return false;
}

bool parseMathMLNamedSpace(const String& string, LayoutUnit& lengthValue, const RenderStyle* style, bool allowNegative)
{
    float length = 0;
    // See if it is one of the namedspaces (ranging -7/18em, -6/18, ... 7/18em)
    if (string == "veryverythinmathspace")
        length = 1;
    else if (string == "verythinmathspace")
        length = 2;
    else if (string == "thinmathspace")
        length = 3;
    else if (string == "mediummathspace")
        length = 4;
    else if (string == "thickmathspace")
        length = 5;
    else if (string == "verythickmathspace")
        length = 6;
    else if (string == "veryverythickmathspace")
        length = 7;
    else if (allowNegative) {
        if (string == "negativeveryverythinmathspace")
            length = -1;
        else if (string == "negativeverythinmathspace")
            length = -2;
        else if (string == "negativethinmathspace")
            length = -3;
        else if (string == "negativemediummathspace")
            length = -4;
        else if (string == "negativethickmathspace")
            length = -5;
        else if (string == "negativeverythickmathspace")
            length = -6;
        else if (string == "negativeveryverythickmathspace")
            length = -7;        
    }
    if (length) {
        lengthValue = length * style->font().size() / 18;
        return true;
    }
    return false;
}

int RenderMathMLTable::firstLineBoxBaseline() const
{
    // In legal MathML, we'll have a MathML parent. That RenderFlexibleBox parent will use our firstLineBoxBaseline() for baseline alignment, per
    // http://dev.w3.org/csswg/css3-flexbox/#flex-baselines. We want to vertically center an <mtable>, such as a matrix. Essentially the whole <mtable> element fits on a
    // single line, whose baseline gives this centering. This is different than RenderTable::firstLineBoxBaseline, which returns the baseline of the first row of a <table>.
    return (logicalHeight() + style()->fontMetrics().xHeight()) / 2;
}

}    

#endif
