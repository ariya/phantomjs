/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef RenderScrollbarPart_h
#define RenderScrollbarPart_h

#include "RenderBlock.h"
#include "ScrollTypes.h"

namespace WebCore {

class RenderScrollbar;

class RenderScrollbarPart : public RenderBlock {
public:
    static RenderScrollbarPart* createAnonymous(Document*, RenderScrollbar* = 0, ScrollbarPart = NoPart);
    
    virtual ~RenderScrollbarPart();

    virtual const char* renderName() const { return "RenderScrollbarPart"; }
    
    virtual bool requiresLayer() const { return false; }

    virtual void layout();
    
    void paintIntoRect(GraphicsContext*, const LayoutPoint&, const LayoutRect&);

    // Scrollbar parts needs to be rendered at device pixel boundaries.
    virtual LayoutUnit marginTop() const OVERRIDE { ASSERT(isIntegerValue(m_marginBox.top())); return m_marginBox.top(); }
    virtual LayoutUnit marginBottom() const OVERRIDE { ASSERT(isIntegerValue(m_marginBox.bottom())); return m_marginBox.bottom(); }
    virtual LayoutUnit marginLeft() const OVERRIDE { ASSERT(isIntegerValue(m_marginBox.left())); return m_marginBox.left(); }
    virtual LayoutUnit marginRight() const OVERRIDE { ASSERT(isIntegerValue(m_marginBox.right())); return m_marginBox.right(); }

    virtual bool isRenderScrollbarPart() const { return true; }
    RenderObject* rendererOwningScrollbar() const;

protected:
    virtual void styleWillChange(StyleDifference diff, const RenderStyle* newStyle);
    virtual void styleDidChange(StyleDifference, const RenderStyle* oldStyle);
    virtual void imageChanged(WrappedImagePtr, const IntRect* = 0);

private:
    RenderScrollbarPart(RenderScrollbar*, ScrollbarPart);

    virtual void computePreferredLogicalWidths();

    void layoutHorizontalPart();
    void layoutVerticalPart();

    void computeScrollbarWidth();
    void computeScrollbarHeight();
    
    RenderScrollbar* m_scrollbar;
    ScrollbarPart m_part;
};

inline RenderScrollbarPart* toRenderScrollbarPart(RenderObject* object)
{
    ASSERT_WITH_SECURITY_IMPLICATION(!object || object->isRenderScrollbarPart());
    return static_cast<RenderScrollbarPart*>(object);
}

inline const RenderScrollbarPart* toRenderScrollbarPart(const RenderObject* object)
{
    ASSERT_WITH_SECURITY_IMPLICATION(!object || object->isRenderScrollbarPart());
    return static_cast<const RenderScrollbarPart*>(object);
}

// This will catch anyone doing an unnecessary cast.
void toRenderScrollbarPart(const RenderScrollbarPart*);

} // namespace WebCore

#endif // RenderScrollbarPart_h
