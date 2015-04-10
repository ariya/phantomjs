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

#include "config.h"

#if ENABLE(SVG)
#include "RenderSVGModelObject.h"

#include "RenderLayerModelObject.h"
#include "RenderSVGResource.h"
#include "SVGNames.h"
#include "SVGResourcesCache.h"
#include "SVGStyledElement.h"

namespace WebCore {

RenderSVGModelObject::RenderSVGModelObject(SVGStyledElement* node)
    : RenderObject(node)
    , m_hasSVGShadow(false)
{
}

LayoutRect RenderSVGModelObject::clippedOverflowRectForRepaint(const RenderLayerModelObject* repaintContainer) const
{
    return SVGRenderSupport::clippedOverflowRectForRepaint(this, repaintContainer);
}

void RenderSVGModelObject::computeFloatRectForRepaint(const RenderLayerModelObject* repaintContainer, FloatRect& repaintRect, bool fixed) const
{
    SVGRenderSupport::computeFloatRectForRepaint(this, repaintContainer, repaintRect, fixed);
}

void RenderSVGModelObject::mapLocalToContainer(const RenderLayerModelObject* repaintContainer, TransformState& transformState, MapCoordinatesFlags, bool* wasFixed) const
{
    SVGRenderSupport::mapLocalToContainer(this, repaintContainer, transformState, wasFixed);
}

const RenderObject* RenderSVGModelObject::pushMappingToContainer(const RenderLayerModelObject* ancestorToStopAt, RenderGeometryMap& geometryMap) const
{
    return SVGRenderSupport::pushMappingToContainer(this, ancestorToStopAt, geometryMap);
}

// Copied from RenderBox, this method likely requires further refactoring to work easily for both SVG and CSS Box Model content.
// FIXME: This may also need to move into SVGRenderSupport as the RenderBox version depends
// on borderBoundingBox() which SVG RenderBox subclases (like SVGRenderBlock) do not implement.
LayoutRect RenderSVGModelObject::outlineBoundsForRepaint(const RenderLayerModelObject* repaintContainer, const RenderGeometryMap*) const
{
    LayoutRect box = enclosingLayoutRect(repaintRectInLocalCoordinates());
    adjustRectForOutlineAndShadow(box);

    FloatQuad containerRelativeQuad = localToContainerQuad(FloatRect(box), repaintContainer);
    return containerRelativeQuad.enclosingBoundingBox();
}

void RenderSVGModelObject::absoluteRects(Vector<IntRect>& rects, const LayoutPoint& accumulatedOffset) const
{
    IntRect rect = enclosingIntRect(strokeBoundingBox());
    rect.moveBy(roundedIntPoint(accumulatedOffset));
    rects.append(rect);
}

void RenderSVGModelObject::absoluteQuads(Vector<FloatQuad>& quads, bool* wasFixed) const
{
    quads.append(localToAbsoluteQuad(strokeBoundingBox(), 0 /* mode */, wasFixed));
}

void RenderSVGModelObject::willBeDestroyed()
{
    SVGResourcesCache::clientDestroyed(this);
    RenderObject::willBeDestroyed();
}

void RenderSVGModelObject::styleWillChange(StyleDifference diff, const RenderStyle* newStyle)
{
    if (diff == StyleDifferenceLayout) {
        setNeedsBoundariesUpdate();
        if (newStyle->hasTransform())
            setNeedsTransformUpdate();
    }
    RenderObject::styleWillChange(diff, newStyle);
}

void RenderSVGModelObject::styleDidChange(StyleDifference diff, const RenderStyle* oldStyle)
{
    RenderObject::styleDidChange(diff, oldStyle);
    SVGResourcesCache::clientStyleChanged(this, diff, style());
}

bool RenderSVGModelObject::nodeAtPoint(const HitTestRequest&, HitTestResult&, const HitTestLocation&, const LayoutPoint&, HitTestAction)
{
    ASSERT_NOT_REACHED();
    return false;
}

static void getElementCTM(SVGElement* element, AffineTransform& transform)
{
    ASSERT(element);
    element->document()->updateLayoutIgnorePendingStylesheets();

    SVGElement* stopAtElement = SVGLocatable::nearestViewportElement(element);
    ASSERT(stopAtElement);

    AffineTransform localTransform;
    Node* current = element;

    while (current && current->isSVGElement()) {
        SVGElement* currentElement = toSVGElement(current);
        if (currentElement->isSVGStyledElement()) {
            localTransform = currentElement->renderer()->localToParentTransform();
            transform = localTransform.multiply(transform);
        }
        // For getCTM() computation, stop at the nearest viewport element
        if (currentElement == stopAtElement)
            break;

        current = current->parentOrShadowHostNode();
    }
}

// FloatRect::intersects does not consider horizontal or vertical lines (because of isEmpty()).
// So special-case handling of such lines.
static bool intersectsAllowingEmpty(const FloatRect& r, const FloatRect& other)
{
    if (r.isEmpty() && other.isEmpty())
        return false;
    if (r.isEmpty() && !other.isEmpty()) {
        return (other.contains(r.x(), r.y()) && !other.contains(r.maxX(), r.maxY()))
               || (!other.contains(r.x(), r.y()) && other.contains(r.maxX(), r.maxY()));
    }
    if (other.isEmpty() && !r.isEmpty())
        return intersectsAllowingEmpty(other, r);
    return r.intersects(other);
}

// One of the element types that can cause graphics to be drawn onto the target canvas. Specifically: circle, ellipse,
// image, line, path, polygon, polyline, rect, text and use.
static bool isGraphicsElement(RenderObject* renderer)
{
    return renderer->isSVGShape() || renderer->isSVGText() || renderer->isSVGImage() || renderer->node()->hasTagName(SVGNames::useTag);
}

// The SVG addFocusRingRects() method adds rects in local coordinates so the default absoluteFocusRingQuads
// returns incorrect values for SVG objects. Overriding this method provides access to the absolute bounds.
void RenderSVGModelObject::absoluteFocusRingQuads(Vector<FloatQuad>& quads)
{
    quads.append(localToAbsoluteQuad(FloatQuad(repaintRectInLocalCoordinates())));
}
    
bool RenderSVGModelObject::checkIntersection(RenderObject* renderer, const FloatRect& rect)
{
    if (!renderer || renderer->style()->pointerEvents() == PE_NONE)
        return false;
    if (!isGraphicsElement(renderer))
        return false;
    AffineTransform ctm;
    SVGElement* svgElement = toSVGElement(renderer->node());
    getElementCTM(svgElement, ctm);
    ASSERT(svgElement->renderer());
    return intersectsAllowingEmpty(rect, ctm.mapRect(svgElement->renderer()->repaintRectInLocalCoordinates()));
}

bool RenderSVGModelObject::checkEnclosure(RenderObject* renderer, const FloatRect& rect)
{
    if (!renderer || renderer->style()->pointerEvents() == PE_NONE)
        return false;
    if (!isGraphicsElement(renderer))
        return false;
    AffineTransform ctm;
    SVGElement* svgElement = toSVGElement(renderer->node());
    getElementCTM(svgElement, ctm);
    ASSERT(svgElement->renderer());
    return rect.contains(ctm.mapRect(svgElement->renderer()->repaintRectInLocalCoordinates()));
}

} // namespace WebCore

#endif // ENABLE(SVG)
