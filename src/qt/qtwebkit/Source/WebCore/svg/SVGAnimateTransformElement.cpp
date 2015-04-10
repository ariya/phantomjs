/*
 * Copyright (C) 2004, 2005 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006, 2007 Rob Buis <buis@kde.org>
 * Copyright (C) 2007 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2008 Apple Inc. All rights reserved.
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
#include "SVGAnimateTransformElement.h"

#include "Attribute.h"
#include "SVGNames.h"
#include "SVGTransformable.h"

namespace WebCore {

inline SVGAnimateTransformElement::SVGAnimateTransformElement(const QualifiedName& tagName, Document* document)
    : SVGAnimateElement(tagName, document)
    , m_type(SVGTransform::SVG_TRANSFORM_UNKNOWN)
{
    ASSERT(hasTagName(SVGNames::animateTransformTag));
}

PassRefPtr<SVGAnimateTransformElement> SVGAnimateTransformElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new SVGAnimateTransformElement(tagName, document));
}

bool SVGAnimateTransformElement::hasValidAttributeType()
{
    SVGElement* targetElement = this->targetElement();
    if (!targetElement)
        return false;

    return m_animatedPropertyType == AnimatedTransformList;
}

bool SVGAnimateTransformElement::isSupportedAttribute(const QualifiedName& attrName)
{
    DEFINE_STATIC_LOCAL(HashSet<QualifiedName>, supportedAttributes, ());
    if (supportedAttributes.isEmpty())
        supportedAttributes.add(SVGNames::typeAttr);
    return supportedAttributes.contains<SVGAttributeHashTranslator>(attrName);
}

void SVGAnimateTransformElement::parseAttribute(const QualifiedName& name, const AtomicString& value)
{
    if (!isSupportedAttribute(name)) {
        SVGAnimateElement::parseAttribute(name, value);
        return;
    }

    if (name == SVGNames::typeAttr) {
        m_type = SVGTransformable::parseTransformType(value);
        if (m_type == SVGTransform::SVG_TRANSFORM_MATRIX)
            m_type = SVGTransform::SVG_TRANSFORM_UNKNOWN;
        return;
    }

    ASSERT_NOT_REACHED();
}

}

#endif // ENABLE(SVG)
