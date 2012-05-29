/*
 * Copyright (C) 2004, 2005, 2008 Nikolas Zimmermann <zimmermann@kde.org>
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

#include "config.h"

#if ENABLE(SVG)
#include "SVGStyledTransformableElement.h"

#include "AffineTransform.h"
#include "Attribute.h"
#include "RenderSVGPath.h"
#include "RenderSVGResource.h"
#include "SVGNames.h"

namespace WebCore {

// Animated property definitions
DEFINE_ANIMATED_TRANSFORM_LIST(SVGStyledTransformableElement, SVGNames::transformAttr, Transform, transform)

SVGStyledTransformableElement::SVGStyledTransformableElement(const QualifiedName& tagName, Document* document)
    : SVGStyledLocatableElement(tagName, document)
{
}

SVGStyledTransformableElement::~SVGStyledTransformableElement()
{
}

AffineTransform SVGStyledTransformableElement::getCTM(StyleUpdateStrategy styleUpdateStrategy) const
{
    return SVGLocatable::computeCTM(this, SVGLocatable::NearestViewportScope, styleUpdateStrategy);
}

AffineTransform SVGStyledTransformableElement::getScreenCTM(StyleUpdateStrategy styleUpdateStrategy) const
{
    return SVGLocatable::computeCTM(this, SVGLocatable::ScreenScope, styleUpdateStrategy);
}

AffineTransform SVGStyledTransformableElement::animatedLocalTransform() const
{
    AffineTransform matrix;
    transform().concatenate(matrix);
    if (m_supplementalTransform)
        matrix *= *m_supplementalTransform;
    return matrix;
}

AffineTransform* SVGStyledTransformableElement::supplementalTransform()
{
    if (!m_supplementalTransform)
        m_supplementalTransform = adoptPtr(new AffineTransform);
    return m_supplementalTransform.get();
}

void SVGStyledTransformableElement::parseMappedAttribute(Attribute* attr)
{
    if (SVGTransformable::isKnownAttribute(attr->name())) {
        SVGTransformList newList;
        if (!SVGTransformable::parseTransformAttribute(newList, attr->value()))
            newList.clear();
        detachAnimatedTransformListWrappers(newList.size());
        setTransformBaseValue(newList);
    } else 
        SVGStyledLocatableElement::parseMappedAttribute(attr);
}

void SVGStyledTransformableElement::svgAttributeChanged(const QualifiedName& attrName)
{
    SVGStyledLocatableElement::svgAttributeChanged(attrName);

    if (!SVGStyledTransformableElement::isKnownAttribute(attrName))
        return;

    RenderObject* object = renderer();
    if (!object)
        return;

    object->setNeedsTransformUpdate();
    RenderSVGResource::markForLayoutAndParentResourceInvalidation(object);
}

void SVGStyledTransformableElement::synchronizeProperty(const QualifiedName& attrName)
{
    SVGStyledLocatableElement::synchronizeProperty(attrName);

    if (attrName == anyQName() || SVGTransformable::isKnownAttribute(attrName))
        synchronizeTransform();
}

bool SVGStyledTransformableElement::isKnownAttribute(const QualifiedName& attrName)
{
    return SVGTransformable::isKnownAttribute(attrName) || SVGStyledLocatableElement::isKnownAttribute(attrName);
}

SVGElement* SVGStyledTransformableElement::nearestViewportElement() const
{
    return SVGTransformable::nearestViewportElement(this);
}

SVGElement* SVGStyledTransformableElement::farthestViewportElement() const
{
    return SVGTransformable::farthestViewportElement(this);
}

FloatRect SVGStyledTransformableElement::getBBox(StyleUpdateStrategy styleUpdateStrategy) const
{
    return SVGTransformable::getBBox(this, styleUpdateStrategy);
}

RenderObject* SVGStyledTransformableElement::createRenderer(RenderArena* arena, RenderStyle*)
{
    // By default, any subclass is expected to do path-based drawing
    return new (arena) RenderSVGPath(this);
}

void SVGStyledTransformableElement::fillPassedAttributeToPropertyTypeMap(AttributeToPropertyTypeMap& attributeToPropertyTypeMap)
{
    SVGStyledElement::fillPassedAttributeToPropertyTypeMap(attributeToPropertyTypeMap);
    
    attributeToPropertyTypeMap.set(SVGNames::transformAttr, AnimatedTransformList);
}

void SVGStyledTransformableElement::toClipPath(Path& path) const
{
    toPathData(path);
    // FIXME: How do we know the element has done a layout?
    path.transform(animatedLocalTransform());
}

}

#endif // ENABLE(SVG)
