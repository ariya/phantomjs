/*
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef SVGTextFragment_h
#define SVGTextFragment_h

#if ENABLE(SVG)
#include "AffineTransform.h"

namespace WebCore {

// A SVGTextFragment describes a text fragment of a RenderSVGInlineText which can be rendered at once.
struct SVGTextFragment {
    SVGTextFragment()
        : characterOffset(0)
        , metricsListOffset(0)
        , length(0)
        , isTextOnPath(false)
        , x(0)
        , y(0)
        , width(0)
        , height(0)
    {
    }

    enum TransformType {
        TransformRespectingTextLength,
        TransformIgnoringTextLength
    };

    void buildFragmentTransform(AffineTransform& result, TransformType type = TransformRespectingTextLength) const
    {
        if (type == TransformIgnoringTextLength) {
            result = transform;
            transformAroundOrigin(result);
            return;
        }

        if (isTextOnPath)
            buildTransformForTextOnPath(result);
        else
            buildTransformForTextOnLine(result);
    }

    // The first rendered character starts at RenderSVGInlineText::characters() + characterOffset.
    unsigned characterOffset;
    unsigned metricsListOffset;
    unsigned length : 31;
    bool isTextOnPath : 1;

    float x;
    float y;
    float width;
    float height;

    // Includes rotation/glyph-orientation-(horizontal|vertical) transforms, as well as orientation related shifts
    // (see SVGTextLayoutEngine, which builds this transformation).
    AffineTransform transform;

    // Contains lengthAdjust related transformations, which are not allowd to influence the SVGTextQuery code.
    AffineTransform lengthAdjustTransform;

private:
    void transformAroundOrigin(AffineTransform& result) const
    {
        // Returns (translate(x, y) * result) * translate(-x, -y).
        result.setE(result.e() + x);
        result.setF(result.f() + y);
        result.translate(-x, -y);
    }

    void buildTransformForTextOnPath(AffineTransform& result) const
    {
        // For text-on-path layout, multiply the transform with the lengthAdjustTransform before orienting the resulting transform.
        result = lengthAdjustTransform.isIdentity() ? transform : transform * lengthAdjustTransform;
        if (!result.isIdentity())
            transformAroundOrigin(result);
    }

    void buildTransformForTextOnLine(AffineTransform& result) const
    {
        // For text-on-line layout, orient the transform first, then multiply the lengthAdjustTransform with the oriented transform.
        if (transform.isIdentity()) {
            result = lengthAdjustTransform;
            return;
        }

        result = transform;
        transformAroundOrigin(result);

        if (!lengthAdjustTransform.isIdentity())
            result = lengthAdjustTransform * result;
    }
};

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
