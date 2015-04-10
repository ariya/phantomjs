/*
 * Copyright (C) 2013 The MathJax Consortium. All rights reserved.
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
#include "RenderMathMLSpace.h"

#if ENABLE(MATHML)

#include "GraphicsContext.h"
#include "MathMLNames.h"
#include "PaintInfo.h"

namespace WebCore {
    
using namespace MathMLNames;

RenderMathMLSpace::RenderMathMLSpace(Element* element)
    : RenderMathMLBlock(element)
    , m_width(0)
    , m_height(0)
    , m_depth(0)
{
}

void RenderMathMLSpace::computeIntrinsicLogicalWidths(LayoutUnit& minLogicalWidth, LayoutUnit& maxLogicalWidth) const
{
    minLogicalWidth = m_width;
    maxLogicalWidth = m_width;
}

void RenderMathMLSpace::updateFromElement()
{
    Element* space = toElement(node());

    // This parses the mspace attributes, using 0 as the default values.
    m_width = 0;
    m_height = 0;
    m_depth = 0;
    parseMathMLLength(space->getAttribute(MathMLNames::widthAttr), m_width, style());
    parseMathMLLength(space->getAttribute(MathMLNames::heightAttr), m_height, style());
    parseMathMLLength(space->getAttribute(MathMLNames::depthAttr), m_depth, style());

    // FIXME: Negative width values should be accepted.
    if (m_width < 0)
        m_width = 0;

    // If the total height is negative, set vertical dimensions to 0.
    if (m_height + m_depth < 0) {
        m_height = 0;
        m_depth = 0;
    }

    setNeedsLayoutAndPrefWidthsRecalc();
}

void RenderMathMLSpace::updateLogicalWidth()
{
    setLogicalWidth(m_width);
}

void RenderMathMLSpace::updateLogicalHeight()
{
    setLogicalHeight(m_height + m_depth);
}

void RenderMathMLSpace::styleDidChange(StyleDifference diff, const RenderStyle* oldStyle)
{
    RenderMathMLBlock::styleDidChange(diff, oldStyle);
    updateFromElement();
}

int RenderMathMLSpace::firstLineBoxBaseline() const
{
    return m_height;
}

}

#endif
