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

#ifndef RenderMathMLFraction_h
#define RenderMathMLFraction_h

#if ENABLE(MATHML)

#include "RenderMathMLBlock.h"

namespace WebCore {

class RenderMathMLFraction : public RenderMathMLBlock {
public:
    RenderMathMLFraction(Element*);
    virtual void addChild(RenderObject* child, RenderObject* beforeChild = 0);
    virtual void updateFromElement();
    
    virtual RenderMathMLOperator* unembellishedOperator();
    
    virtual int firstLineBoxBaseline() const OVERRIDE;
    float lineThickness() const { return m_lineThickness; }
    virtual void paint(PaintInfo&, const LayoutPoint&);
protected:
    virtual void layout();
    
private:
    virtual bool isRenderMathMLFraction() const { return true; }
    void fixChildStyle(RenderObject* child);
    virtual void styleDidChange(StyleDifference, const RenderStyle* oldStyle) OVERRIDE;

    virtual const char* renderName() const { return "RenderMathMLFraction"; }
    
    LayoutUnit m_lineThickness;
};
    
inline RenderMathMLFraction* toRenderMathMLFraction(RenderObject* object)
{
    ASSERT_WITH_SECURITY_IMPLICATION(!object || (object->isRenderMathMLBlock() && toRenderMathMLBlock(object)->isRenderMathMLFraction()));
    return static_cast<RenderMathMLFraction*>(object);
}

inline const RenderMathMLFraction* toRenderMathMLFraction(const RenderObject* object)
{
    ASSERT_WITH_SECURITY_IMPLICATION(!object || (object->isRenderMathMLBlock() && toRenderMathMLBlock(object)->isRenderMathMLFraction()));
    return static_cast<const RenderMathMLFraction*>(object);
}

}

#endif // ENABLE(MATHML)

#endif // RenderMathMLFraction_h
