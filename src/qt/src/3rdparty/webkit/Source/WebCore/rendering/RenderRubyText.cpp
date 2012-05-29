/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
 * Copyright (C) 2011 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
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

#include "RenderRubyText.h"

using namespace std;

namespace WebCore {

RenderRubyText::RenderRubyText(Node* node)
    : RenderBlock(node)
{
}

RenderRubyText::~RenderRubyText()
{
}

bool RenderRubyText::isChildAllowed(RenderObject* child, RenderStyle*) const
{
    return child->isInline();
}

ETextAlign RenderRubyText::textAlignmentForLine(bool endsWithSoftBreak) const
{
    ETextAlign textAlign = style()->textAlign();
    if (textAlign != TAAUTO)
        return RenderBlock::textAlignmentForLine(endsWithSoftBreak);

    // The default behavior is to allow ruby text to expand if it is shorter than the ruby base.
    return JUSTIFY;
}

void RenderRubyText::adjustInlineDirectionLineBounds(int expansionOpportunityCount, float& logicalLeft, float& logicalWidth) const
{
    ETextAlign textAlign = style()->textAlign();
    if (textAlign != TAAUTO)
        return RenderBlock::adjustInlineDirectionLineBounds(expansionOpportunityCount, logicalLeft, logicalWidth);

    int maxPreferredLogicalWidth = this->maxPreferredLogicalWidth();
    if (maxPreferredLogicalWidth >= logicalWidth)
        return;

    // Inset the ruby text by half the inter-ideograph expansion amount, but no more than a full-width
    // ruby character on each side.
    float inset = (logicalWidth - maxPreferredLogicalWidth) / (expansionOpportunityCount + 1);
    if (expansionOpportunityCount)
        inset = min<float>(2 * style()->fontSize(), inset);

    logicalLeft += inset / 2;
    logicalWidth -= inset;
}

} // namespace WebCore
