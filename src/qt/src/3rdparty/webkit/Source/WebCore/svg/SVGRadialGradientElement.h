/*
 * Copyright (C) 2004, 2005, 2006, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006 Rob Buis <buis@kde.org>
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

#ifndef SVGRadialGradientElement_h
#define SVGRadialGradientElement_h

#if ENABLE(SVG)
#include "SVGAnimatedLength.h"
#include "SVGGradientElement.h"

namespace WebCore {

struct RadialGradientAttributes;

class SVGRadialGradientElement : public SVGGradientElement {
public:
    static PassRefPtr<SVGRadialGradientElement> create(const QualifiedName&, Document*);

    void collectGradientAttributes(RadialGradientAttributes&);
    void calculateFocalCenterPointsAndRadius(const RadialGradientAttributes&, FloatPoint& focalPoint, FloatPoint& centerPoint, float& radius);

private:
    SVGRadialGradientElement(const QualifiedName&, Document*);

    virtual void parseMappedAttribute(Attribute*);
    virtual void svgAttributeChanged(const QualifiedName&);
    virtual void synchronizeProperty(const QualifiedName&);
    virtual void fillAttributeToPropertyTypeMap();
    virtual AttributeToPropertyTypeMap& attributeToPropertyTypeMap();

    virtual RenderObject* createRenderer(RenderArena*, RenderStyle*);

    virtual bool selfHasRelativeLengths() const;

    // Animated property declarations
    DECLARE_ANIMATED_LENGTH(Cx, cx)
    DECLARE_ANIMATED_LENGTH(Cy, cy)
    DECLARE_ANIMATED_LENGTH(R, r)
    DECLARE_ANIMATED_LENGTH(Fx, fx)
    DECLARE_ANIMATED_LENGTH(Fy, fy)
};

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
