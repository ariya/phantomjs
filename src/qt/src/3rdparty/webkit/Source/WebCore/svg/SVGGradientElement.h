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

#ifndef SVGGradientElement_h
#define SVGGradientElement_h

#if ENABLE(SVG)
#include "Gradient.h"
#include "SVGAnimatedBoolean.h"
#include "SVGAnimatedEnumeration.h"
#include "SVGAnimatedTransformList.h"
#include "SVGExternalResourcesRequired.h"
#include "SVGStyledElement.h"
#include "SVGURIReference.h"

namespace WebCore {

class SVGGradientElement : public SVGStyledElement,
                           public SVGURIReference,
                           public SVGExternalResourcesRequired {
public:
    Vector<Gradient::ColorStop> buildStops();
 
protected:
    SVGGradientElement(const QualifiedName&, Document*);

    virtual void parseMappedAttribute(Attribute*);
    virtual void svgAttributeChanged(const QualifiedName&);
    virtual void synchronizeProperty(const QualifiedName&);
    void fillPassedAttributeToPropertyTypeMap(AttributeToPropertyTypeMap&);

private:
    virtual bool needsPendingResourceHandling() const { return false; }

    virtual void childrenChanged(bool changedByParser = false, Node* beforeChange = 0, Node* afterChange = 0, int childCountDelta = 0);

    // Animated property declarations
    DECLARE_ANIMATED_ENUMERATION(SpreadMethod, spreadMethod)
    DECLARE_ANIMATED_ENUMERATION(GradientUnits, gradientUnits)
    DECLARE_ANIMATED_TRANSFORM_LIST(GradientTransform, gradientTransform)

    // SVGURIReference
    DECLARE_ANIMATED_STRING(Href, href)

    // SVGExternalResourcesRequired
    DECLARE_ANIMATED_BOOLEAN(ExternalResourcesRequired, externalResourcesRequired)
};

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
