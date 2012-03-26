/*
 * Copyright (c) 2009, Google Inc. All rights reserved.
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

#ifndef RenderSVGModelObject_h
#define RenderSVGModelObject_h

#if ENABLE(SVG)

#include "RenderObject.h"
#include "SVGRenderSupport.h"

namespace WebCore {

// Most renderers in the SVG rendering tree will inherit from this class
// but not all. (e.g. RenderSVGForeignObject, RenderSVGBlock) thus methods
// required by SVG renders need to be declared on RenderObject, but shared
// logic can go in this class or in SVGRenderSupport.

class SVGStyledElement;

class RenderSVGModelObject : public RenderObject {
public:
    explicit RenderSVGModelObject(SVGStyledElement*);

    virtual bool requiresLayer() const { return false; }

    virtual IntRect clippedOverflowRectForRepaint(RenderBoxModelObject* repaintContainer);
    virtual void computeRectForRepaint(RenderBoxModelObject* repaintContainer, IntRect&, bool fixed = false);
    virtual IntRect outlineBoundsForRepaint(RenderBoxModelObject* repaintContainer, IntPoint*) const;

    virtual void absoluteRects(Vector<IntRect>&, int tx, int ty);
    virtual void absoluteQuads(Vector<FloatQuad>&);

    virtual void destroy();

    virtual void mapLocalToContainer(RenderBoxModelObject* repaintContainer, bool useTransforms, bool fixed, TransformState&) const;
    virtual void styleWillChange(StyleDifference, const RenderStyle* newStyle);
    virtual void styleDidChange(StyleDifference, const RenderStyle* oldStyle);
    virtual void updateFromElement();

private:
    // This method should never be called, SVG uses a different nodeAtPoint method
    bool nodeAtPoint(const HitTestRequest&, HitTestResult&, int xInContainer, int yInContainer, int dxParentToContainer, int dyParentToContainer, HitTestAction);
};

}

#endif // ENABLE(SVG)
#endif
