/*
 * Copyright (C) 2004, 2005 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006 Rob Buis <buis@kde.org>
 * Copyright (C) 2005 Oliver Hunt <oliver@nerget.com>
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

#ifndef SVGFELightElement_h
#define SVGFELightElement_h

#if ENABLE(SVG) && ENABLE(FILTERS)
#include "LightSource.h"
#include "SVGAnimatedNumber.h"
#include "SVGElement.h"

namespace WebCore {

class SVGFELightElement : public SVGElement {
public:
    virtual PassRefPtr<LightSource> lightSource() const = 0;
    static SVGFELightElement* findLightElement(const SVGElement*);
    static PassRefPtr<LightSource> findLightSource(const SVGElement*);

protected:
    SVGFELightElement(const QualifiedName&, Document*);

private:
    virtual void parseMappedAttribute(Attribute*);
    virtual void svgAttributeChanged(const QualifiedName&);
    virtual void synchronizeProperty(const QualifiedName&);
    virtual void fillAttributeToPropertyTypeMap();
    virtual AttributeToPropertyTypeMap& attributeToPropertyTypeMap();
    virtual void childrenChanged(bool changedByParser = false, Node* beforeChange = 0, Node* afterChange = 0, int childCountDelta = 0);

    // Animated property declarations
    DECLARE_ANIMATED_NUMBER(Azimuth, azimuth)
    DECLARE_ANIMATED_NUMBER(Elevation, elevation)
    DECLARE_ANIMATED_NUMBER(X, x)
    DECLARE_ANIMATED_NUMBER(Y, y)
    DECLARE_ANIMATED_NUMBER(Z, z)
    DECLARE_ANIMATED_NUMBER(PointsAtX, pointsAtX)
    DECLARE_ANIMATED_NUMBER(PointsAtY, pointsAtY)
    DECLARE_ANIMATED_NUMBER(PointsAtZ, pointsAtZ)
    DECLARE_ANIMATED_NUMBER(SpecularExponent, specularExponent)
    DECLARE_ANIMATED_NUMBER(LimitingConeAngle, limitingConeAngle)
};

} // namespace WebCore

#endif // ENABLE(SVG) && ENABLE(FILTERS)
#endif
