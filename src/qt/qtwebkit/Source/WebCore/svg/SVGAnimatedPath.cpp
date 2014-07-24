/*
 * Copyright (C) Research In Motion Limited 2011, 2012. All rights reserved.
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
#include "SVGAnimatedPath.h"

#include "SVGAnimateElement.h"
#include "SVGAnimatedPathSegListPropertyTearOff.h"
#include "SVGPathUtilities.h"

namespace WebCore {

SVGAnimatedPathAnimator::SVGAnimatedPathAnimator(SVGAnimationElement* animationElement, SVGElement* contextElement)
    : SVGAnimatedTypeAnimator(AnimatedPath, animationElement, contextElement)
{
}

PassOwnPtr<SVGAnimatedType> SVGAnimatedPathAnimator::constructFromString(const String& string)
{
    OwnPtr<SVGPathByteStream> byteStream = SVGPathByteStream::create();
    buildSVGPathByteStreamFromString(string, byteStream.get(), UnalteredParsing);
    return SVGAnimatedType::createPath(byteStream.release());
}

PassOwnPtr<SVGAnimatedType> SVGAnimatedPathAnimator::startAnimValAnimation(const SVGElementAnimatedPropertyList& animatedTypes)
{
    ASSERT(animatedTypes.size() >= 1);
    SVGAnimatedPathSegListPropertyTearOff* property = castAnimatedPropertyToActualType<SVGAnimatedPathSegListPropertyTearOff>(animatedTypes[0].properties[0].get());
    const SVGPathSegList& baseValue = property->currentBaseValue();

    // Build initial path byte stream.
    OwnPtr<SVGPathByteStream> byteStream = SVGPathByteStream::create();
    buildSVGPathByteStreamFromSVGPathSegList(baseValue, byteStream.get(), UnalteredParsing);

    Vector<RefPtr<SVGAnimatedPathSegListPropertyTearOff> > result;

    SVGElementAnimatedPropertyList::const_iterator end = animatedTypes.end();
    for (SVGElementAnimatedPropertyList::const_iterator it = animatedTypes.begin(); it != end; ++it)
        result.append(castAnimatedPropertyToActualType<SVGAnimatedPathSegListPropertyTearOff>(it->properties[0].get()));

    SVGElementInstance::InstanceUpdateBlocker blocker(property->contextElement());

    size_t resultSize = result.size();
    for (size_t i = 0; i < resultSize; ++i)
        result[i]->animationStarted(byteStream.get(), &baseValue);

    return SVGAnimatedType::createPath(byteStream.release());
}

void SVGAnimatedPathAnimator::stopAnimValAnimation(const SVGElementAnimatedPropertyList& animatedTypes)
{
    stopAnimValAnimationForType<SVGAnimatedPathSegListPropertyTearOff>(animatedTypes);
}

void SVGAnimatedPathAnimator::resetAnimValToBaseVal(const SVGElementAnimatedPropertyList& animatedTypes, SVGAnimatedType* type)
{
    ASSERT(animatedTypes.size() >= 1);
    ASSERT(type);
    ASSERT(type->type() == m_type);
    const SVGPathSegList& baseValue = castAnimatedPropertyToActualType<SVGAnimatedPathSegListPropertyTearOff>(animatedTypes[0].properties[0].get())->currentBaseValue();
    buildSVGPathByteStreamFromSVGPathSegList(baseValue, type->path(), UnalteredParsing);
}

void SVGAnimatedPathAnimator::animValWillChange(const SVGElementAnimatedPropertyList& animatedTypes)
{
    animValWillChangeForType<SVGAnimatedPathSegListPropertyTearOff>(animatedTypes);
}

void SVGAnimatedPathAnimator::animValDidChange(const SVGElementAnimatedPropertyList& animatedTypes)
{
    animValDidChangeForType<SVGAnimatedPathSegListPropertyTearOff>(animatedTypes);
}

void SVGAnimatedPathAnimator::addAnimatedTypes(SVGAnimatedType* from, SVGAnimatedType* to)
{
    ASSERT(from->type() == AnimatedPath);
    ASSERT(from->type() == to->type());

    SVGPathByteStream* fromPath = from->path();
    SVGPathByteStream* toPath = to->path();
    unsigned fromPathSize = fromPath->size();
    if (!fromPathSize || fromPathSize != toPath->size())
        return;
    addToSVGPathByteStream(toPath, fromPath);
}

void SVGAnimatedPathAnimator::calculateAnimatedValue(float percentage, unsigned repeatCount, SVGAnimatedType* from, SVGAnimatedType* to, SVGAnimatedType* toAtEndOfDuration, SVGAnimatedType* animated)
{
    ASSERT(m_animationElement);
    ASSERT(m_contextElement);

    SVGPathByteStream* fromPath = from->path();
    SVGPathByteStream* toPath = to->path();
    SVGPathByteStream* toAtEndOfDurationPath = toAtEndOfDuration->path();
    SVGPathByteStream* animatedPath = animated->path();

    OwnPtr<SVGPathByteStream> underlyingPath;
    bool isToAnimation = m_animationElement->animationMode() == ToAnimation;
    if (isToAnimation) {
        underlyingPath = animatedPath->copy();
        fromPath = underlyingPath.get();
    }

    // Cache the current animated value before the buildAnimatedSVGPathByteStream() clears animatedPath.
    OwnPtr<SVGPathByteStream> lastAnimatedPath;
    if (!fromPath->size() || (m_animationElement->isAdditive() && !isToAnimation))
        lastAnimatedPath = animatedPath->copy();

    // Pass false to 'resizeAnimatedListIfNeeded' here, as the path animation is not a regular Vector<SVGXXX> type, but a SVGPathByteStream, that works differently.
    if (!m_animationElement->adjustFromToListValues<SVGPathByteStream>(*fromPath, *toPath, *animatedPath, percentage, false))
        return;

    buildAnimatedSVGPathByteStream(fromPath, toPath, animatedPath, percentage);

    // Handle additive='sum'.
    if (lastAnimatedPath)
        addToSVGPathByteStream(animatedPath, lastAnimatedPath.get());

    // Handle accumulate='sum'.
    if (m_animationElement->isAccumulated() && repeatCount)
        addToSVGPathByteStream(animatedPath, toAtEndOfDurationPath, repeatCount);
}
   
float SVGAnimatedPathAnimator::calculateDistance(const String&, const String&)
{
    // FIXME: Support paced animations.
    return -1;
}

}

#endif // ENABLE(SVG)
