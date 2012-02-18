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

#ifndef RenderMathMLOperator_h
#define RenderMathMLOperator_h

#if ENABLE(MATHML)

#include "RenderMathMLBlock.h"
#include <wtf/unicode/CharacterNames.h>

namespace WebCore {
    
class RenderMathMLOperator : public RenderMathMLBlock {
public:
    RenderMathMLOperator(Node* container);
    RenderMathMLOperator(Node* container, UChar operatorChar);
    virtual bool isRenderMathMLOperator() const { return true; }
    virtual void stretchToHeight(int pixelHeight);
    virtual void updateFromElement(); 
    virtual bool isChildAllowed(RenderObject*, RenderStyle*) const;
    virtual int baselinePosition(FontBaseline, bool firstLine, LineDirectionMode, LinePositionMode = PositionOnContainingLine) const;
        
protected:
    virtual void layout();
    virtual RefPtr<RenderStyle> createStackableStyle(int size, int topRelative);
    virtual RenderBlock* createGlyph(UChar glyph, int size = 0, int charRelative = 0, int topRelative = 0);
    
private:
    int m_stretchHeight;
    bool m_isStacked;
    UChar m_operator;
};

inline RenderMathMLOperator* toRenderMathMLOperator(RenderMathMLBlock* block)
{ 
    ASSERT(!block || block->isRenderMathMLOperator());
    return static_cast<RenderMathMLOperator*>(block);
}

inline const RenderMathMLOperator* toRenderMathMLOperator(const RenderMathMLBlock* block)
{ 
    ASSERT(!block || block->isRenderMathMLOperator());
    return static_cast<const RenderMathMLOperator*>(block);
}

inline UChar convertHyphenMinusToMinusSign(UChar glyph)
{
    // When rendered as a mathematical operator, minus glyph should be larger.
    if (glyph == hyphenMinus)
        return minusSign;
    
    return glyph;
}

}

#endif // ENABLE(MATHML)
#endif // RenderMathMLOperator_h
