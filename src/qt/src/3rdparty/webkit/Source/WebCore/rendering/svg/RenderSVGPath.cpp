/*
 * Copyright (C) 2004, 2005, 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2008 Rob Buis <buis@kde.org>
 * Copyright (C) 2005, 2007 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2009 Google, Inc.
 * Copyright (C) 2009 Dirk Schulze <krit@webkit.org>
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
 * Copyright (C) 2009 Jeff Schiller <codedread@gmail.com>
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

#include "config.h"

#if ENABLE(SVG)
#include "RenderSVGPath.h"

#include "FloatPoint.h"
#include "FloatQuad.h"
#include "GraphicsContext.h"
#include "HitTestRequest.h"
#include "PointerEventsHitRules.h"
#include "RenderSVGContainer.h"
#include "RenderSVGResourceMarker.h"
#include "RenderSVGResourceSolidColor.h"
#include "SVGRenderSupport.h"
#include "SVGResources.h"
#include "SVGStyledTransformableElement.h"
#include "SVGTransformList.h"
#include "SVGURIReference.h"
#include "StrokeStyleApplier.h"
#include <wtf/MathExtras.h>

namespace WebCore {

class BoundingRectStrokeStyleApplier : public StrokeStyleApplier {
public:
    BoundingRectStrokeStyleApplier(const RenderObject* object, RenderStyle* style)
        : m_object(object)
        , m_style(style)
    {
        ASSERT(style);
        ASSERT(object);
    }

    void strokeStyle(GraphicsContext* gc)
    {
        SVGRenderSupport::applyStrokeStyleToContext(gc, m_style, m_object);
    }

private:
    const RenderObject* m_object;
    RenderStyle* m_style;
};

RenderSVGPath::RenderSVGPath(SVGStyledTransformableElement* node)
    : RenderSVGModelObject(node)
    , m_needsBoundariesUpdate(false) // default is false, the cached rects are empty from the beginning
    , m_needsPathUpdate(true) // default is true, so we grab a Path object once from SVGStyledTransformableElement
    , m_needsTransformUpdate(true) // default is true, so we grab a AffineTransform object once from SVGStyledTransformableElement
{
}

RenderSVGPath::~RenderSVGPath()
{
}

bool RenderSVGPath::fillContains(const FloatPoint& point, bool requiresFill, WindRule fillRule)
{
    if (!m_fillBoundingBox.contains(point))
        return false;

    Color fallbackColor;
    if (requiresFill && !RenderSVGResource::fillPaintingResource(this, style(), fallbackColor))
        return false;

    return m_path.contains(point, fillRule);
}

bool RenderSVGPath::strokeContains(const FloatPoint& point, bool requiresStroke)
{
    if (!m_strokeAndMarkerBoundingBox.contains(point))
        return false;

    Color fallbackColor;
    if (requiresStroke && !RenderSVGResource::strokePaintingResource(this, style(), fallbackColor))
        return false;

    BoundingRectStrokeStyleApplier strokeStyle(this, style());
    return m_path.strokeContains(&strokeStyle, point);
}

void RenderSVGPath::layout()
{
    LayoutRepainter repainter(*this, checkForRepaintDuringLayout() && selfNeedsLayout());
    SVGStyledTransformableElement* element = static_cast<SVGStyledTransformableElement*>(node());

    bool updateCachedBoundariesInParents = false;

    bool needsPathUpdate = m_needsPathUpdate;
    if (needsPathUpdate) {
        m_path.clear();
        element->toPathData(m_path);
        m_needsPathUpdate = false;
        updateCachedBoundariesInParents = true;
    }

    if (m_needsTransformUpdate) {
        m_localTransform = element->animatedLocalTransform();
        m_needsTransformUpdate = false;
        updateCachedBoundariesInParents = true;
    }

    if (m_needsBoundariesUpdate)
        updateCachedBoundariesInParents = true;

    // Invalidate all resources of this client if our layout changed.
    if (m_everHadLayout && selfNeedsLayout()) {
        SVGResourcesCache::clientLayoutChanged(this);
        m_markerLayoutInfo.clear();
    }

    // At this point LayoutRepainter already grabbed the old bounds,
    // recalculate them now so repaintAfterLayout() uses the new bounds.
    if (needsPathUpdate || m_needsBoundariesUpdate) {
        updateCachedBoundaries();
        m_needsBoundariesUpdate = false;
    }

    // If our bounds changed, notify the parents.
    if (updateCachedBoundariesInParents)
        RenderSVGModelObject::setNeedsBoundariesUpdate();

    repainter.repaintAfterLayout();
    setNeedsLayout(false);
}

void RenderSVGPath::fillAndStrokePath(GraphicsContext* context)
{
    RenderStyle* style = this->style();

    Color fallbackColor;
    if (RenderSVGResource* fillPaintingResource = RenderSVGResource::fillPaintingResource(this, style, fallbackColor)) {
        if (fillPaintingResource->applyResource(this, style, context, ApplyToFillMode))
            fillPaintingResource->postApplyResource(this, context, ApplyToFillMode, &m_path);
        else if (fallbackColor.isValid()) {
            RenderSVGResourceSolidColor* fallbackResource = RenderSVGResource::sharedSolidPaintingResource();
            fallbackResource->setColor(fallbackColor);
            if (fallbackResource->applyResource(this, style, context, ApplyToFillMode))
                fallbackResource->postApplyResource(this, context, ApplyToFillMode, &m_path);
        }
    }

    fallbackColor = Color();
    RenderSVGResource* strokePaintingResource = RenderSVGResource::strokePaintingResource(this, style, fallbackColor);
    if (!strokePaintingResource)
        return;

    Path path;

    bool nonScalingStroke = style->svgStyle()->vectorEffect() == VE_NON_SCALING_STROKE;

    GraphicsContextStateSaver stateSaver(*context, false);
    if (nonScalingStroke) {
        SVGStyledTransformableElement* element = static_cast<SVGStyledTransformableElement*>(node());
        AffineTransform nonScalingStrokeTransform = element->getScreenCTM(SVGLocatable::DisallowStyleUpdate);
        if (!nonScalingStrokeTransform.isInvertible())
            return;

        path = m_path;
        path.transform(nonScalingStrokeTransform);

        stateSaver.save();
        context->concatCTM(nonScalingStrokeTransform.inverse());
    }

    if (strokePaintingResource->applyResource(this, style, context, ApplyToStrokeMode))
        strokePaintingResource->postApplyResource(this, context, ApplyToStrokeMode, nonScalingStroke ? &path : &m_path);
    else if (fallbackColor.isValid()) {
        RenderSVGResourceSolidColor* fallbackResource = RenderSVGResource::sharedSolidPaintingResource();
        fallbackResource->setColor(fallbackColor);
        if (fallbackResource->applyResource(this, style, context, ApplyToStrokeMode))
            fallbackResource->postApplyResource(this, context, ApplyToStrokeMode, nonScalingStroke ? &path : &m_path);
    }
}

void RenderSVGPath::paint(PaintInfo& paintInfo, int, int)
{
    if (paintInfo.context->paintingDisabled() || style()->visibility() == HIDDEN || m_path.isEmpty())
        return;

    FloatRect boundingBox = repaintRectInLocalCoordinates();
    if (!SVGRenderSupport::paintInfoIntersectsRepaintRect(boundingBox, m_localTransform, paintInfo))
        return;

    PaintInfo childPaintInfo(paintInfo);
    bool drawsOutline = style()->outlineWidth() && (childPaintInfo.phase == PaintPhaseOutline || childPaintInfo.phase == PaintPhaseSelfOutline);
    if (drawsOutline || childPaintInfo.phase == PaintPhaseForeground) {
        GraphicsContextStateSaver stateSaver(*childPaintInfo.context);
        childPaintInfo.applyTransform(m_localTransform);

        if (childPaintInfo.phase == PaintPhaseForeground) {
            PaintInfo savedInfo(childPaintInfo);

            if (SVGRenderSupport::prepareToRenderSVGContent(this, childPaintInfo)) {
                const SVGRenderStyle* svgStyle = style()->svgStyle();
                if (svgStyle->shapeRendering() == SR_CRISPEDGES)
                    childPaintInfo.context->setShouldAntialias(false);

                fillAndStrokePath(childPaintInfo.context);

                if (svgStyle->hasMarkers())
                    m_markerLayoutInfo.drawMarkers(childPaintInfo);
            }

            SVGRenderSupport::finishRenderSVGContent(this, childPaintInfo, savedInfo.context);
        }

        if (drawsOutline)
            paintOutline(childPaintInfo.context, static_cast<int>(boundingBox.x()), static_cast<int>(boundingBox.y()),
                static_cast<int>(boundingBox.width()), static_cast<int>(boundingBox.height()));
    }
}

// This method is called from inside paintOutline() since we call paintOutline()
// while transformed to our coord system, return local coords
void RenderSVGPath::addFocusRingRects(Vector<IntRect>& rects, int, int) 
{
    IntRect rect = enclosingIntRect(repaintRectInLocalCoordinates());
    if (!rect.isEmpty())
        rects.append(rect);
}

bool RenderSVGPath::nodeAtFloatPoint(const HitTestRequest& request, HitTestResult& result, const FloatPoint& pointInParent, HitTestAction hitTestAction)
{
    // We only draw in the forground phase, so we only hit-test then.
    if (hitTestAction != HitTestForeground)
        return false;

    FloatPoint localPoint = m_localTransform.inverse().mapPoint(pointInParent);

    if (!SVGRenderSupport::pointInClippingArea(this, localPoint))
        return false;

    PointerEventsHitRules hitRules(PointerEventsHitRules::SVG_PATH_HITTESTING, request, style()->pointerEvents());
    bool isVisible = (style()->visibility() == VISIBLE);
    if (isVisible || !hitRules.requireVisible) {
        const SVGRenderStyle* svgStyle = style()->svgStyle();
        WindRule fillRule = svgStyle->fillRule();
        if (request.svgClipContent())
            fillRule = svgStyle->clipRule();
        if ((hitRules.canHitStroke && (svgStyle->hasStroke() || !hitRules.requireStroke) && strokeContains(localPoint, hitRules.requireStroke))
            || (hitRules.canHitFill && (svgStyle->hasFill() || !hitRules.requireFill) && fillContains(localPoint, hitRules.requireFill, fillRule))) {
            updateHitTestResult(result, roundedIntPoint(localPoint));
            return true;
        }
    }
    return false;
}

FloatRect RenderSVGPath::calculateMarkerBoundsIfNeeded()
{
    SVGElement* svgElement = static_cast<SVGElement*>(node());
    ASSERT(svgElement && svgElement->document());
    if (!svgElement->isStyled())
        return FloatRect();

    SVGStyledElement* styledElement = static_cast<SVGStyledElement*>(svgElement);
    if (!styledElement->supportsMarkers())
        return FloatRect();

    const SVGRenderStyle* svgStyle = style()->svgStyle();
    ASSERT(svgStyle->hasMarkers());

    SVGResources* resources = SVGResourcesCache::cachedResourcesForRenderObject(this);
    if (!resources)
        return FloatRect();

    RenderSVGResourceMarker* markerStart = resources->markerStart();
    RenderSVGResourceMarker* markerMid = resources->markerMid();
    RenderSVGResourceMarker* markerEnd = resources->markerEnd();
    if (!markerStart && !markerMid && !markerEnd)
        return FloatRect();

    return m_markerLayoutInfo.calculateBoundaries(markerStart, markerMid, markerEnd, svgStyle->strokeWidth().value(svgElement), m_path);
}

void RenderSVGPath::updateCachedBoundaries()
{
    if (m_path.isEmpty()) {
        m_fillBoundingBox = FloatRect();
        m_strokeAndMarkerBoundingBox = FloatRect();
        m_repaintBoundingBox = FloatRect();
        return;
    }

    // Cache _unclipped_ fill bounding box, used for calculations in resources
    m_fillBoundingBox = m_path.boundingRect();

    // Cache _unclipped_ stroke bounding box, used for calculations in resources (includes marker boundaries)
    m_strokeAndMarkerBoundingBox = m_fillBoundingBox;

    const SVGRenderStyle* svgStyle = style()->svgStyle();
    if (svgStyle->hasStroke()) {
        BoundingRectStrokeStyleApplier strokeStyle(this, style());
        m_strokeAndMarkerBoundingBox.unite(m_path.strokeBoundingRect(&strokeStyle));
    }

    if (svgStyle->hasMarkers()) {
        FloatRect markerBounds = calculateMarkerBoundsIfNeeded();
        if (!markerBounds.isEmpty())
            m_strokeAndMarkerBoundingBox.unite(markerBounds);
    }

    // Cache smallest possible repaint rectangle
    m_repaintBoundingBox = m_strokeAndMarkerBoundingBox;
    SVGRenderSupport::intersectRepaintRectWithResources(this, m_repaintBoundingBox);
}

}

#endif // ENABLE(SVG)
