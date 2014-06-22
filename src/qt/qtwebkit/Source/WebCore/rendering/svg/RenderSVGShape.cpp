/*
 * Copyright (C) 2004, 2005, 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2008 Rob Buis <buis@kde.org>
 * Copyright (C) 2005, 2007 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2009 Google, Inc.
 * Copyright (C) 2009 Dirk Schulze <krit@webkit.org>
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
 * Copyright (C) 2009 Jeff Schiller <codedread@gmail.com>
 * Copyright (C) 2011 Renata Hodovan <reni@webkit.org>
 * Copyright (C) 2011 University of Szeged
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
#include "RenderSVGShape.h"

#include "FloatPoint.h"
#include "FloatQuad.h"
#include "GraphicsContext.h"
#include "HitTestRequest.h"
#include "LayoutRepainter.h"
#include "PointerEventsHitRules.h"
#include "RenderSVGContainer.h"
#include "RenderSVGResourceMarker.h"
#include "RenderSVGResourceSolidColor.h"
#include "SVGGraphicsElement.h"
#include "SVGPathData.h"
#include "SVGRenderingContext.h"
#include "SVGResources.h"
#include "SVGResourcesCache.h"
#include "SVGTransformList.h"
#include "SVGURIReference.h"
#include "StrokeStyleApplier.h"
#include <wtf/MathExtras.h>
#include <wtf/StackStats.h>

namespace WebCore {

RenderSVGShape::RenderSVGShape(SVGGraphicsElement* node)
    : RenderSVGModelObject(node)
    , m_needsBoundariesUpdate(false) // Default is false, the cached rects are empty from the beginning.
    , m_needsShapeUpdate(true) // Default is true, so we grab a Path object once from SVGGraphicsElement.
    , m_needsTransformUpdate(true) // Default is true, so we grab a AffineTransform object once from SVGGraphicsElement.
{
}

RenderSVGShape::~RenderSVGShape()
{
}

void RenderSVGShape::updateShapeFromElement()
{
    m_path.clear();
    m_path = adoptPtr(new Path);
    ASSERT(RenderSVGShape::isEmpty());

    SVGGraphicsElement* element = toSVGGraphicsElement(node());
    updatePathFromGraphicsElement(element, path());
    processMarkerPositions();

    m_fillBoundingBox = calculateObjectBoundingBox();
    m_strokeBoundingBox = calculateStrokeBoundingBox();
}

bool RenderSVGShape::isEmpty() const
{
    return path().isEmpty();
}

void RenderSVGShape::fillShape(GraphicsContext* context) const
{
    context->fillPath(path());
}

void RenderSVGShape::strokeShape(GraphicsContext* context) const
{
    ASSERT(m_path);
    Path* usePath = m_path.get();

    if (hasNonScalingStroke())
        usePath = nonScalingStrokePath(usePath, nonScalingStrokeTransform());

    context->strokePath(*usePath);
}

bool RenderSVGShape::shapeDependentStrokeContains(const FloatPoint& point)
{
    ASSERT(m_path);
    BoundingRectStrokeStyleApplier applier(this, style());

    if (hasNonScalingStroke()) {
        AffineTransform nonScalingTransform = nonScalingStrokeTransform();
        Path* usePath = nonScalingStrokePath(m_path.get(), nonScalingTransform);

        return usePath->strokeContains(&applier, nonScalingTransform.mapPoint(point));
    }

    return m_path->strokeContains(&applier, point);
}

bool RenderSVGShape::shapeDependentFillContains(const FloatPoint& point, const WindRule fillRule) const
{
    return path().contains(point, fillRule);
}

bool RenderSVGShape::fillContains(const FloatPoint& point, bool requiresFill, const WindRule fillRule)
{
    if (!m_fillBoundingBox.contains(point))
        return false;

    Color fallbackColor;
    if (requiresFill && !RenderSVGResource::fillPaintingResource(this, style(), fallbackColor))
        return false;

    return shapeDependentFillContains(point, fillRule);
}

bool RenderSVGShape::strokeContains(const FloatPoint& point, bool requiresStroke)
{
    if (!strokeBoundingBox().contains(point))
        return false;

    Color fallbackColor;
    if (requiresStroke && !RenderSVGResource::strokePaintingResource(this, style(), fallbackColor))
        return false;

    return shapeDependentStrokeContains(point);
}

void RenderSVGShape::layout()
{
    StackStats::LayoutCheckPoint layoutCheckPoint;
    LayoutRepainter repainter(*this, SVGRenderSupport::checkForSVGRepaintDuringLayout(this) && selfNeedsLayout());
    SVGGraphicsElement* element = toSVGGraphicsElement(node());

    bool updateCachedBoundariesInParents = false;

    if (m_needsShapeUpdate || m_needsBoundariesUpdate) {
        updateShapeFromElement();
        m_needsShapeUpdate = false;
        updateRepaintBoundingBox();
        m_needsBoundariesUpdate = false;
        updateCachedBoundariesInParents = true;
    }

    if (m_needsTransformUpdate) {
        m_localTransform = element->animatedLocalTransform();
        m_needsTransformUpdate = false;
        updateCachedBoundariesInParents = true;
    }

    // Invalidate all resources of this client if our layout changed.
    if (everHadLayout() && selfNeedsLayout())
        SVGResourcesCache::clientLayoutChanged(this);

    // If our bounds changed, notify the parents.
    if (updateCachedBoundariesInParents)
        RenderSVGModelObject::setNeedsBoundariesUpdate();

    repainter.repaintAfterLayout();
    setNeedsLayout(false);
}

Path* RenderSVGShape::nonScalingStrokePath(const Path* path, const AffineTransform& strokeTransform) const
{
    DEFINE_STATIC_LOCAL(Path, tempPath, ());

    tempPath = *path;
    tempPath.transform(strokeTransform);

    return &tempPath;
}

bool RenderSVGShape::setupNonScalingStrokeContext(AffineTransform& strokeTransform, GraphicsContextStateSaver& stateSaver)
{
    if (!strokeTransform.isInvertible())
        return false;

    stateSaver.save();
    stateSaver.context()->concatCTM(strokeTransform.inverse());
    return true;
}

AffineTransform RenderSVGShape::nonScalingStrokeTransform() const
{
    SVGGraphicsElement* element = toSVGGraphicsElement(node());
    return element->getScreenCTM(SVGLocatable::DisallowStyleUpdate);
}

bool RenderSVGShape::shouldGenerateMarkerPositions() const
{
    if (!style()->svgStyle()->hasMarkers())
        return false;

    SVGGraphicsElement* element = toSVGGraphicsElement(node());
    if (!element->supportsMarkers())
        return false;

    SVGResources* resources = SVGResourcesCache::cachedResourcesForRenderObject(this);
    if (!resources)
        return false;

    return resources->markerStart() || resources->markerMid() || resources->markerEnd();
}

void RenderSVGShape::fillShape(RenderStyle* style, GraphicsContext* context)
{
    Color fallbackColor;
    if (RenderSVGResource* fillPaintingResource = RenderSVGResource::fillPaintingResource(this, style, fallbackColor)) {
        if (fillPaintingResource->applyResource(this, style, context, ApplyToFillMode))
            fillPaintingResource->postApplyResource(this, context, ApplyToFillMode, 0, this);
        else if (fallbackColor.isValid()) {
            RenderSVGResourceSolidColor* fallbackResource = RenderSVGResource::sharedSolidPaintingResource();
            fallbackResource->setColor(fallbackColor);
            if (fallbackResource->applyResource(this, style, context, ApplyToFillMode))
                fallbackResource->postApplyResource(this, context, ApplyToFillMode, 0, this);
        }
    }
}

void RenderSVGShape::strokeShape(RenderStyle* style, GraphicsContext* context)
{
    Color fallbackColor;
    if (RenderSVGResource* strokePaintingResource = RenderSVGResource::strokePaintingResource(this, style, fallbackColor)) {
        if (strokePaintingResource->applyResource(this, style, context, ApplyToStrokeMode))
            strokePaintingResource->postApplyResource(this, context, ApplyToStrokeMode, 0, this);
        else if (fallbackColor.isValid()) {
            RenderSVGResourceSolidColor* fallbackResource = RenderSVGResource::sharedSolidPaintingResource();
            fallbackResource->setColor(fallbackColor);
            if (fallbackResource->applyResource(this, style, context, ApplyToStrokeMode))
                fallbackResource->postApplyResource(this, context, ApplyToStrokeMode, 0, this);
        }
    }
}

void RenderSVGShape::fillAndStrokeShape(GraphicsContext* context)
{
    RenderStyle* style = this->style();

    fillShape(style, context);

    if (!style->svgStyle()->hasVisibleStroke())
        return;

    GraphicsContextStateSaver stateSaver(*context, false);

    if (hasNonScalingStroke()) {
        AffineTransform nonScalingTransform = nonScalingStrokeTransform();
        if (!setupNonScalingStrokeContext(nonScalingTransform, stateSaver))
            return;
    }

    strokeShape(style, context);
}

void RenderSVGShape::paint(PaintInfo& paintInfo, const LayoutPoint&)
{
    if (paintInfo.context->paintingDisabled() || style()->visibility() == HIDDEN || isEmpty())
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
            SVGRenderingContext renderingContext(this, childPaintInfo);

            if (renderingContext.isRenderingPrepared()) {
                const SVGRenderStyle* svgStyle = style()->svgStyle();
                if (svgStyle->shapeRendering() == SR_CRISPEDGES)
                    childPaintInfo.context->setShouldAntialias(false);

                fillAndStrokeShape(childPaintInfo.context);
                if (!m_markerPositions.isEmpty())
                    drawMarkers(childPaintInfo);
            }
        }

        if (drawsOutline)
            paintOutline(childPaintInfo, IntRect(boundingBox));
    }
}

// This method is called from inside paintOutline() since we call paintOutline()
// while transformed to our coord system, return local coords
void RenderSVGShape::addFocusRingRects(Vector<IntRect>& rects, const LayoutPoint&, const RenderLayerModelObject*)
{
    IntRect rect = enclosingIntRect(repaintRectInLocalCoordinates());
    if (!rect.isEmpty())
        rects.append(rect);
}

bool RenderSVGShape::nodeAtFloatPoint(const HitTestRequest& request, HitTestResult& result, const FloatPoint& pointInParent, HitTestAction hitTestAction)
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
            updateHitTestResult(result, roundedLayoutPoint(localPoint));
            return true;
        }
    }
    return false;
}

static inline RenderSVGResourceMarker* markerForType(SVGMarkerType type, RenderSVGResourceMarker* markerStart, RenderSVGResourceMarker* markerMid, RenderSVGResourceMarker* markerEnd)
{
    switch (type) {
    case StartMarker:
        return markerStart;
    case MidMarker:
        return markerMid;
    case EndMarker:
        return markerEnd;
    }

    ASSERT_NOT_REACHED();
    return 0;
}

FloatRect RenderSVGShape::markerRect(float strokeWidth) const
{
    ASSERT(!m_markerPositions.isEmpty());

    SVGResources* resources = SVGResourcesCache::cachedResourcesForRenderObject(this);
    ASSERT(resources);

    RenderSVGResourceMarker* markerStart = resources->markerStart();
    RenderSVGResourceMarker* markerMid = resources->markerMid();
    RenderSVGResourceMarker* markerEnd = resources->markerEnd();
    ASSERT(markerStart || markerMid || markerEnd);

    FloatRect boundaries;
    unsigned size = m_markerPositions.size();
    for (unsigned i = 0; i < size; ++i) {
        if (RenderSVGResourceMarker* marker = markerForType(m_markerPositions[i].type, markerStart, markerMid, markerEnd))
            boundaries.unite(marker->markerBoundaries(marker->markerTransformation(m_markerPositions[i].origin, m_markerPositions[i].angle, strokeWidth)));
    }
    return boundaries;
}

FloatRect RenderSVGShape::calculateObjectBoundingBox() const
{
    return path().fastBoundingRect();
}

FloatRect RenderSVGShape::calculateStrokeBoundingBox() const
{
    ASSERT(m_path);
    FloatRect strokeBoundingBox = m_fillBoundingBox;

    const SVGRenderStyle* svgStyle = style()->svgStyle();
    if (svgStyle->hasStroke()) {
        BoundingRectStrokeStyleApplier strokeStyle(this, style());
        if (hasNonScalingStroke()) {
            AffineTransform nonScalingTransform = nonScalingStrokeTransform();
            if (nonScalingTransform.isInvertible()) {
                Path* usePath = nonScalingStrokePath(m_path.get(), nonScalingTransform);
                FloatRect strokeBoundingRect = usePath->strokeBoundingRect(&strokeStyle);
                strokeBoundingRect = nonScalingTransform.inverse().mapRect(strokeBoundingRect);
                strokeBoundingBox.unite(strokeBoundingRect);
            }
        } else
            strokeBoundingBox.unite(path().strokeBoundingRect(&strokeStyle));
    }

    if (!m_markerPositions.isEmpty())
        strokeBoundingBox.unite(markerRect(strokeWidth()));

    return strokeBoundingBox;
}

void RenderSVGShape::updateRepaintBoundingBox()
{
    m_repaintBoundingBoxExcludingShadow = strokeBoundingBox();
    SVGRenderSupport::intersectRepaintRectWithResources(this, m_repaintBoundingBoxExcludingShadow);

    m_repaintBoundingBox = m_repaintBoundingBoxExcludingShadow;
    SVGRenderSupport::intersectRepaintRectWithShadows(this, m_repaintBoundingBox);
}

float RenderSVGShape::strokeWidth() const
{
    SVGElement* svgElement = toSVGElement(node());
    SVGLengthContext lengthContext(svgElement);
    return style()->svgStyle()->strokeWidth().value(lengthContext);
}

bool RenderSVGShape::hasSmoothStroke() const
{
    const SVGRenderStyle* svgStyle = style()->svgStyle();
    return svgStyle->strokeDashArray().isEmpty()
        && svgStyle->strokeMiterLimit() == svgStyle->initialStrokeMiterLimit()
        && svgStyle->joinStyle() == svgStyle->initialJoinStyle()
        && svgStyle->capStyle() == svgStyle->initialCapStyle();
}

void RenderSVGShape::drawMarkers(PaintInfo& paintInfo)
{
    ASSERT(!m_markerPositions.isEmpty());

    SVGResources* resources = SVGResourcesCache::cachedResourcesForRenderObject(this);
    if (!resources)
        return;

    RenderSVGResourceMarker* markerStart = resources->markerStart();
    RenderSVGResourceMarker* markerMid = resources->markerMid();
    RenderSVGResourceMarker* markerEnd = resources->markerEnd();
    if (!markerStart && !markerMid && !markerEnd)
        return;

    float strokeWidth = this->strokeWidth();
    unsigned size = m_markerPositions.size();
    for (unsigned i = 0; i < size; ++i) {
        if (RenderSVGResourceMarker* marker = markerForType(m_markerPositions[i].type, markerStart, markerMid, markerEnd))
            marker->draw(paintInfo, marker->markerTransformation(m_markerPositions[i].origin, m_markerPositions[i].angle, strokeWidth));
    }
}

void RenderSVGShape::processMarkerPositions()
{
    m_markerPositions.clear();

    if (!shouldGenerateMarkerPositions())
        return;

    ASSERT(m_path);

    SVGMarkerData markerData(m_markerPositions);
    m_path->apply(&markerData, SVGMarkerData::updateFromPathElement);
    markerData.pathIsDone();
}

}

#endif // ENABLE(SVG)
