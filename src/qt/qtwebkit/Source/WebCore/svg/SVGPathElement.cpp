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
#include "SVGPathElement.h"

#include "Attribute.h"
#include "RenderSVGPath.h"
#include "RenderSVGResource.h"
#include "SVGElementInstance.h"
#include "SVGMPathElement.h"
#include "SVGNames.h"
#include "SVGPathSegArcAbs.h"
#include "SVGPathSegArcRel.h"
#include "SVGPathSegClosePath.h"
#include "SVGPathSegCurvetoCubicAbs.h"
#include "SVGPathSegCurvetoCubicRel.h"
#include "SVGPathSegCurvetoCubicSmoothAbs.h"
#include "SVGPathSegCurvetoCubicSmoothRel.h"
#include "SVGPathSegCurvetoQuadraticAbs.h"
#include "SVGPathSegCurvetoQuadraticRel.h"
#include "SVGPathSegCurvetoQuadraticSmoothAbs.h"
#include "SVGPathSegCurvetoQuadraticSmoothRel.h"
#include "SVGPathSegLinetoAbs.h"
#include "SVGPathSegLinetoHorizontalAbs.h"
#include "SVGPathSegLinetoHorizontalRel.h"
#include "SVGPathSegLinetoRel.h"
#include "SVGPathSegLinetoVerticalAbs.h"
#include "SVGPathSegLinetoVerticalRel.h"
#include "SVGPathSegList.h"
#include "SVGPathSegListBuilder.h"
#include "SVGPathSegListPropertyTearOff.h"
#include "SVGPathSegMovetoAbs.h"
#include "SVGPathSegMovetoRel.h"
#include "SVGPathUtilities.h"
#include "SVGSVGElement.h"

namespace WebCore {

// Define custom animated property 'd'.
const SVGPropertyInfo* SVGPathElement::dPropertyInfo()
{
    static const SVGPropertyInfo* s_propertyInfo = 0;
    if (!s_propertyInfo) {
        s_propertyInfo = new SVGPropertyInfo(AnimatedPath,
                                             PropertyIsReadWrite,
                                             SVGNames::dAttr,
                                             SVGNames::dAttr.localName(),
                                             &SVGPathElement::synchronizeD,
                                             &SVGPathElement::lookupOrCreateDWrapper);
    }
    return s_propertyInfo;
}

// Animated property definitions
DEFINE_ANIMATED_NUMBER(SVGPathElement, SVGNames::pathLengthAttr, PathLength, pathLength)
DEFINE_ANIMATED_BOOLEAN(SVGPathElement, SVGNames::externalResourcesRequiredAttr, ExternalResourcesRequired, externalResourcesRequired)

BEGIN_REGISTER_ANIMATED_PROPERTIES(SVGPathElement)
    REGISTER_LOCAL_ANIMATED_PROPERTY(d)
    REGISTER_LOCAL_ANIMATED_PROPERTY(pathLength)
    REGISTER_LOCAL_ANIMATED_PROPERTY(externalResourcesRequired)
    REGISTER_PARENT_ANIMATED_PROPERTIES(SVGGraphicsElement)
END_REGISTER_ANIMATED_PROPERTIES

inline SVGPathElement::SVGPathElement(const QualifiedName& tagName, Document* document)
    : SVGGraphicsElement(tagName, document)
    , m_pathByteStream(SVGPathByteStream::create())
    , m_pathSegList(PathSegUnalteredRole)
    , m_isAnimValObserved(false)
{
    ASSERT(hasTagName(SVGNames::pathTag));
    registerAnimatedPropertiesForSVGPathElement();
}

PassRefPtr<SVGPathElement> SVGPathElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new SVGPathElement(tagName, document));
}

float SVGPathElement::getTotalLength()
{
    float totalLength = 0;
    getTotalLengthOfSVGPathByteStream(pathByteStream(), totalLength);
    return totalLength;
}

SVGPoint SVGPathElement::getPointAtLength(float length)
{
    SVGPoint point;
    getPointAtLengthOfSVGPathByteStream(pathByteStream(), length, point);
    return point;
}

unsigned SVGPathElement::getPathSegAtLength(float length)
{
    unsigned pathSeg = 0;
    getSVGPathSegAtLengthFromSVGPathByteStream(pathByteStream(), length, pathSeg);
    return pathSeg;
}

PassRefPtr<SVGPathSegClosePath> SVGPathElement::createSVGPathSegClosePath(SVGPathSegRole role)
{
    return SVGPathSegClosePath::create(this, role);
}

PassRefPtr<SVGPathSegMovetoAbs> SVGPathElement::createSVGPathSegMovetoAbs(float x, float y, SVGPathSegRole role)
{
    return SVGPathSegMovetoAbs::create(this, role, x, y);
}

PassRefPtr<SVGPathSegMovetoRel> SVGPathElement::createSVGPathSegMovetoRel(float x, float y, SVGPathSegRole role)
{
    return SVGPathSegMovetoRel::create(this, role, x, y);
}

PassRefPtr<SVGPathSegLinetoAbs> SVGPathElement::createSVGPathSegLinetoAbs(float x, float y, SVGPathSegRole role)
{
    return SVGPathSegLinetoAbs::create(this, role, x, y);
}

PassRefPtr<SVGPathSegLinetoRel> SVGPathElement::createSVGPathSegLinetoRel(float x, float y, SVGPathSegRole role)
{
    return SVGPathSegLinetoRel::create(this, role, x, y);
}

PassRefPtr<SVGPathSegCurvetoCubicAbs> SVGPathElement::createSVGPathSegCurvetoCubicAbs(float x, float y, float x1, float y1, float x2, float y2, SVGPathSegRole role)
{
    return SVGPathSegCurvetoCubicAbs::create(this, role, x, y, x1, y1, x2, y2);
}

PassRefPtr<SVGPathSegCurvetoCubicRel> SVGPathElement::createSVGPathSegCurvetoCubicRel(float x, float y, float x1, float y1, float x2, float y2, SVGPathSegRole role)
{
    return SVGPathSegCurvetoCubicRel::create(this, role, x, y, x1, y1, x2, y2);
}

PassRefPtr<SVGPathSegCurvetoQuadraticAbs> SVGPathElement::createSVGPathSegCurvetoQuadraticAbs(float x, float y, float x1, float y1, SVGPathSegRole role)
{
    return SVGPathSegCurvetoQuadraticAbs::create(this, role, x, y, x1, y1);
}

PassRefPtr<SVGPathSegCurvetoQuadraticRel> SVGPathElement::createSVGPathSegCurvetoQuadraticRel(float x, float y, float x1, float y1, SVGPathSegRole role)
{
    return SVGPathSegCurvetoQuadraticRel::create(this, role, x, y, x1, y1);
}

PassRefPtr<SVGPathSegArcAbs> SVGPathElement::createSVGPathSegArcAbs(float x, float y, float r1, float r2, float angle, bool largeArcFlag, bool sweepFlag, SVGPathSegRole role)
{
    return SVGPathSegArcAbs::create(this, role, x, y, r1, r2, angle, largeArcFlag, sweepFlag);
}

PassRefPtr<SVGPathSegArcRel> SVGPathElement::createSVGPathSegArcRel(float x, float y, float r1, float r2, float angle, bool largeArcFlag, bool sweepFlag, SVGPathSegRole role)
{
    return SVGPathSegArcRel::create(this, role, x, y, r1, r2, angle, largeArcFlag, sweepFlag);
}

PassRefPtr<SVGPathSegLinetoHorizontalAbs> SVGPathElement::createSVGPathSegLinetoHorizontalAbs(float x, SVGPathSegRole role)
{
    return SVGPathSegLinetoHorizontalAbs::create(this, role, x);
}

PassRefPtr<SVGPathSegLinetoHorizontalRel> SVGPathElement::createSVGPathSegLinetoHorizontalRel(float x, SVGPathSegRole role)
{
    return SVGPathSegLinetoHorizontalRel::create(this, role, x);
}

PassRefPtr<SVGPathSegLinetoVerticalAbs> SVGPathElement::createSVGPathSegLinetoVerticalAbs(float y, SVGPathSegRole role)
{
    return SVGPathSegLinetoVerticalAbs::create(this, role, y);
}

PassRefPtr<SVGPathSegLinetoVerticalRel> SVGPathElement::createSVGPathSegLinetoVerticalRel(float y, SVGPathSegRole role)
{
    return SVGPathSegLinetoVerticalRel::create(this, role, y);
}

PassRefPtr<SVGPathSegCurvetoCubicSmoothAbs> SVGPathElement::createSVGPathSegCurvetoCubicSmoothAbs(float x, float y, float x2, float y2, SVGPathSegRole role)
{
    return SVGPathSegCurvetoCubicSmoothAbs::create(this, role, x, y, x2, y2);
}

PassRefPtr<SVGPathSegCurvetoCubicSmoothRel> SVGPathElement::createSVGPathSegCurvetoCubicSmoothRel(float x, float y, float x2, float y2, SVGPathSegRole role)
{
    return SVGPathSegCurvetoCubicSmoothRel::create(this, role, x, y, x2, y2);
}

PassRefPtr<SVGPathSegCurvetoQuadraticSmoothAbs> SVGPathElement::createSVGPathSegCurvetoQuadraticSmoothAbs(float x, float y, SVGPathSegRole role)
{
    return SVGPathSegCurvetoQuadraticSmoothAbs::create(this, role, x, y);
}

PassRefPtr<SVGPathSegCurvetoQuadraticSmoothRel> SVGPathElement::createSVGPathSegCurvetoQuadraticSmoothRel(float x, float y, SVGPathSegRole role)
{
    return SVGPathSegCurvetoQuadraticSmoothRel::create(this, role, x, y);
}

bool SVGPathElement::isSupportedAttribute(const QualifiedName& attrName)
{
    DEFINE_STATIC_LOCAL(HashSet<QualifiedName>, supportedAttributes, ());
    if (supportedAttributes.isEmpty()) {
        SVGLangSpace::addSupportedAttributes(supportedAttributes);
        SVGExternalResourcesRequired::addSupportedAttributes(supportedAttributes);
        supportedAttributes.add(SVGNames::dAttr);
        supportedAttributes.add(SVGNames::pathLengthAttr);
    }
    return supportedAttributes.contains<SVGAttributeHashTranslator>(attrName);
}

void SVGPathElement::parseAttribute(const QualifiedName& name, const AtomicString& value)
{
    if (!isSupportedAttribute(name)) {
        SVGGraphicsElement::parseAttribute(name, value);
        return;
    }

    if (name == SVGNames::dAttr) {
        if (!buildSVGPathByteStreamFromString(value, m_pathByteStream.get(), UnalteredParsing))
            document()->accessSVGExtensions()->reportError("Problem parsing d=\"" + value + "\"");
        return;
    }

    if (name == SVGNames::pathLengthAttr) {
        setPathLengthBaseValue(value.toFloat());
        if (pathLengthBaseValue() < 0)
            document()->accessSVGExtensions()->reportError("A negative value for path attribute <pathLength> is not allowed");
        return;
    }

    if (SVGLangSpace::parseAttribute(name, value))
        return;
    if (SVGExternalResourcesRequired::parseAttribute(name, value))
        return;

    ASSERT_NOT_REACHED();
}

void SVGPathElement::svgAttributeChanged(const QualifiedName& attrName)
{
    if (!isSupportedAttribute(attrName)) {
        SVGGraphicsElement::svgAttributeChanged(attrName);
        return;
    }

    SVGElementInstance::InvalidationGuard invalidationGuard(this);

    RenderSVGPath* renderer = toRenderSVGPath(this->renderer());

    if (attrName == SVGNames::dAttr) {
        if (m_pathSegList.shouldSynchronize && !SVGAnimatedProperty::lookupWrapper<SVGPathElement, SVGAnimatedPathSegListPropertyTearOff>(this, dPropertyInfo())->isAnimating()) {
            SVGPathSegList newList(PathSegUnalteredRole);
            buildSVGPathSegListFromByteStream(m_pathByteStream.get(), this, newList, UnalteredParsing);
            m_pathSegList.value = newList;
        }

        if (renderer)
            renderer->setNeedsShapeUpdate();

        invalidateMPathDependencies();
    }

    if (renderer)
        RenderSVGResource::markForLayoutAndParentResourceInvalidation(renderer);
}

void SVGPathElement::invalidateMPathDependencies()
{
    // <mpath> can only reference <path> but this dependency is not handled in
    // markForLayoutAndParentResourceInvalidation so we update any mpath dependencies manually.
    ASSERT(document());
    if (HashSet<SVGElement*>* dependencies = document()->accessSVGExtensions()->setOfElementsReferencingTarget(this)) {
        HashSet<SVGElement*>::iterator end = dependencies->end();
        for (HashSet<SVGElement*>::iterator it = dependencies->begin(); it != end; ++it) {
            if ((*it)->hasTagName(SVGNames::mpathTag))
                static_cast<SVGMPathElement*>(*it)->targetPathChanged();
        }
    }
}

Node::InsertionNotificationRequest SVGPathElement::insertedInto(ContainerNode* rootParent)
{
    SVGGraphicsElement::insertedInto(rootParent);
    invalidateMPathDependencies();
    return InsertionDone;
}

void SVGPathElement::removedFrom(ContainerNode* rootParent)
{
    SVGGraphicsElement::removedFrom(rootParent);
    invalidateMPathDependencies();
}

SVGPathByteStream* SVGPathElement::pathByteStream() const
{
    SVGAnimatedProperty* property = SVGAnimatedProperty::lookupWrapper<SVGPathElement, SVGAnimatedPathSegListPropertyTearOff>(this, dPropertyInfo());
    if (!property || !property->isAnimating())
        return m_pathByteStream.get();
    return static_cast<SVGAnimatedPathSegListPropertyTearOff*>(property)->animatedPathByteStream();
}

PassRefPtr<SVGAnimatedProperty> SVGPathElement::lookupOrCreateDWrapper(SVGElement* contextElement)
{
    ASSERT(contextElement);
    SVGPathElement* ownerType = toSVGPathElement(contextElement);

    if (SVGAnimatedProperty* property = SVGAnimatedProperty::lookupWrapper<SVGPathElement, SVGAnimatedPathSegListPropertyTearOff>(ownerType, dPropertyInfo()))
        return property;

    // Build initial SVGPathSegList.
    buildSVGPathSegListFromByteStream(ownerType->m_pathByteStream.get(), ownerType, ownerType->m_pathSegList.value, UnalteredParsing);

    return SVGAnimatedProperty::lookupOrCreateWrapper<SVGPathElement, SVGAnimatedPathSegListPropertyTearOff, SVGPathSegList>
           (ownerType, dPropertyInfo(), ownerType->m_pathSegList.value);
}

void SVGPathElement::synchronizeD(SVGElement* contextElement)
{
    ASSERT(contextElement);
    SVGPathElement* ownerType = toSVGPathElement(contextElement);
    if (!ownerType->m_pathSegList.shouldSynchronize)
        return;
    ownerType->m_pathSegList.synchronize(ownerType, dPropertyInfo()->attributeName, ownerType->m_pathSegList.value.valueAsString());
}

SVGPathSegListPropertyTearOff* SVGPathElement::pathSegList()
{
    m_pathSegList.shouldSynchronize = true;
    return static_cast<SVGPathSegListPropertyTearOff*>(static_pointer_cast<SVGAnimatedPathSegListPropertyTearOff>(lookupOrCreateDWrapper(this))->baseVal());
}

SVGPathSegListPropertyTearOff* SVGPathElement::normalizedPathSegList()
{
    // FIXME: https://bugs.webkit.org/show_bug.cgi?id=15412 - Implement normalized path segment lists!
    return 0;
}

SVGPathSegListPropertyTearOff* SVGPathElement::animatedPathSegList()
{
    m_pathSegList.shouldSynchronize = true;
    m_isAnimValObserved = true;
    return static_cast<SVGPathSegListPropertyTearOff*>(static_pointer_cast<SVGAnimatedPathSegListPropertyTearOff>(lookupOrCreateDWrapper(this))->animVal());
}

SVGPathSegListPropertyTearOff* SVGPathElement::animatedNormalizedPathSegList()
{
    // FIXME: https://bugs.webkit.org/show_bug.cgi?id=15412 - Implement normalized path segment lists!
    return 0;
}

void SVGPathElement::pathSegListChanged(SVGPathSegRole role, ListModification listModification)
{
    switch (role) {
    case PathSegNormalizedRole:
        // FIXME: https://bugs.webkit.org/show_bug.cgi?id=15412 - Implement normalized path segment lists!
        break;
    case PathSegUnalteredRole:
        if (listModification == ListModificationAppend) {
            ASSERT(!m_pathSegList.value.isEmpty());
            appendSVGPathByteStreamFromSVGPathSeg(m_pathSegList.value.last(), m_pathByteStream.get(), UnalteredParsing);
        } else
            buildSVGPathByteStreamFromSVGPathSegList(m_pathSegList.value, m_pathByteStream.get(), UnalteredParsing);
        break;
    case PathSegUndefinedRole:
        return;
    }

    invalidateSVGAttributes();
    
    RenderSVGPath* renderer = toRenderSVGPath(this->renderer());
    if (!renderer)
        return;

    renderer->setNeedsShapeUpdate();
    RenderSVGResource::markForLayoutAndParentResourceInvalidation(renderer);
}

FloatRect SVGPathElement::getBBox(StyleUpdateStrategy styleUpdateStrategy)
{
    if (styleUpdateStrategy == AllowStyleUpdate)
        this->document()->updateLayoutIgnorePendingStylesheets();

    RenderSVGPath* renderer = toRenderSVGPath(this->renderer());

    // FIXME: Eventually we should support getBBox for detached elements.
    if (!renderer)
        return FloatRect();

    return renderer->path().boundingRect();
}

RenderObject* SVGPathElement::createRenderer(RenderArena* arena, RenderStyle*)
{
    // By default, any subclass is expected to do path-based drawing
    return new (arena) RenderSVGPath(this);
}

}

#endif // ENABLE(SVG)
