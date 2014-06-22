/**
 * Copyright (C) 2007 Rob Buis <buis@kde.org>
 * Copyright (C) 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2007 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2009 Google, Inc.  All rights reserved.
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
 * Copyright (C) 2012 Zoltan Herczeg <zherczeg@webkit.org>.
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

#ifndef SVGRenderingContext_h
#define SVGRenderingContext_h

#if ENABLE(SVG)
#include "ImageBuffer.h"
#include "PaintInfo.h"

namespace WebCore {

class AffineTransform;
class RenderObject;
class FloatRect;
class RenderSVGResourceFilter;

// SVGRenderingContext 
class SVGRenderingContext {
public:
    enum NeedsGraphicsContextSave {
        SaveGraphicsContext,
        DontSaveGraphicsContext,
    };

    // Does not start rendering.
    SVGRenderingContext()
        : m_renderingFlags(0)
        , m_object(0)
        , m_paintInfo(0)
        , m_savedContext(0)
#if ENABLE(FILTERS)
        , m_filter(0)
#endif
    {
    }

    SVGRenderingContext(RenderObject* object, PaintInfo& paintinfo, NeedsGraphicsContextSave needsGraphicsContextSave = DontSaveGraphicsContext)
        : m_renderingFlags(0)
        , m_object(0)
        , m_paintInfo(0)
        , m_savedContext(0)
#if ENABLE(FILTERS)
        , m_filter(0)
#endif
    {
        prepareToRenderSVGContent(object, paintinfo, needsGraphicsContextSave);
    }

    // Automatically finishes context rendering.
    ~SVGRenderingContext();

    // Used by all SVG renderers who apply clip/filter/etc. resources to the renderer content.
    void prepareToRenderSVGContent(RenderObject*, PaintInfo&, NeedsGraphicsContextSave = DontSaveGraphicsContext);
    bool isRenderingPrepared() const { return m_renderingFlags & RenderingPrepared; }

    static bool createImageBuffer(const FloatRect& paintRect, const AffineTransform& absoluteTransform, OwnPtr<ImageBuffer>&, ColorSpace, RenderingMode);
    // Patterns need a different float-to-integer coordinate mapping.
    static bool createImageBufferForPattern(const FloatRect& absoluteTargetRect, const FloatRect& clampedAbsoluteTargetRect, OwnPtr<ImageBuffer>&, ColorSpace, RenderingMode);

    static void renderSubtreeToImageBuffer(ImageBuffer*, RenderObject*, const AffineTransform&);
    static void clipToImageBuffer(GraphicsContext*, const AffineTransform& absoluteTransform, const FloatRect& targetRect, OwnPtr<ImageBuffer>&, bool safeToClear);

    static float calculateScreenFontSizeScalingFactor(const RenderObject*);
    static void calculateTransformationToOutermostCoordinateSystem(const RenderObject*, AffineTransform& absoluteTransform);
    static IntSize clampedAbsoluteSize(const IntSize&);
    static FloatRect clampedAbsoluteTargetRect(const FloatRect& absoluteTargetRect);
    static void clear2DRotation(AffineTransform&);

    static IntRect calculateImageBufferRect(const FloatRect& targetRect, const AffineTransform& absoluteTransform)
    {
        return enclosingIntRect(absoluteTransform.mapRect(targetRect));
    }

    // Support for the buffered-rendering hint.
    bool bufferForeground(OwnPtr<ImageBuffer>&);

private:
    // To properly revert partially successful initializtions in the destructor, we record all successful steps.
    enum RenderingFlags {
        RenderingPrepared = 1,
        RestoreGraphicsContext = 1 << 1,
        EndOpacityLayer = 1 << 2,
        EndShadowLayer = 1 << 3,
        EndFilterLayer = 1 << 4,
        PrepareToRenderSVGContentWasCalled = 1 << 5
    };

    // List of those flags which require actions during the destructor.
    const static int ActionsNeeded = RestoreGraphicsContext | EndOpacityLayer | EndShadowLayer | EndFilterLayer;

    int m_renderingFlags;
    RenderObject* m_object;
    PaintInfo* m_paintInfo;
    GraphicsContext* m_savedContext;
    IntRect m_savedPaintRect;
#if ENABLE(FILTERS)
    RenderSVGResourceFilter* m_filter;
#endif
};

} // namespace WebCore

#endif // ENABLE(SVG)
#endif // SVGRenderingContext_h
