/*
 * Copyright (C) 2004, 2005, 2006, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006, 2007 Rob Buis <buis@kde.org>
 * Copyright (C) 2008 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2008 Dirk Schulze <krit@webkit.org>
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
#include "SVGLinearGradientElement.h"

#include "Attribute.h"
#include "Document.h"
#include "FloatPoint.h"
#include "LinearGradientAttributes.h"
#include "RenderSVGResourceLinearGradient.h"
#include "SVGElementInstance.h"
#include "SVGLength.h"
#include "SVGNames.h"
#include "SVGTransform.h"
#include "SVGTransformList.h"
#include "SVGUnitTypes.h"

namespace WebCore {

// Animated property definitions
DEFINE_ANIMATED_LENGTH(SVGLinearGradientElement, SVGNames::x1Attr, X1, x1)
DEFINE_ANIMATED_LENGTH(SVGLinearGradientElement, SVGNames::y1Attr, Y1, y1)
DEFINE_ANIMATED_LENGTH(SVGLinearGradientElement, SVGNames::x2Attr, X2, x2)
DEFINE_ANIMATED_LENGTH(SVGLinearGradientElement, SVGNames::y2Attr, Y2, y2)

BEGIN_REGISTER_ANIMATED_PROPERTIES(SVGLinearGradientElement)
    REGISTER_LOCAL_ANIMATED_PROPERTY(x1)
    REGISTER_LOCAL_ANIMATED_PROPERTY(y1)
    REGISTER_LOCAL_ANIMATED_PROPERTY(x2)
    REGISTER_LOCAL_ANIMATED_PROPERTY(y2)
    REGISTER_PARENT_ANIMATED_PROPERTIES(SVGGradientElement)
END_REGISTER_ANIMATED_PROPERTIES

inline SVGLinearGradientElement::SVGLinearGradientElement(const QualifiedName& tagName, Document* document)
    : SVGGradientElement(tagName, document)
    , m_x1(LengthModeWidth)
    , m_y1(LengthModeHeight)
    , m_x2(LengthModeWidth, "100%")
    , m_y2(LengthModeHeight)
{
    // Spec: If the x2 attribute is not specified, the effect is as if a value of "100%" were specified.
    ASSERT(hasTagName(SVGNames::linearGradientTag));
    registerAnimatedPropertiesForSVGLinearGradientElement();
}

PassRefPtr<SVGLinearGradientElement> SVGLinearGradientElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new SVGLinearGradientElement(tagName, document));
}

bool SVGLinearGradientElement::isSupportedAttribute(const QualifiedName& attrName)
{
    DEFINE_STATIC_LOCAL(HashSet<QualifiedName>, supportedAttributes, ());
    if (supportedAttributes.isEmpty()) {
        supportedAttributes.add(SVGNames::x1Attr);
        supportedAttributes.add(SVGNames::x2Attr);
        supportedAttributes.add(SVGNames::y1Attr);
        supportedAttributes.add(SVGNames::y2Attr);
    }
    return supportedAttributes.contains<SVGAttributeHashTranslator>(attrName);
}

void SVGLinearGradientElement::parseAttribute(const QualifiedName& name, const AtomicString& value)
{
    SVGParsingError parseError = NoError;

    if (!isSupportedAttribute(name))
        SVGGradientElement::parseAttribute(name, value);
    else if (name == SVGNames::x1Attr)
        setX1BaseValue(SVGLength::construct(LengthModeWidth, value, parseError));
    else if (name == SVGNames::y1Attr)
        setY1BaseValue(SVGLength::construct(LengthModeHeight, value, parseError));
    else if (name == SVGNames::x2Attr)
        setX2BaseValue(SVGLength::construct(LengthModeWidth, value, parseError));
    else if (name == SVGNames::y2Attr)
        setY2BaseValue(SVGLength::construct(LengthModeHeight, value, parseError));
    else
        ASSERT_NOT_REACHED();

    reportAttributeParsingError(parseError, name, value);
}

void SVGLinearGradientElement::svgAttributeChanged(const QualifiedName& attrName)
{
    if (!isSupportedAttribute(attrName)) {
        SVGGradientElement::svgAttributeChanged(attrName);
        return;
    }

    SVGElementInstance::InvalidationGuard invalidationGuard(this);
    
    updateRelativeLengthsInformation();

    if (RenderObject* object = renderer())
        object->setNeedsLayout(true);
}

RenderObject* SVGLinearGradientElement::createRenderer(RenderArena* arena, RenderStyle*)
{
    return new (arena) RenderSVGResourceLinearGradient(this);
}

bool SVGLinearGradientElement::collectGradientAttributes(LinearGradientAttributes& attributes)
{
    HashSet<SVGGradientElement*> processedGradients;

    bool isLinear = true;
    SVGGradientElement* current = this;

    while (current) {
        if (!current->renderer())
            return false;

        if (!attributes.hasSpreadMethod() && current->hasAttribute(SVGNames::spreadMethodAttr))
            attributes.setSpreadMethod(current->spreadMethod());

        if (!attributes.hasGradientUnits() && current->hasAttribute(SVGNames::gradientUnitsAttr))
            attributes.setGradientUnits(current->gradientUnits());

        if (!attributes.hasGradientTransform() && current->hasAttribute(SVGNames::gradientTransformAttr)) {
            AffineTransform transform;
            current->gradientTransform().concatenate(transform);
            attributes.setGradientTransform(transform);
        }

        if (!attributes.hasStops()) {
            const Vector<Gradient::ColorStop>& stops(current->buildStops());
            if (!stops.isEmpty())
                attributes.setStops(stops);
        }

        if (isLinear) {
            SVGLinearGradientElement* linear = static_cast<SVGLinearGradientElement*>(current);

            if (!attributes.hasX1() && current->hasAttribute(SVGNames::x1Attr))
                attributes.setX1(linear->x1());

            if (!attributes.hasY1() && current->hasAttribute(SVGNames::y1Attr))
                attributes.setY1(linear->y1());

            if (!attributes.hasX2() && current->hasAttribute(SVGNames::x2Attr))
                attributes.setX2(linear->x2());

            if (!attributes.hasY2() && current->hasAttribute(SVGNames::y2Attr))
                attributes.setY2(linear->y2());
        }

        processedGradients.add(current);

        // Respect xlink:href, take attributes from referenced element
        Node* refNode = SVGURIReference::targetElementFromIRIString(current->href(), document());
        if (refNode && (refNode->hasTagName(SVGNames::linearGradientTag) || refNode->hasTagName(SVGNames::radialGradientTag))) {
            current = toSVGGradientElement(refNode);

            // Cycle detection
            if (processedGradients.contains(current)) {
                current = 0;
                break;
            }

            isLinear = current->hasTagName(SVGNames::linearGradientTag);
        } else
            current = 0;
    }

    return true;
}

bool SVGLinearGradientElement::selfHasRelativeLengths() const
{
    return x1().isRelative()
        || y1().isRelative()
        || x2().isRelative()
        || y2().isRelative();
}

}

#endif // ENABLE(SVG)
