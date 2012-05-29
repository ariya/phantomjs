/*
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

#ifndef SVGAnimatedAngle_h
#define SVGAnimatedAngle_h

#if ENABLE(SVG)
#include "SVGAngle.h"
#include "SVGAnimatedPropertyTearOff.h"

namespace WebCore {

typedef SVGAnimatedPropertyTearOff<SVGAngle> SVGAnimatedAngle;

// Helper macros to declare/define a SVGAnimatedAngle object
#define DECLARE_ANIMATED_ANGLE(UpperProperty, LowerProperty) \
DECLARE_ANIMATED_PROPERTY(SVGAnimatedAngle, SVGAngle, UpperProperty, LowerProperty)

#define DEFINE_ANIMATED_ANGLE_MULTIPLE_WRAPPERS(OwnerType, DOMAttribute, SVGDOMAttributeIdentifier, UpperProperty, LowerProperty) \
DEFINE_ANIMATED_PROPERTY(OwnerType, DOMAttribute, SVGDOMAttributeIdentifier, SVGAnimatedAngle, SVGAngle, UpperProperty, LowerProperty)

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
