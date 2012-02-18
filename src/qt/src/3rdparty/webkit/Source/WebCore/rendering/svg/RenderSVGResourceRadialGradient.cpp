/*
 * Copyright (C) 2006 Nikolas Zimmermann <zimmermann@kde.org>
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

#include "config.h"

#if ENABLE(SVG)
#include "RenderSVGResourceRadialGradient.h"

#include "RadialGradientAttributes.h"
#include "SVGRadialGradientElement.h"

namespace WebCore {

RenderSVGResourceType RenderSVGResourceRadialGradient::s_resourceType = RadialGradientResourceType;

RenderSVGResourceRadialGradient::RenderSVGResourceRadialGradient(SVGRadialGradientElement* node)
    : RenderSVGResourceGradient(node)
{
}

RenderSVGResourceRadialGradient::~RenderSVGResourceRadialGradient()
{
}

void RenderSVGResourceRadialGradient::collectGradientAttributes(SVGGradientElement* gradientElement)
{
    m_attributes = RadialGradientAttributes();
    static_cast<SVGRadialGradientElement*>(gradientElement)->collectGradientAttributes(m_attributes);
}

void RenderSVGResourceRadialGradient::buildGradient(GradientData* gradientData, SVGGradientElement* gradientElement) const
{
    SVGRadialGradientElement* radialGradientElement = static_cast<SVGRadialGradientElement*>(gradientElement);

    // Determine gradient focal/center points and radius
    FloatPoint focalPoint;
    FloatPoint centerPoint;
    float radius;
    radialGradientElement->calculateFocalCenterPointsAndRadius(m_attributes, focalPoint, centerPoint, radius);

    gradientData->gradient = Gradient::create(focalPoint,
                                              0, // SVG does not support a "focus radius"
                                              centerPoint,
                                              radius);

    gradientData->gradient->setSpreadMethod(m_attributes.spreadMethod());

    // Add stops
    addStops(gradientData, m_attributes.stops());
}

}

#endif
