/*
 * Copyright (C) 2007, 2010 Rob Buis <buis@kde.org>
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
#include "SVGViewSpec.h"

#include "Document.h"
#include "SVGAnimatedTransformList.h"
#include "SVGFitToViewBox.h"
#include "SVGNames.h"
#include "SVGParserUtilities.h"
#include "SVGSVGElement.h"
#include "SVGTransformable.h"

namespace WebCore {

// Define custom animated property 'viewBox'.
const SVGPropertyInfo* SVGViewSpec::viewBoxPropertyInfo()
{
    static const SVGPropertyInfo* s_propertyInfo = 0;
    if (!s_propertyInfo) {
        s_propertyInfo = new SVGPropertyInfo(AnimatedRect,
                                             PropertyIsReadOnly,
                                             SVGNames::viewBoxAttr,
                                             viewBoxIdentifier(),
                                             0,
                                             0);
    }
    return s_propertyInfo;
}

// Define custom animated property 'preserveAspectRatio'.
const SVGPropertyInfo* SVGViewSpec::preserveAspectRatioPropertyInfo()
{
    static const SVGPropertyInfo* s_propertyInfo = 0;
    if (!s_propertyInfo) {
        s_propertyInfo = new SVGPropertyInfo(AnimatedPreserveAspectRatio,
                                             PropertyIsReadOnly,
                                             SVGNames::preserveAspectRatioAttr,
                                             preserveAspectRatioIdentifier(),
                                             0,
                                             0);
    }
    return s_propertyInfo;
}


// Define custom non-animated property 'transform'.
const SVGPropertyInfo* SVGViewSpec::transformPropertyInfo()
{
    static const SVGPropertyInfo* s_propertyInfo = 0;
    if (!s_propertyInfo) {
        s_propertyInfo = new SVGPropertyInfo(AnimatedTransformList,
                                             PropertyIsReadOnly,
                                             SVGNames::transformAttr,
                                             transformIdentifier(),
                                             0,
                                             0);
    }
    return s_propertyInfo;
}

SVGViewSpec::SVGViewSpec(SVGElement* contextElement)
    : m_contextElement(contextElement)
    , m_zoomAndPan(SVGZoomAndPanMagnify)
{
    ASSERT(m_contextElement);
}

const AtomicString& SVGViewSpec::viewBoxIdentifier()
{
    DEFINE_STATIC_LOCAL(AtomicString, s_identifier, ("SVGViewSpecViewBoxAttribute", AtomicString::ConstructFromLiteral));
    return s_identifier;
}

const AtomicString& SVGViewSpec::preserveAspectRatioIdentifier()
{
    DEFINE_STATIC_LOCAL(AtomicString, s_identifier, ("SVGViewSpecPreserveAspectRatioAttribute", AtomicString::ConstructFromLiteral));
    return s_identifier;
}

const AtomicString& SVGViewSpec::transformIdentifier()
{
    DEFINE_STATIC_LOCAL(AtomicString, s_identifier, ("SVGViewSpecTransformAttribute", AtomicString::ConstructFromLiteral));
    return s_identifier;
}

void SVGViewSpec::setZoomAndPan(unsigned short, ExceptionCode& ec)
{
    // SVGViewSpec and all of its content is read-only.
    ec = NO_MODIFICATION_ALLOWED_ERR;
}

void SVGViewSpec::setTransformString(const String& transform)
{
    if (!m_contextElement)
        return;

    SVGTransformList newList;
    newList.parse(transform);

    if (SVGAnimatedProperty* wrapper = SVGAnimatedProperty::lookupWrapper<SVGElement, SVGAnimatedTransformList>(m_contextElement, transformPropertyInfo()))
        static_cast<SVGAnimatedTransformList*>(wrapper)->detachListWrappers(newList.size());

    m_transform = newList;
}

String SVGViewSpec::transformString() const
{
    return SVGPropertyTraits<SVGTransformList>::toString(m_transform);
}

String SVGViewSpec::viewBoxString() const
{
    return SVGPropertyTraits<FloatRect>::toString(viewBoxBaseValue());
}

String SVGViewSpec::preserveAspectRatioString() const
{
    return SVGPropertyTraits<SVGPreserveAspectRatio>::toString(preserveAspectRatioBaseValue());
}

SVGElement* SVGViewSpec::viewTarget() const
{
    if (!m_contextElement)
        return 0;
    Element* element = m_contextElement->treeScope()->getElementById(m_viewTargetString);
    if (!element || !element->isSVGElement())
        return 0;
    return toSVGElement(element);
}

SVGTransformListPropertyTearOff* SVGViewSpec::transform()
{
    if (!m_contextElement)
        return 0;
    // Return the animVal here, as its readonly by default - which is exactly what we want here.
    return static_cast<SVGTransformListPropertyTearOff*>(static_pointer_cast<SVGAnimatedTransformList>(lookupOrCreateTransformWrapper(this))->animVal());
}

PassRefPtr<SVGAnimatedRect> SVGViewSpec::viewBoxAnimated()
{
    if (!m_contextElement)
        return 0;
    return static_pointer_cast<SVGAnimatedRect>(lookupOrCreateViewBoxWrapper(this));
}

PassRefPtr<SVGAnimatedPreserveAspectRatio> SVGViewSpec::preserveAspectRatioAnimated()
{
    if (!m_contextElement)
        return 0;
    return static_pointer_cast<SVGAnimatedPreserveAspectRatio>(lookupOrCreatePreserveAspectRatioWrapper(this));
}

PassRefPtr<SVGAnimatedProperty> SVGViewSpec::lookupOrCreateViewBoxWrapper(SVGViewSpec* ownerType)
{
    ASSERT(ownerType);
    ASSERT(ownerType->contextElement());
    return SVGAnimatedProperty::lookupOrCreateWrapper<SVGElement, SVGAnimatedRect, FloatRect>(ownerType->contextElement(), viewBoxPropertyInfo(), ownerType->m_viewBox);
}

PassRefPtr<SVGAnimatedProperty> SVGViewSpec::lookupOrCreatePreserveAspectRatioWrapper(SVGViewSpec* ownerType)
{
    ASSERT(ownerType);
    ASSERT(ownerType->contextElement());
    return SVGAnimatedProperty::lookupOrCreateWrapper<SVGElement, SVGAnimatedPreserveAspectRatio, SVGPreserveAspectRatio>(ownerType->contextElement(), preserveAspectRatioPropertyInfo(), ownerType->m_preserveAspectRatio);
}

PassRefPtr<SVGAnimatedProperty> SVGViewSpec::lookupOrCreateTransformWrapper(SVGViewSpec* ownerType)
{
    ASSERT(ownerType);
    ASSERT(ownerType->contextElement());
    return SVGAnimatedProperty::lookupOrCreateWrapper<SVGElement, SVGAnimatedTransformList, SVGTransformList>(ownerType->contextElement(), transformPropertyInfo(), ownerType->m_transform);
}

void SVGViewSpec::reset()
{
    m_zoomAndPan = SVGZoomAndPanMagnify;
    m_transform.clear();
    m_viewBox = FloatRect();
    m_preserveAspectRatio = SVGPreserveAspectRatio();
    m_viewTargetString = emptyString();
}

static const UChar svgViewSpec[] = {'s', 'v', 'g', 'V', 'i', 'e', 'w'};
static const UChar viewBoxSpec[] = {'v', 'i', 'e', 'w', 'B', 'o', 'x'};
static const UChar preserveAspectRatioSpec[] = {'p', 'r', 'e', 's', 'e', 'r', 'v', 'e', 'A', 's', 'p', 'e', 'c', 't', 'R', 'a', 't', 'i', 'o'};
static const UChar transformSpec[] = {'t', 'r', 'a', 'n', 's', 'f', 'o', 'r', 'm'};
static const UChar zoomAndPanSpec[] = {'z', 'o', 'o', 'm', 'A', 'n', 'd', 'P', 'a', 'n'};
static const UChar viewTargetSpec[] =  {'v', 'i', 'e', 'w', 'T', 'a', 'r', 'g', 'e', 't'};

bool SVGViewSpec::parseViewSpec(const String& viewSpec)
{
    const UChar* currViewSpec = viewSpec.characters();
    const UChar* end = currViewSpec + viewSpec.length();

    if (currViewSpec >= end || !m_contextElement)
        return false;

    if (!skipString(currViewSpec, end, svgViewSpec, WTF_ARRAY_LENGTH(svgViewSpec)))
        return false;

    if (currViewSpec >= end || *currViewSpec != '(')
        return false;
    currViewSpec++;

    while (currViewSpec < end && *currViewSpec != ')') {
        if (*currViewSpec == 'v') {
            if (skipString(currViewSpec, end, viewBoxSpec, WTF_ARRAY_LENGTH(viewBoxSpec))) {
                if (currViewSpec >= end || *currViewSpec != '(')
                    return false;
                currViewSpec++;
                FloatRect viewBox;
                if (!SVGFitToViewBox::parseViewBox(m_contextElement->document(), currViewSpec, end, viewBox, false))
                    return false;
                setViewBoxBaseValue(viewBox);
                if (currViewSpec >= end || *currViewSpec != ')')
                    return false;
                currViewSpec++;
            } else if (skipString(currViewSpec, end, viewTargetSpec, WTF_ARRAY_LENGTH(viewTargetSpec))) {
                if (currViewSpec >= end || *currViewSpec != '(')
                    return false;
                const UChar* viewTargetStart = ++currViewSpec;
                while (currViewSpec < end && *currViewSpec != ')')
                    currViewSpec++;
                if (currViewSpec >= end)
                    return false;
                setViewTargetString(String(viewTargetStart, currViewSpec - viewTargetStart));
                currViewSpec++;
            } else
                return false;
        } else if (*currViewSpec == 'z') {
            if (!skipString(currViewSpec, end, zoomAndPanSpec, WTF_ARRAY_LENGTH(zoomAndPanSpec)))
                return false;
            if (currViewSpec >= end || *currViewSpec != '(')
                return false;
            currViewSpec++;
            if (!parseZoomAndPan(currViewSpec, end, m_zoomAndPan))
                return false;
            if (currViewSpec >= end || *currViewSpec != ')')
                return false;
            currViewSpec++;
        } else if (*currViewSpec == 'p') {
            if (!skipString(currViewSpec, end, preserveAspectRatioSpec, WTF_ARRAY_LENGTH(preserveAspectRatioSpec)))
                return false;
            if (currViewSpec >= end || *currViewSpec != '(')
                return false;
            currViewSpec++;
            SVGPreserveAspectRatio preserveAspectRatio;
            if (!preserveAspectRatio.parse(currViewSpec, end, false))
                return false;
            setPreserveAspectRatioBaseValue(preserveAspectRatio);
            if (currViewSpec >= end || *currViewSpec != ')')
                return false;
            currViewSpec++;
        } else if (*currViewSpec == 't') {
            if (!skipString(currViewSpec, end, transformSpec, WTF_ARRAY_LENGTH(transformSpec)))
                return false;
            if (currViewSpec >= end || *currViewSpec != '(')
                return false;
            currViewSpec++;
            SVGTransformable::parseTransformAttribute(m_transform, currViewSpec, end, SVGTransformable::DoNotClearList);
            if (currViewSpec >= end || *currViewSpec != ')')
                return false;
            currViewSpec++;
        } else
            return false;

        if (currViewSpec < end && *currViewSpec == ';')
            currViewSpec++;
    }
    
    if (currViewSpec >= end || *currViewSpec != ')')
        return false;

    return true;
}

}

#endif // ENABLE(SVG)
