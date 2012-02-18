/**
 * Copyright (C) 2007 Rob Buis <buis@kde.org>
 * Copyright (C) 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2007 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2009 Google, Inc.  All rights reserved.
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

#ifndef SVGRenderSupport_h
#define SVGRenderSupport_h

#if ENABLE(SVG)
#include "PaintInfo.h"

namespace WebCore {

class FloatPoint;
class FloatRect;
class ImageBuffer;
class RenderBoxModelObject;
class RenderObject;
class RenderStyle;
class RenderSVGRoot;
class TransformState;

// SVGRendererSupport is a helper class sharing code between all SVG renderers.
class SVGRenderSupport {
public:
    // Used by all SVG renderers who apply clip/filter/etc. resources to the renderer content
    static bool prepareToRenderSVGContent(RenderObject*, PaintInfo&);
    static void finishRenderSVGContent(RenderObject*, PaintInfo&, GraphicsContext* savedContext);

    // Shares child layouting code between RenderSVGRoot/RenderSVG(Hidden)Container
    static void layoutChildren(RenderObject*, bool selfNeedsLayout);

    // Helper function determining wheter overflow is hidden
    static bool isOverflowHidden(const RenderObject*);

    // Calculates the repaintRect in combination with filter, clipper and masker in local coordinates.
    static void intersectRepaintRectWithResources(const RenderObject*, FloatRect&);

    // Determines whether the passed point lies in a clipping area
    static bool pointInClippingArea(RenderObject*, const FloatPoint&);

    static void computeContainerBoundingBoxes(const RenderObject* container, FloatRect& objectBoundingBox, FloatRect& strokeBoundingBox, FloatRect& repaintBoundingBox);
    static bool paintInfoIntersectsRepaintRect(const FloatRect& localRepaintRect, const AffineTransform& localTransform, const PaintInfo&);

    // Important functions used by nearly all SVG renderers centralizing coordinate transformations / repaint rect calculations
    static IntRect clippedOverflowRectForRepaint(RenderObject*, RenderBoxModelObject* repaintContainer);
    static void computeRectForRepaint(RenderObject*, RenderBoxModelObject* repaintContainer, IntRect&, bool fixed);
    static void mapLocalToContainer(const RenderObject*, RenderBoxModelObject* repaintContainer, bool useTransforms, bool fixed, TransformState&);

    // Shared between SVG renderers and resources.
    static void applyStrokeStyleToContext(GraphicsContext*, const RenderStyle*, const RenderObject*);

    // FIXME: These methods do not belong here.
    static const RenderSVGRoot* findTreeRootObject(const RenderObject*);

private:
    // This class is not constructable.
    SVGRenderSupport();
    ~SVGRenderSupport();
};

} // namespace WebCore

#endif // ENABLE(SVG)
#endif // SVGRenderSupport_h
