/*
 * Copyright (C) Research In Motion Limited 2011. All rights reserved.
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

#ifndef SVGPropertyInfo_h
#define SVGPropertyInfo_h

#if ENABLE(SVG)
#include "QualifiedName.h"
#include <wtf/PassRefPtr.h>

namespace WebCore {

class SVGAnimatedProperty;
class SVGElement;

enum AnimatedPropertyState {
    PropertyIsReadWrite,
    PropertyIsReadOnly
};

enum AnimatedPropertyType {
    AnimatedAngle,
    AnimatedBoolean,
    AnimatedColor,
    AnimatedEnumeration,
    AnimatedInteger,
    AnimatedIntegerOptionalInteger,
    AnimatedLength,
    AnimatedLengthList,
    AnimatedNumber,
    AnimatedNumberList,
    AnimatedNumberOptionalNumber,
    AnimatedPath,
    AnimatedPoints,
    AnimatedPreserveAspectRatio,
    AnimatedRect,
    AnimatedString,
    AnimatedTransformList,
    AnimatedUnknown
};

struct SVGPropertyInfo {
    WTF_MAKE_FAST_ALLOCATED;
public:
    typedef void (*SynchronizeProperty)(SVGElement*);
    typedef PassRefPtr<SVGAnimatedProperty> (*LookupOrCreateWrapperForAnimatedProperty)(SVGElement*);

    SVGPropertyInfo(AnimatedPropertyType newType, AnimatedPropertyState newState, const QualifiedName& newAttributeName,
                    const AtomicString& newPropertyIdentifier, SynchronizeProperty newSynchronizeProperty,
                    LookupOrCreateWrapperForAnimatedProperty newLookupOrCreateWrapperForAnimatedProperty)
        : animatedPropertyType(newType)
        , animatedPropertyState(newState)
        , attributeName(newAttributeName)
        , propertyIdentifier(newPropertyIdentifier)
        , synchronizeProperty(newSynchronizeProperty)
        , lookupOrCreateWrapperForAnimatedProperty(newLookupOrCreateWrapperForAnimatedProperty)
    {
    }

    AnimatedPropertyType animatedPropertyType;
    AnimatedPropertyState animatedPropertyState;
    const QualifiedName& attributeName;
    const AtomicString& propertyIdentifier;
    SynchronizeProperty synchronizeProperty;
    LookupOrCreateWrapperForAnimatedProperty lookupOrCreateWrapperForAnimatedProperty;
};

}

#endif // ENABLE(SVG)
#endif // SVGPropertyInfo_h
