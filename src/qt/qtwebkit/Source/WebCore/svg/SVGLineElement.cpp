/*
 * Copyright (C) 2004, 2005, 2006, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006, 2007 Rob Buis <buis@kde.org>
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
#include "SVGLineElement.h"

#include "Attribute.h"
#include "FloatPoint.h"
#include "RenderSVGPath.h"
#include "RenderSVGResource.h"
#include "SVGElementInstance.h"
#include "SVGLength.h"
#include "SVGNames.h"

namespace WebCore {

// Animated property definitions
DEFINE_ANIMATED_LENGTH(SVGLineElement, SVGNames::x1Attr, X1, x1)
DEFINE_ANIMATED_LENGTH(SVGLineElement, SVGNames::y1Attr, Y1, y1)
DEFINE_ANIMATED_LENGTH(SVGLineElement, SVGNames::x2Attr, X2, x2)
DEFINE_ANIMATED_LENGTH(SVGLineElement, SVGNames::y2Attr, Y2, y2)
DEFINE_ANIMATED_BOOLEAN(SVGLineElement, SVGNames::externalResourcesRequiredAttr, ExternalResourcesRequired, externalResourcesRequired)

BEGIN_REGISTER_ANIMATED_PROPERTIES(SVGLineElement)
    REGISTER_LOCAL_ANIMATED_PROPERTY(x1)
    REGISTER_LOCAL_ANIMATED_PROPERTY(y1)
    REGISTER_LOCAL_ANIMATED_PROPERTY(x2)
    REGISTER_LOCAL_ANIMATED_PROPERTY(y2)
    REGISTER_LOCAL_ANIMATED_PROPERTY(externalResourcesRequired)
    REGISTER_PARENT_ANIMATED_PROPERTIES(SVGGraphicsElement)
END_REGISTER_ANIMATED_PROPERTIES

inline SVGLineElement::SVGLineElement(const QualifiedName& tagName, Document* document)
    : SVGGraphicsElement(tagName, document)
    , m_x1(LengthModeWidth)
    , m_y1(LengthModeHeight)
    , m_x2(LengthModeWidth)
    , m_y2(LengthModeHeight)
{
    ASSERT(hasTagName(SVGNames::lineTag));
    registerAnimatedPropertiesForSVGLineElement();
}

PassRefPtr<SVGLineElement> SVGLineElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new SVGLineElement(tagName, document));
}

bool SVGLineElement::isSupportedAttribute(const QualifiedName& attrName)
{
    DEFINE_STATIC_LOCAL(HashSet<QualifiedName>, supportedAttributes, ());
    if (supportedAttributes.isEmpty()) {
        SVGLangSpace::addSupportedAttributes(supportedAttributes);
        SVGExternalResourcesRequired::addSupportedAttributes(supportedAttributes);
        supportedAttributes.add(SVGNames::x1Attr);
        supportedAttributes.add(SVGNames::x2Attr);
        supportedAttributes.add(SVGNames::y1Attr);
        supportedAttributes.add(SVGNames::y2Attr);
    }
    return supportedAttributes.contains<SVGAttributeHashTranslator>(attrName);
}

void SVGLineElement::parseAttribute(const QualifiedName& name, const AtomicString& value)
{
    SVGParsingError parseError = NoError;

    if (!isSupportedAttribute(name))
        SVGGraphicsElement::parseAttribute(name, value);
    else if (name == SVGNames::x1Attr)
        setX1BaseValue(SVGLength::construct(LengthModeWidth, value, parseError));
    else if (name == SVGNames::y1Attr)
        setY1BaseValue(SVGLength::construct(LengthModeHeight, value, parseError));
    else if (name == SVGNames::x2Attr)
        setX2BaseValue(SVGLength::construct(LengthModeWidth, value, parseError));
    else if (name == SVGNames::y2Attr)
        setY2BaseValue(SVGLength::construct(LengthModeHeight, value, parseError));
    else if (SVGLangSpace::parseAttribute(name, value)
             || SVGExternalResourcesRequired::parseAttribute(name, value)) {
    } else
        ASSERT_NOT_REACHED();

    reportAttributeParsingError(parseError, name, value);
}

void SVGLineElement::svgAttributeChanged(const QualifiedName& attrName)
{
    if (!isSupportedAttribute(attrName)) {
        SVGGraphicsElement::svgAttributeChanged(attrName);
        return;
    }

    SVGElementInstance::InvalidationGuard invalidationGuard(this);
    
    bool isLengthAttribute = attrName == SVGNames::x1Attr
                          || attrName == SVGNames::y1Attr
                          || attrName == SVGNames::x2Attr
                          || attrName == SVGNames::y2Attr;

    if (isLengthAttribute)
        updateRelativeLengthsInformation();

    RenderSVGShape* renderer = toRenderSVGShape(this->renderer());
    if (!renderer)
        return;

    if (isLengthAttribute) {
        renderer->setNeedsShapeUpdate();
        RenderSVGResource::markForLayoutAndParentResourceInvalidation(renderer);
        return;
    }

    if (SVGLangSpace::isKnownAttribute(attrName) || SVGExternalResourcesRequired::isKnownAttribute(attrName)) {
        RenderSVGResource::markForLayoutAndParentResourceInvalidation(renderer);
        return;
    }

    ASSERT_NOT_REACHED();
}

bool SVGLineElement::selfHasRelativeLengths() const
{
    return x1().isRelative()
        || y1().isRelative()
        || x2().isRelative()
        || y2().isRelative();
}

}

#endif // ENABLE(SVG)
