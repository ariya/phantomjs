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
#include "RenderSVGResourceLinearGradient.h"

#include "LinearGradientAttributes.h"
#include "SVGLinearGradientElement.h"

namespace WebCore {

RenderSVGResourceType RenderSVGResourceLinearGradient::s_resourceType = LinearGradientResourceType;

RenderSVGResourceLinearGradient::RenderSVGResourceLinearGradient(SVGLinearGradientElement* node)
    : RenderSVGResourceGradient(node)
{
}

RenderSVGResourceLinearGradient::~RenderSVGResourceLinearGradient()
{
}

void RenderSVGResourceLinearGradient::collectGradientAttributes(SVGGradientElement* gradientElement)
{
    m_attributes = LinearGradientAttributes();
    static_cast<SVGLinearGradientElement*>(gradientElement)->collectGradientAttributes(m_attributes);
}

void RenderSVGResourceLinearGradient::buildGradient(GradientData* gradientData, SVGGradientElement* gradientElement) const
{
    SVGLinearGradientElement* linearGradientElement = static_cast<SVGLinearGradientElement*>(gradientElement);

    // Determine gradient start/end points
    FloatPoint startPoint;
    FloatPoint endPoint;
    linearGradientElement->calculateStartEndPoints(m_attributes, startPoint, endPoint);

    gradientData->gradient = Gradient::create(startPoint, endPoint);
    gradientData->gradient->setSpreadMethod(m_attributes.spreadMethod());

    // Add stops
    addStops(gradientData, m_attributes.stops());
}

}

#endif
