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

#ifndef RenderMathMLSpace_h
#define RenderMathMLSpace_h

#if ENABLE(MATHML)

#include "RenderMathMLBlock.h"

namespace WebCore {
    
class RenderMathMLSpace : public RenderMathMLBlock {
public:
    explicit RenderMathMLSpace(Element*);

    virtual int firstLineBoxBaseline() const OVERRIDE;
    virtual void updateLogicalWidth() OVERRIDE;
    virtual void updateLogicalHeight() OVERRIDE;

private:
    virtual void styleDidChange(StyleDifference, const RenderStyle* oldStyle) OVERRIDE;
    virtual const char* renderName() const OVERRIDE { return isAnonymous() ? "RenderMathMLSpace (anonymous)" : "RenderMathMLSpace"; }

    virtual bool isRenderMathMLSpace() const OVERRIDE { return true; }

    virtual bool isChildAllowed(RenderObject*, RenderStyle*) const OVERRIDE { return false; } 
    virtual void computeIntrinsicLogicalWidths(LayoutUnit& minLogicalWidth, LayoutUnit& maxLogicalWidth) const OVERRIDE;

    virtual void updateFromElement() OVERRIDE;

    LayoutUnit m_width;
    LayoutUnit m_height;
    LayoutUnit m_depth;
};

inline RenderMathMLSpace* toRenderMathMLSpace(RenderMathMLBlock* block)
{ 
    ASSERT_WITH_SECURITY_IMPLICATION(!block || block->isRenderMathMLSpace());
    return static_cast<RenderMathMLSpace*>(block);
}

inline const RenderMathMLSpace* toRenderMathMLSpace(const RenderMathMLBlock* block)
{ 
    ASSERT_WITH_SECURITY_IMPLICATION(!block || block->isRenderMathMLSpace());
    return static_cast<const RenderMathMLSpace*>(block);
}

// This will catch anyone doing an unnecessary cast.
void toRenderMathMLSpace(const RenderMathMLSpace*);
}

#endif // ENABLE(MATHML)
#endif // RenderMathMLSpace_h
