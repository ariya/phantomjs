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
#include "RenderSVGPath.h"

#include "SVGGraphicsElement.h"
#include "SVGPathElement.h"
#include "SVGSubpathData.h"

namespace WebCore {

RenderSVGPath::RenderSVGPath(SVGGraphicsElement* node)
    : RenderSVGShape(node)
{
}

RenderSVGPath::~RenderSVGPath()
{
}

void RenderSVGPath::updateShapeFromElement()
{
    RenderSVGShape::updateShapeFromElement();
    updateZeroLengthSubpaths();

    m_strokeBoundingBox = calculateUpdatedStrokeBoundingBox();
}

FloatRect RenderSVGPath::calculateUpdatedStrokeBoundingBox() const
{
    FloatRect strokeBoundingBox = m_strokeBoundingBox;

    if (style()->svgStyle()->hasStroke()) {
        // FIXME: zero-length subpaths do not respect vector-effect = non-scaling-stroke.
        float strokeWidth = this->strokeWidth();
        for (size_t i = 0; i < m_zeroLengthLinecapLocations.size(); ++i)
            strokeBoundingBox.unite(zeroLengthSubpathRect(m_zeroLengthLinecapLocations[i], strokeWidth));
    }

    return strokeBoundingBox;
}

static void useStrokeStyleToFill(GraphicsContext* context)
{
    if (Gradient* gradient = context->strokeGradient())
        context->setFillGradient(gradient);
    else if (Pattern* pattern = context->strokePattern())
        context->setFillPattern(pattern);
    else
        context->setFillColor(context->strokeColor(), context->strokeColorSpace());
}

void RenderSVGPath::strokeShape(GraphicsContext* context) const
{
    if (!style()->svgStyle()->hasVisibleStroke())
        return;

    RenderSVGShape::strokeShape(context);

    if (m_zeroLengthLinecapLocations.isEmpty())
        return;

    Path* usePath;
    AffineTransform nonScalingTransform;

    if (hasNonScalingStroke())
        nonScalingTransform = nonScalingStrokeTransform();

    GraphicsContextStateSaver stateSaver(*context, true);
    useStrokeStyleToFill(context);
    for (size_t i = 0; i < m_zeroLengthLinecapLocations.size(); ++i) {
        usePath = zeroLengthLinecapPath(m_zeroLengthLinecapLocations[i]);
        if (hasNonScalingStroke())
            usePath = nonScalingStrokePath(usePath, nonScalingTransform);
        context->fillPath(*usePath);
    }
}

bool RenderSVGPath::shapeDependentStrokeContains(const FloatPoint& point)
{
    if (RenderSVGShape::shapeDependentStrokeContains(point))
        return true;

    const SVGRenderStyle* svgStyle = style()->svgStyle();
    for (size_t i = 0; i < m_zeroLengthLinecapLocations.size(); ++i) {
        ASSERT(svgStyle->hasStroke());
        float strokeWidth = this->strokeWidth();
        if (svgStyle->capStyle() == SquareCap) {
            if (zeroLengthSubpathRect(m_zeroLengthLinecapLocations[i], strokeWidth).contains(point))
                return true;
        } else {
            ASSERT(svgStyle->capStyle() == RoundCap);
            FloatPoint radiusVector(point.x() - m_zeroLengthLinecapLocations[i].x(), point.y() -  m_zeroLengthLinecapLocations[i].y());
            if (radiusVector.lengthSquared() < strokeWidth * strokeWidth * .25f)
                return true;
        }
    }
    return false;
}

bool RenderSVGPath::shouldStrokeZeroLengthSubpath() const
{
    // Spec(11.4): Any zero length subpath shall not be stroked if the "stroke-linecap" property has a value of butt
    // but shall be stroked if the "stroke-linecap" property has a value of round or square
    return style()->svgStyle()->hasStroke() && style()->svgStyle()->capStyle() != ButtCap;
}

Path* RenderSVGPath::zeroLengthLinecapPath(const FloatPoint& linecapPosition) const
{
    DEFINE_STATIC_LOCAL(Path, tempPath, ());

    tempPath.clear();
    if (style()->svgStyle()->capStyle() == SquareCap)
        tempPath.addRect(zeroLengthSubpathRect(linecapPosition, this->strokeWidth()));
    else
        tempPath.addEllipse(zeroLengthSubpathRect(linecapPosition, this->strokeWidth()));

    return &tempPath;
}

FloatRect RenderSVGPath::zeroLengthSubpathRect(const FloatPoint& linecapPosition, float strokeWidth) const
{
    return FloatRect(linecapPosition.x() - strokeWidth / 2, linecapPosition.y() - strokeWidth / 2, strokeWidth, strokeWidth);
}

void RenderSVGPath::updateZeroLengthSubpaths()
{
    m_zeroLengthLinecapLocations.clear();

    if (!strokeWidth() || !shouldStrokeZeroLengthSubpath())
        return;

    SVGSubpathData subpathData(m_zeroLengthLinecapLocations);
    path().apply(&subpathData, SVGSubpathData::updateFromPathElement);
    subpathData.pathIsDone();
}

}

#endif // ENABLE(SVG)
