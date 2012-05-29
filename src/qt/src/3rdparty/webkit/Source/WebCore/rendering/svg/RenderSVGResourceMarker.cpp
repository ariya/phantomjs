/*
 * Copyright (C) 2004, 2005, 2007, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006, 2007, 2008 Rob Buis <buis@kde.org>
 * Copyright (C) Research In Motion Limited 2009-2010. All rights reserved.
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
#include "RenderSVGResourceMarker.h"

#include "GraphicsContext.h"
#include "RenderSVGContainer.h"
#include "SVGElement.h"
#include "SVGMarkerElement.h"
#include "SVGRenderSupport.h"
#include "SVGStyledElement.h"
#include "SVGStyledTransformableElement.h"

namespace WebCore {

RenderSVGResourceType RenderSVGResourceMarker::s_resourceType = MarkerResourceType;

RenderSVGResourceMarker::RenderSVGResourceMarker(SVGMarkerElement* node)
    : RenderSVGResourceContainer(node)
{
}

RenderSVGResourceMarker::~RenderSVGResourceMarker()
{
}

void RenderSVGResourceMarker::layout()
{
    // Invalidate all resources if our layout changed.
    if (m_everHadLayout && selfNeedsLayout())
        removeAllClientsFromCache();

    // RenderSVGHiddenContainer overwrites layout(). We need the
    // layouting of RenderSVGContainer for calculating  local
    // transformations and repaint.
    RenderSVGContainer::layout();
}

void RenderSVGResourceMarker::removeAllClientsFromCache(bool markForInvalidation)
{
    markAllClientsForInvalidation(markForInvalidation ? LayoutAndBoundariesInvalidation : ParentOnlyInvalidation);
}

void RenderSVGResourceMarker::removeClientFromCache(RenderObject* client, bool markForInvalidation)
{
    ASSERT(client);
    markClientForInvalidation(client, markForInvalidation ? BoundariesInvalidation : ParentOnlyInvalidation);
}

void RenderSVGResourceMarker::applyViewportClip(PaintInfo& paintInfo)
{
    if (SVGRenderSupport::isOverflowHidden(this))
        paintInfo.context->clip(m_viewport);
}

FloatRect RenderSVGResourceMarker::markerBoundaries(const AffineTransform& markerTransformation) const
{
    FloatRect coordinates = RenderSVGContainer::repaintRectInLocalCoordinates();

    // Map repaint rect into parent coordinate space, in which the marker boundaries have to be evaluated
    coordinates = localToParentTransform().mapRect(coordinates);

    return markerTransformation.mapRect(coordinates);
}

const AffineTransform& RenderSVGResourceMarker::localToParentTransform() const
{
    m_localToParentTransform = AffineTransform::translation(m_viewport.x(), m_viewport.y()) * viewportTransform();
    return m_localToParentTransform;
    // If this class were ever given a localTransform(), then the above would read:
    // return viewportTranslation * localTransform() * viewportTransform();
}

FloatPoint RenderSVGResourceMarker::referencePoint() const
{
    SVGMarkerElement* marker = static_cast<SVGMarkerElement*>(node());
    ASSERT(marker);

    return FloatPoint(marker->refX().value(marker), marker->refY().value(marker));
}

float RenderSVGResourceMarker::angle() const
{
    SVGMarkerElement* marker = static_cast<SVGMarkerElement*>(node());
    ASSERT(marker);

    float angle = -1;
    if (marker->orientType() == SVGMarkerElement::SVG_MARKER_ORIENT_ANGLE)
        angle = marker->orientAngle().value();

    return angle;
}

AffineTransform RenderSVGResourceMarker::markerTransformation(const FloatPoint& origin, float autoAngle, float strokeWidth) const
{
    SVGMarkerElement* marker = static_cast<SVGMarkerElement*>(node());
    ASSERT(marker);

    float markerAngle = angle();
    bool useStrokeWidth = (marker->markerUnits() == SVGMarkerElement::SVG_MARKERUNITS_STROKEWIDTH);

    AffineTransform transform;
    transform.translate(origin.x(), origin.y());
    transform.rotate(markerAngle == -1 ? autoAngle : markerAngle);
    transform = markerContentTransformation(transform, referencePoint(), useStrokeWidth ? strokeWidth : -1);
    return transform;
}

void RenderSVGResourceMarker::draw(PaintInfo& paintInfo, const AffineTransform& transform)
{
    PaintInfo info(paintInfo);
    GraphicsContextStateSaver stateSaver(*info.context);
    info.applyTransform(transform);
    RenderSVGContainer::paint(info, 0, 0);
}

AffineTransform RenderSVGResourceMarker::markerContentTransformation(const AffineTransform& contentTransformation, const FloatPoint& origin, float strokeWidth) const
{
    // The 'origin' coordinate maps to SVGs refX/refY, given in coordinates relative to the viewport established by the marker
    FloatPoint mappedOrigin = viewportTransform().mapPoint(origin);

    AffineTransform transformation = contentTransformation;
    if (strokeWidth != -1)
        transformation.scaleNonUniform(strokeWidth, strokeWidth);

    transformation.translate(-mappedOrigin.x(), -mappedOrigin.y());
    return transformation;
}

AffineTransform RenderSVGResourceMarker::viewportTransform() const
{
    SVGMarkerElement* marker = static_cast<SVGMarkerElement*>(node());
    ASSERT(marker);

    return marker->viewBoxToViewTransform(m_viewport.width(), m_viewport.height());
}

void RenderSVGResourceMarker::calcViewport()
{
    if (!selfNeedsLayout())
        return;

    SVGMarkerElement* marker = static_cast<SVGMarkerElement*>(node());
    ASSERT(marker);

    float w = marker->markerWidth().value(marker);
    float h = marker->markerHeight().value(marker);
    m_viewport = FloatRect(0, 0, w, h);
}

}

#endif // ENABLE(SVG)
