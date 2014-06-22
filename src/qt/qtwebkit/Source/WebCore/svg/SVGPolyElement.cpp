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
#include "SVGPolyElement.h"

#include "Attribute.h"
#include "Document.h"
#include "FloatPoint.h"
#include "RenderSVGPath.h"
#include "RenderSVGResource.h"
#include "SVGAnimatedPointList.h"
#include "SVGElementInstance.h"
#include "SVGNames.h"
#include "SVGParserUtilities.h"

namespace WebCore {

// Define custom animated property 'points'.
const SVGPropertyInfo* SVGPolyElement::pointsPropertyInfo()
{
    static const SVGPropertyInfo* s_propertyInfo = 0;
    if (!s_propertyInfo) {
        s_propertyInfo = new SVGPropertyInfo(AnimatedPoints,
                                             PropertyIsReadWrite,
                                             SVGNames::pointsAttr,
                                             SVGNames::pointsAttr.localName(),
                                             &SVGPolyElement::synchronizePoints,
                                             &SVGPolyElement::lookupOrCreatePointsWrapper);
    }
    return s_propertyInfo;
}

// Animated property definitions
DEFINE_ANIMATED_BOOLEAN(SVGPolyElement, SVGNames::externalResourcesRequiredAttr, ExternalResourcesRequired, externalResourcesRequired)

BEGIN_REGISTER_ANIMATED_PROPERTIES(SVGPolyElement)
    REGISTER_LOCAL_ANIMATED_PROPERTY(points)
    REGISTER_LOCAL_ANIMATED_PROPERTY(externalResourcesRequired)
    REGISTER_PARENT_ANIMATED_PROPERTIES(SVGGraphicsElement)
END_REGISTER_ANIMATED_PROPERTIES

SVGPolyElement::SVGPolyElement(const QualifiedName& tagName, Document* document)
    : SVGGraphicsElement(tagName, document)
{
    registerAnimatedPropertiesForSVGPolyElement();    
}

bool SVGPolyElement::isSupportedAttribute(const QualifiedName& attrName)
{
    DEFINE_STATIC_LOCAL(HashSet<QualifiedName>, supportedAttributes, ());
    if (supportedAttributes.isEmpty()) {
        SVGLangSpace::addSupportedAttributes(supportedAttributes);
        SVGExternalResourcesRequired::addSupportedAttributes(supportedAttributes);
        supportedAttributes.add(SVGNames::pointsAttr);
    }
    return supportedAttributes.contains<SVGAttributeHashTranslator>(attrName);
}

void SVGPolyElement::parseAttribute(const QualifiedName& name, const AtomicString& value)
{
    if (!isSupportedAttribute(name)) {
        SVGGraphicsElement::parseAttribute(name, value);
        return;
    }

    if (name == SVGNames::pointsAttr) {
        SVGPointList newList;
        if (!pointsListFromSVGData(newList, value))
            document()->accessSVGExtensions()->reportError("Problem parsing points=\"" + value + "\"");

        if (SVGAnimatedProperty* wrapper = SVGAnimatedProperty::lookupWrapper<SVGPolyElement, SVGAnimatedPointList>(this, pointsPropertyInfo()))
            static_cast<SVGAnimatedPointList*>(wrapper)->detachListWrappers(newList.size());

        m_points.value = newList;
        return;
    }

    if (SVGLangSpace::parseAttribute(name, value))
        return;
    if (SVGExternalResourcesRequired::parseAttribute(name, value))
        return;

    ASSERT_NOT_REACHED();
}

void SVGPolyElement::svgAttributeChanged(const QualifiedName& attrName)
{
    if (!isSupportedAttribute(attrName)) {
        SVGGraphicsElement::svgAttributeChanged(attrName);
        return;
    }

    SVGElementInstance::InvalidationGuard invalidationGuard(this);

    RenderSVGShape* renderer = toRenderSVGShape(this->renderer());
    if (!renderer)
        return;

    if (attrName == SVGNames::pointsAttr) {
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

void SVGPolyElement::synchronizePoints(SVGElement* contextElement)
{
    ASSERT(contextElement);
    SVGPolyElement* ownerType = toSVGPolyElement(contextElement);
    if (!ownerType->m_points.shouldSynchronize)
        return;
    ownerType->m_points.synchronize(ownerType, pointsPropertyInfo()->attributeName, ownerType->m_points.value.valueAsString());
}

PassRefPtr<SVGAnimatedProperty> SVGPolyElement::lookupOrCreatePointsWrapper(SVGElement* contextElement)
{
    ASSERT(contextElement);
    SVGPolyElement* ownerType = toSVGPolyElement(contextElement);
    return SVGAnimatedProperty::lookupOrCreateWrapper<SVGPolyElement, SVGAnimatedPointList, SVGPointList>
           (ownerType, pointsPropertyInfo(), ownerType->m_points.value);
}

SVGListPropertyTearOff<SVGPointList>* SVGPolyElement::points()
{
    m_points.shouldSynchronize = true;
    return static_cast<SVGListPropertyTearOff<SVGPointList>*>(static_pointer_cast<SVGAnimatedPointList>(lookupOrCreatePointsWrapper(this))->baseVal());
}

SVGListPropertyTearOff<SVGPointList>* SVGPolyElement::animatedPoints()
{
    m_points.shouldSynchronize = true;
    return static_cast<SVGListPropertyTearOff<SVGPointList>*>(static_pointer_cast<SVGAnimatedPointList>(lookupOrCreatePointsWrapper(this))->animVal());
}

}

#endif // ENABLE(SVG)
