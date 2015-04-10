/*
 * Copyright (C) 2010 University of Szeged
 * Copyright (C) 2010 Zoltan Herczeg
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
 * THIS SOFTWARE IS PROVIDED BY UNIVERSITY OF SZEGED ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL UNIVERSITY OF SZEGED OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef RenderSVGResourceFilterPrimitive_h
#define RenderSVGResourceFilterPrimitive_h

#if ENABLE(SVG) && ENABLE(FILTERS)

#include "RenderSVGHiddenContainer.h"
#include "RenderSVGResourceFilter.h"

namespace WebCore {

class FilterEffect;

class RenderSVGResourceFilterPrimitive : public RenderSVGHiddenContainer {
public:
    explicit RenderSVGResourceFilterPrimitive(SVGStyledElement* filterPrimitiveElement)
        : RenderSVGHiddenContainer(filterPrimitiveElement)
    {
    }

    virtual void styleDidChange(StyleDifference, const RenderStyle*);

    virtual const char* renderName() const { return "RenderSVGResourceFilterPrimitive"; }
    virtual bool isSVGResourceFilterPrimitive() const { return true; }

    // They depend on the RenderObject argument of RenderSVGResourceFilter::applyResource.
    static FloatRect determineFilterPrimitiveSubregion(FilterEffect*);

    inline void primitiveAttributeChanged(const QualifiedName& attribute)
    {
        RenderObject* filter = parent();
        if (!filter || !filter->isSVGResourceFilter())
            return;
        static_cast<RenderSVGResourceFilter*>(filter)->primitiveAttributeChanged(this, attribute);
    }
};

} // namespace WebCore

#endif // ENABLE(SVG) && ENABLE(FILTERS)

#endif // RenderSVGResourceFilterPrimitive_h
