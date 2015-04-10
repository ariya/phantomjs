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

#ifndef RenderSVGResourceLinearGradient_h
#define RenderSVGResourceLinearGradient_h

#if ENABLE(SVG)
#include "LinearGradientAttributes.h"
#include "RenderSVGResourceGradient.h"

namespace WebCore {

class SVGLinearGradientElement;

class RenderSVGResourceLinearGradient : public RenderSVGResourceGradient {
public:
    RenderSVGResourceLinearGradient(SVGLinearGradientElement*);
    virtual ~RenderSVGResourceLinearGradient();

    virtual const char* renderName() const { return "RenderSVGResourceLinearGradient"; }

    virtual RenderSVGResourceType resourceType() const { return s_resourceType; }
    static RenderSVGResourceType s_resourceType;

    virtual SVGUnitTypes::SVGUnitType gradientUnits() const { return m_attributes.gradientUnits(); }
    virtual void calculateGradientTransform(AffineTransform& transform) { transform = m_attributes.gradientTransform(); }
    virtual bool collectGradientAttributes(SVGGradientElement*);
    virtual void buildGradient(GradientData*) const;

    FloatPoint startPoint(const LinearGradientAttributes&) const;
    FloatPoint endPoint(const LinearGradientAttributes&) const;

private:
    LinearGradientAttributes m_attributes;
};

}

#endif
#endif
