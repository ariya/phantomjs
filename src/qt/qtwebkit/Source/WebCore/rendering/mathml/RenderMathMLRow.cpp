/*
 * Copyright (C) 2010 Alex Milowski (alex@milowski.com). All rights reserved.
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

#include "RenderMathMLRow.h"

#include "MathMLNames.h"
#include "RenderMathMLOperator.h"

namespace WebCore {

using namespace MathMLNames;

RenderMathMLRow::RenderMathMLRow(Element* element)
    : RenderMathMLBlock(element)
{
}

// FIXME: Change all these createAnonymous... routines to return a PassOwnPtr<>.
RenderMathMLRow* RenderMathMLRow::createAnonymousWithParentRenderer(const RenderObject* parent)
{
    RefPtr<RenderStyle> newStyle = RenderStyle::createAnonymousStyleWithDisplay(parent->style(), FLEX);
    RenderMathMLRow* newMRow = new (parent->renderArena()) RenderMathMLRow(0);
    newMRow->setDocumentForAnonymous(parent->document());
    newMRow->setStyle(newStyle.release());
    return newMRow;
}

void RenderMathMLRow::computePreferredLogicalWidths()
{
    ASSERT(preferredLogicalWidthsDirty() && needsLayout());

#ifndef NDEBUG
    // FIXME: Remove this once mathml stops modifying the render tree here.
    SetLayoutNeededForbiddenScope layoutForbiddenScope(this, false);
#endif

    computeChildrenPreferredLogicalHeights();
    int stretchLogicalHeight = 0;
    for (RenderObject* child = firstChild(); child; child = child->nextSibling()) {
        if (child->isRenderMathMLBlock()) {
            RenderMathMLOperator* renderMo = toRenderMathMLBlock(child)->unembellishedOperator();
            // FIXME: Only skip renderMo if it is stretchy.
            if (renderMo)
                continue;
        }
        stretchLogicalHeight = max<int>(stretchLogicalHeight, roundToInt(preferredLogicalHeightAfterSizing(child)));
    }
    if (!stretchLogicalHeight)
        stretchLogicalHeight = style()->fontSize();
    
    // Set the sizes of (possibly embellished) stretchy operator children.
    for (RenderObject* child = firstChild(); child; child = child->nextSibling()) {
        if (child->isRenderMathMLBlock()) {
            RenderMathMLOperator* renderMo = toRenderMathMLBlock(child)->unembellishedOperator();
            if (renderMo)
                renderMo->stretchToHeight(stretchLogicalHeight);
        }
    }

    RenderMathMLBlock::computePreferredLogicalWidths();
    
    // Shrink our logical width to its probable value now without triggering unnecessary relayout of our children.
    ASSERT(needsLayout() && logicalWidth() >= maxPreferredLogicalWidth());
    setLogicalWidth(maxPreferredLogicalWidth());
}

void RenderMathMLRow::layout()
{
    // Our computePreferredLogicalWidths() may change our logical width and then layout our children, which
    // RenderBlock::layout()'s relayoutChildren logic isn't expecting.
    if (preferredLogicalWidthsDirty())
        computePreferredLogicalWidths();
    
    RenderMathMLBlock::layout();
}

}

#endif // ENABLE(MATHML)
