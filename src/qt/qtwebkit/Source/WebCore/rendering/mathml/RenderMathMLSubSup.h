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

#ifndef RenderMathMLSubSup_h
#define RenderMathMLSubSup_h

#if ENABLE(MATHML)

#include "RenderMathMLBlock.h"

namespace WebCore {
    
// Render a base with a subscript and/or a superscript.
class RenderMathMLSubSup : public RenderMathMLBlock {
public:
    RenderMathMLSubSup(Element*);
    virtual void addChild(RenderObject* child, RenderObject* beforeChild = 0);
    
    virtual RenderMathMLOperator* unembellishedOperator();

protected:
    virtual void layout();
    
private:
    virtual bool isRenderMathMLSubSup() const { return true; }
    void fixAnonymousStyles();

    virtual void styleDidChange(StyleDifference, const RenderStyle* oldStyle) OVERRIDE;

    virtual const char* renderName() const { return "RenderMathMLSubSup"; }

    // Omit our subscript and/or superscript. This may return 0 for a non-MathML base (which
    // won't occur in valid MathML).
    RenderBoxModelObject* base() const;
    
    enum SubSupType { Sub, Super, SubSup };
    SubSupType m_kind;
    RenderMathMLBlock* m_scripts;
};
    
}

#endif // ENABLE(MATHML)

#endif // RenderMathMLSubSup_h
