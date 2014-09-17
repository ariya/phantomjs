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

#ifndef RenderMathMLBlock_h
#define RenderMathMLBlock_h

#if ENABLE(MATHML)

#include "RenderBlock.h"

#define ENABLE_DEBUG_MATH_LAYOUT 0

namespace WebCore {
    
class RenderMathMLBlock : public RenderBlock {
public:
    RenderMathMLBlock(Node* container);
    virtual bool isChildAllowed(RenderObject*, RenderStyle*) const;
    
    virtual bool isRenderMathMLBlock() const { return true; }
    virtual bool isRenderMathMLOperator() const { return false; }
    virtual bool isRenderMathMLRow() const { return false; }
    virtual bool isRenderMathMLMath() const { return false; }
    virtual bool hasBase() const { return false; }
    virtual int nonOperatorHeight() const;
    virtual void stretchToHeight(int height);

#if ENABLE(DEBUG_MATH_LAYOUT)
    virtual void paint(PaintInfo&, int tx, int ty);
#endif
    
protected:
    int getBoxModelObjectHeight(RenderObject* object) 
    {
        if (object && object->isBoxModelObject()) {
            RenderBoxModelObject* box = toRenderBoxModelObject(object);
            return box->offsetHeight();
        }
        
        return 0;
    }
    int getBoxModelObjectHeight(const RenderObject* object) 
    {
        if (object && object->isBoxModelObject()) {
            const RenderBoxModelObject* box = toRenderBoxModelObject(object);
            return box->offsetHeight();
        }
        
        return 0;
    }
    int getBoxModelObjectWidth(RenderObject* object) 
    {
        if (object && object->isBoxModelObject()) {
            RenderBoxModelObject* box = toRenderBoxModelObject(object);
            return box->offsetWidth();
        }
        
        return 0;
    }
    int getBoxModelObjectWidth(const RenderObject* object) 
    {
        if (object && object->isBoxModelObject()) {
            const RenderBoxModelObject* box = toRenderBoxModelObject(object);
            return box->offsetWidth();
        }
        
        return 0;
    }
    virtual PassRefPtr<RenderStyle> makeBlockStyle();
    
};

inline RenderMathMLBlock* toRenderMathMLBlock(RenderObject* object)
{ 
    ASSERT(!object || object->isRenderMathMLBlock());
    return static_cast<RenderMathMLBlock*>(object);
}

inline const RenderMathMLBlock* toRenderMathMLBlock(const RenderObject* object)
{ 
    ASSERT(!object || object->isRenderMathMLBlock());
    return static_cast<const RenderMathMLBlock*>(object);
}

}


#endif // ENABLE(MATHML)
#endif // RenderMathMLBlock_h
