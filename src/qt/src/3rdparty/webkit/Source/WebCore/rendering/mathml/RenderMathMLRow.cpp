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

RenderMathMLRow::RenderMathMLRow(Node* row)
    : RenderMathMLBlock(row)
{
}

int RenderMathMLRow::nonOperatorHeight() const
{
    int maxHeight = 0;
    for (RenderObject* current = firstChild(); current; current = current->nextSibling()) {
        if (current->isRenderMathMLBlock()) {
            RenderMathMLBlock* block = toRenderMathMLBlock(current);
            int blockHeight = block->nonOperatorHeight();
            // Check to see if this box has a larger height
            if (blockHeight > maxHeight)
                maxHeight = blockHeight;
        } else if (current->isBoxModelObject()) {
            RenderBoxModelObject* box = toRenderBoxModelObject(current);
            // Check to see if this box has a larger height
            if (box->offsetHeight() > maxHeight)
                maxHeight = box->offsetHeight();
        }
        
    }
    return maxHeight;
}

void RenderMathMLRow::layout() 
{
    RenderBlock::layout();
    
    int maxHeight = 0;
    int childCount = 0;
    int operatorCount = 0;

    // Calculate the non-operator max height of the row.
    int operatorHeight = 0;
    for (RenderObject* current = firstChild(); current; current = current->nextSibling()) {
        childCount++;
        if (current->isRenderMathMLBlock()) {
            RenderMathMLBlock* block = toRenderMathMLBlock(current);
            // Check to see if the non-operator block has a greater height.
            if (!block->hasBase() && !block->isRenderMathMLOperator() && block->offsetHeight() > maxHeight)
                maxHeight = block->offsetHeight();
            if (block->hasBase() && block->nonOperatorHeight() > maxHeight) 
                maxHeight = block->nonOperatorHeight();
            // If the block is an operator, capture the maximum height and increment the count.
            if (block->isRenderMathMLOperator()) {
                if (block->offsetHeight() > operatorHeight)
                    operatorHeight = block->offsetHeight();
                operatorCount++;
            }
        } else if (current->isBoxModelObject()) {
            RenderBoxModelObject* box = toRenderBoxModelObject(current);
            // Check to see if this box has a larger height.
            if (box->offsetHeight() > maxHeight)
                maxHeight = box->offsetHeight();
        }
    }
    
    if (childCount > 0 && childCount == operatorCount) {
        // We have only operators and so set the max height to the operator height.
        maxHeight = operatorHeight;
    }
    
    // Stretch everything to the same height (blocks can ignore the request).
    if (maxHeight > 0) {
        bool didStretch = false;
        for (RenderObject* current = firstChild(); current; current = current->nextSibling()) {
            if (current->isRenderMathMLBlock()) {
                RenderMathMLBlock* block = toRenderMathMLBlock(current);
                block->stretchToHeight(maxHeight);
                didStretch = true;
            }
        }
        if (didStretch) {
            setNeedsLayout(true);
            setPreferredLogicalWidthsDirty(true, false);
            RenderBlock::layout();
        }
    }
    
}    

int RenderMathMLRow::baselinePosition(FontBaseline, bool firstLine, LineDirectionMode direction, LinePositionMode linePositionMode) const
{
    if (firstChild() && firstChild()->isRenderMathMLBlock()) {
        RenderMathMLBlock* block = toRenderMathMLBlock(firstChild());
        if (block->isRenderMathMLOperator())
            return block->y() + block->baselinePosition(AlphabeticBaseline, firstLine, direction, linePositionMode);
    }
    
    return RenderBlock::baselinePosition(AlphabeticBaseline, firstLine, direction, linePositionMode);
}
    
}

#endif // ENABLE(MATHML)

