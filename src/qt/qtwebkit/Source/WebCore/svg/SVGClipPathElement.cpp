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
#include "SVGClipPathElement.h"

#include "Attribute.h"
#include "Document.h"
#include "RenderSVGResourceClipper.h"
#include "SVGElementInstance.h"
#include "SVGNames.h"
#include "SVGTransformList.h"
#include "StyleResolver.h"

namespace WebCore {

// Animated property definitions
DEFINE_ANIMATED_ENUMERATION(SVGClipPathElement, SVGNames::clipPathUnitsAttr, ClipPathUnits, clipPathUnits, SVGUnitTypes::SVGUnitType)
DEFINE_ANIMATED_BOOLEAN(SVGClipPathElement, SVGNames::externalResourcesRequiredAttr, ExternalResourcesRequired, externalResourcesRequired)

BEGIN_REGISTER_ANIMATED_PROPERTIES(SVGClipPathElement)
    REGISTER_LOCAL_ANIMATED_PROPERTY(clipPathUnits)
    REGISTER_LOCAL_ANIMATED_PROPERTY(externalResourcesRequired)
    REGISTER_PARENT_ANIMATED_PROPERTIES(SVGGraphicsElement)
END_REGISTER_ANIMATED_PROPERTIES

inline SVGClipPathElement::SVGClipPathElement(const QualifiedName& tagName, Document* document)
    : SVGGraphicsElement(tagName, document)
    , m_clipPathUnits(SVGUnitTypes::SVG_UNIT_TYPE_USERSPACEONUSE)
{
    ASSERT(hasTagName(SVGNames::clipPathTag));
    registerAnimatedPropertiesForSVGClipPathElement();
}

PassRefPtr<SVGClipPathElement> SVGClipPathElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new SVGClipPathElement(tagName, document));
}

bool SVGClipPathElement::isSupportedAttribute(const QualifiedName& attrName)
{
    DEFINE_STATIC_LOCAL(HashSet<QualifiedName>, supportedAttributes, ());
    if (supportedAttributes.isEmpty()) {
        SVGLangSpace::addSupportedAttributes(supportedAttributes);
        SVGExternalResourcesRequired::addSupportedAttributes(supportedAttributes);
        supportedAttributes.add(SVGNames::clipPathUnitsAttr);
    }
    return supportedAttributes.contains<SVGAttributeHashTranslator>(attrName);
}

void SVGClipPathElement::parseAttribute(const QualifiedName& name, const AtomicString& value)
{
    if (!isSupportedAttribute(name)) {
        SVGGraphicsElement::parseAttribute(name, value);
        return;
    }

    if (name == SVGNames::clipPathUnitsAttr) {
        SVGUnitTypes::SVGUnitType propertyValue = SVGPropertyTraits<SVGUnitTypes::SVGUnitType>::fromString(value);
        if (propertyValue > 0)
            setClipPathUnitsBaseValue(propertyValue);
        return;
    }

    if (SVGLangSpace::parseAttribute(name, value))
        return;
    if (SVGExternalResourcesRequired::parseAttribute(name, value))
        return;

    ASSERT_NOT_REACHED();
}

void SVGClipPathElement::svgAttributeChanged(const QualifiedName& attrName)
{
    if (!isSupportedAttribute(attrName)) {
        SVGGraphicsElement::svgAttributeChanged(attrName);
        return;
    }

    SVGElementInstance::InvalidationGuard invalidationGuard(this);

    if (RenderObject* object = renderer())
        object->setNeedsLayout(true);
}

void SVGClipPathElement::childrenChanged(bool changedByParser, Node* beforeChange, Node* afterChange, int childCountDelta)
{
    SVGGraphicsElement::childrenChanged(changedByParser, beforeChange, afterChange, childCountDelta);

    if (changedByParser)
        return;

    if (RenderObject* object = renderer())
        object->setNeedsLayout(true);
}

RenderObject* SVGClipPathElement::createRenderer(RenderArena* arena, RenderStyle*)
{
    return new (arena) RenderSVGResourceClipper(this);
}

}

#endif // ENABLE(SVG)
