/*
 * Copyright (C) 2004, 2005 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005 Rob Buis <buis@kde.org>
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

#ifndef SVGAnimateElement_h
#define SVGAnimateElement_h

#if ENABLE(SVG) && ENABLE(SVG_ANIMATION)

#include "Color.h"
#include "SVGAnimationElement.h"
#include "SVGPathByteStream.h"
#include "SVGPointList.h"
#include <wtf/OwnPtr.h>

namespace WebCore {

class SVGPathSegList;

class SVGAnimateElement : public SVGAnimationElement {
public:
    static PassRefPtr<SVGAnimateElement> create(const QualifiedName&, Document*);

    virtual ~SVGAnimateElement();

protected:
    SVGAnimateElement(const QualifiedName&, Document*);

    virtual void resetToBaseValue(const String&);
    virtual bool calculateFromAndToValues(const String& fromString, const String& toString);
    virtual bool calculateFromAndByValues(const String& fromString, const String& byString);
    virtual void calculateAnimatedValue(float percentage, unsigned repeat, SVGSMILElement* resultElement);
    virtual void applyResultsToTarget();
    virtual float calculateDistance(const String& fromString, const String& toString);

private:
    // If we have 'currentColor' or 'inherit' as animation value, we need to grab the value during the animation
    // since the value can be animated itself.
    enum AnimatedPropertyValueType {
        RegularPropertyValue,
        CurrentColorValue,
        InheritValue
    };
    
    virtual bool hasValidAttributeType() const;
    AnimatedAttributeType determineAnimatedAttributeType(SVGElement*) const;
    AnimatedAttributeType m_animatedAttributeType;

    AnimatedPropertyValueType m_fromPropertyValueType;
    AnimatedPropertyValueType m_toPropertyValueType;
    double m_fromNumber;
    double m_toNumber;
    double m_animatedNumber;
    String m_numberUnit;
    Color m_fromColor;
    Color m_toColor;
    Color m_animatedColor;
    String m_fromString;
    String m_toString;
    String m_animatedString;
    OwnPtr<SVGPathByteStream> m_fromPath;
    OwnPtr<SVGPathByteStream> m_toPath;
    OwnPtr<SVGPathByteStream> m_animatedPath;
    SVGPathByteStream* m_animatedPathPointer;
    SVGPointList m_fromPoints;
    SVGPointList m_toPoints;
    SVGPointList m_animatedPoints;
};

} // namespace WebCore

#endif // ENABLE(SVG)
#endif // SVGAnimateElement_h

// vim:ts=4:noet
