/*
 * Copyright (C) 2004, 2005 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006 Rob Buis <buis@kde.org>
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

#include "config.h"

#if ENABLE(SVG) && ENABLE(SVG_ANIMATION)
#include "SVGAnimateElement.h"

#include "CSSComputedStyleDeclaration.h"
#include "CSSParser.h"
#include "CSSPropertyNames.h"
#include "ColorDistance.h"
#include "FloatConversion.h"
#include "QualifiedName.h"
#include "RenderObject.h"
#include "SVGColor.h"
#include "SVGNames.h"
#include "SVGParserUtilities.h"
#include "SVGPathParserFactory.h"
#include "SVGPathSegList.h"
#include "SVGPointList.h"
#include "SVGStyledElement.h"

using namespace std;

namespace WebCore {

SVGAnimateElement::SVGAnimateElement(const QualifiedName& tagName, Document* document)
    : SVGAnimationElement(tagName, document)
    , m_animatedAttributeType(AnimatedString)
    , m_fromNumber(0)
    , m_toNumber(0)
    , m_animatedNumber(numeric_limits<double>::infinity())
    , m_animatedPathPointer(0)
{
}

PassRefPtr<SVGAnimateElement> SVGAnimateElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new SVGAnimateElement(tagName, document));
}

SVGAnimateElement::~SVGAnimateElement()
{
}

static bool parseNumberValueAndUnit(const String& in, double& value, String& unit)
{
    // FIXME: These are from top of my head, figure out all property types that can be animated as numbers.
    unsigned unitLength = 0;
    String parse = in.stripWhiteSpace();
    if (parse.endsWith("%"))
        unitLength = 1;
    else if (parse.endsWith("px") || parse.endsWith("pt") || parse.endsWith("em"))
        unitLength = 2;
    else if (parse.endsWith("deg") || parse.endsWith("rad"))
        unitLength = 3;
    else if (parse.endsWith("grad"))
        unitLength = 4;
    String newUnit = parse.right(unitLength);
    String number = parse.left(parse.length() - unitLength);
    if ((!unit.isEmpty() && newUnit != unit) || number.isEmpty())
        return false;
    UChar last = number[number.length() - 1];
    if (last < '0' || last > '9')
        return false;
    unit = newUnit;
    bool ok;
    value = number.toDouble(&ok);
    return ok;
}

static inline void adjustForCurrentColor(SVGElement* targetElement, Color& color)
{
    ASSERT(targetElement);
    
    if (RenderObject* targetRenderer = targetElement->renderer())
        color = targetRenderer->style()->visitedDependentColor(CSSPropertyColor);
    else
        color = Color();
}

static inline void adjustForInheritance(SVGElement* targetElement, const QualifiedName& attributeName, String& value)
{
    // FIXME: At the moment the computed style gets returned as a String and needs to get parsed again.
    // In the future we might want to work with the value type directly to avoid the String parsing.
    ASSERT(targetElement);

    Element* parent = targetElement->parentElement();
    if (!parent || !parent->isSVGElement())
        return;

    SVGElement* svgParent = static_cast<SVGElement*>(parent);
    if (svgParent->isStyled())
        value = computedStyle(svgParent)->getPropertyValue(cssPropertyID(attributeName.localName()));
}

bool SVGAnimateElement::hasValidAttributeType() const
{
    SVGElement* targetElement = this->targetElement();
    if (!targetElement)
        return false;
    
    return determineAnimatedAttributeType(targetElement) != AnimatedUnknown;
}

AnimatedAttributeType SVGAnimateElement::determineAnimatedAttributeType(SVGElement* targetElement) const
{
    ASSERT(targetElement);

    AnimatedAttributeType type = targetElement->animatedPropertyTypeForAttribute(attributeName());
    if (type == AnimatedUnknown || (hasTagName(SVGNames::animateColorTag) && type != AnimatedColor))
        return AnimatedUnknown;

    // FIXME: We need type specific animations in the future. Many animations marked as AnimatedString today will
    // support continuous animations.
    switch (type) {
    case AnimatedBoolean:
    case AnimatedEnumeration:
    case AnimatedLengthList:
    case AnimatedNumberList:
    case AnimatedNumberOptionalNumber:
    case AnimatedPreserveAspectRatio:
    case AnimatedRect:
    case AnimatedString:
        return AnimatedString;
    case AnimatedAngle:
    case AnimatedInteger:
    case AnimatedLength:
    case AnimatedNumber:
        return AnimatedNumber;
    case AnimatedPath:
        return AnimatedPath;
    case AnimatedPoints:
        return AnimatedPoints;
    case AnimatedColor:
        return AnimatedColor;
    case AnimatedUnknown:
    case AnimatedTransformList:
        // Animations of transform lists are not allowed for <animate> or <set>
        // http://www.w3.org/TR/SVG/animate.html#AnimationAttributesAndProperties
        return AnimatedUnknown;
    }

    ASSERT_NOT_REACHED();
    return AnimatedUnknown;
}

void SVGAnimateElement::calculateAnimatedValue(float percentage, unsigned repeat, SVGSMILElement* resultElement)
{
    ASSERT(percentage >= 0 && percentage <= 1);
    ASSERT(resultElement);
    bool isInFirstHalfOfAnimation = percentage < 0.5f;
    AnimationMode animationMode = this->animationMode();
    SVGElement* targetElement = 0;
    // Avoid targetElement() call if possible. It might slow down animations.
    if (m_fromPropertyValueType == InheritValue || m_toPropertyValueType == InheritValue
        || m_fromPropertyValueType == CurrentColorValue || m_toPropertyValueType == CurrentColorValue) {
        targetElement = this->targetElement();
        if (!targetElement)
            return;
    }
    
    if (hasTagName(SVGNames::setTag))
        percentage = 1;
    if (!resultElement->hasTagName(SVGNames::animateTag) && !resultElement->hasTagName(SVGNames::animateColorTag) 
        && !resultElement->hasTagName(SVGNames::setTag))
        return;
    SVGAnimateElement* results = static_cast<SVGAnimateElement*>(resultElement);
    // Can't accumulate over a string property.
    if (results->m_animatedAttributeType == AnimatedString && m_animatedAttributeType != AnimatedString)
        return;
    if (m_animatedAttributeType == AnimatedNumber) {
        // To animation uses contributions from the lower priority animations as the base value.
        if (animationMode == ToAnimation)
            m_fromNumber = results->m_animatedNumber;
        
        // Replace 'currentColor' / 'inherit' by their computed property values.
        if (m_fromPropertyValueType == InheritValue) {
            String fromNumberString;
            adjustForInheritance(targetElement, attributeName(), fromNumberString);
            if (!parseNumberValueAndUnit(fromNumberString, m_fromNumber, m_numberUnit))
                return;
        }
        if (m_toPropertyValueType == InheritValue) {
            String toNumberString;
            adjustForInheritance(targetElement, attributeName(), toNumberString);
            if (!parseNumberValueAndUnit(toNumberString, m_toNumber, m_numberUnit))
                return;
        }

        double number;
        if (calcMode() == CalcModeDiscrete)
            number = isInFirstHalfOfAnimation ? m_fromNumber : m_toNumber;
        else
            number = (m_toNumber - m_fromNumber) * percentage + m_fromNumber;

        // FIXME: This is not correct for values animation.
        if (isAccumulated() && repeat)
            number += m_toNumber * repeat;
        if (isAdditive() && animationMode != ToAnimation)
            results->m_animatedNumber += number;
        else 
            results->m_animatedNumber = number;
        return;
    } 
    if (m_animatedAttributeType == AnimatedColor) {
        if (animationMode == ToAnimation)
            m_fromColor = results->m_animatedColor;

        // Replace 'currentColor' / 'inherit' by their computed property values.
        if (m_fromPropertyValueType == CurrentColorValue)
            adjustForCurrentColor(targetElement, m_fromColor);
        else if (m_fromPropertyValueType == InheritValue) {
            String fromColorString;
            adjustForInheritance(targetElement, attributeName(), fromColorString);
            m_fromColor = SVGColor::colorFromRGBColorString(fromColorString);
        }
        if (m_toPropertyValueType == CurrentColorValue)
            adjustForCurrentColor(targetElement, m_toColor);
        else if (m_toPropertyValueType == InheritValue) {
            String toColorString;
            adjustForInheritance(targetElement, attributeName(), toColorString);
            m_toColor = SVGColor::colorFromRGBColorString(toColorString);
        }

        Color color;
        if (calcMode() == CalcModeDiscrete)
            color = isInFirstHalfOfAnimation ? m_fromColor : m_toColor;
        else
            color = ColorDistance(m_fromColor, m_toColor).scaledDistance(percentage).addToColorAndClamp(m_fromColor);

        // FIXME: Accumulate colors.
        if (isAdditive() && animationMode != ToAnimation)
            results->m_animatedColor = ColorDistance::addColorsAndClamp(results->m_animatedColor, color);
        else
            results->m_animatedColor = color;
        return;
    }
    if (m_animatedAttributeType == AnimatedPath) {
        if (animationMode == ToAnimation) {
            ASSERT(results->m_animatedPathPointer);
            m_fromPath = results->m_animatedPathPointer->copy();
        }
        if (!percentage) {
            ASSERT(m_fromPath);
            ASSERT(percentage >= 0);
            results->m_animatedPathPointer = m_fromPath.get();
        } else if (percentage == 1) {
            ASSERT(m_toPath);
            results->m_animatedPathPointer = m_toPath.get();
        } else {
            if (m_fromPath && m_toPath) {
                SVGPathParserFactory* factory = SVGPathParserFactory::self();
                if (!factory->buildAnimatedSVGPathByteStream(m_fromPath.get(), m_toPath.get(), results->m_animatedPath, percentage)) {
                    results->m_animatedPath.clear();
                    results->m_animatedPathPointer = 0;
                } else
                    results->m_animatedPathPointer = results->m_animatedPath.get();
            } else
                results->m_animatedPathPointer = 0;
            // Fall back to discrete animation if the paths are not compatible
            if (!results->m_animatedPathPointer) {
                ASSERT(m_fromPath);
                ASSERT(m_toPath);
                ASSERT(!results->m_animatedPath);
                results->m_animatedPathPointer = ((animationMode == FromToAnimation && percentage > 0.5f) || animationMode == ToAnimation || percentage == 1) 
                    ? m_toPath.get() : m_fromPath.get();
            }
        }
        return;
    }
    if (m_animatedAttributeType == AnimatedPoints) {
        if (!percentage)
            results->m_animatedPoints = m_fromPoints;
        else if (percentage == 1)
            results->m_animatedPoints = m_toPoints;
        else {
            if (!m_fromPoints.isEmpty() && !m_toPoints.isEmpty())
                SVGPointList::createAnimated(m_fromPoints, m_toPoints, results->m_animatedPoints, percentage);
            else
                results->m_animatedPoints.clear();
            // Fall back to discrete animation if the points are not compatible
            if (results->m_animatedPoints.isEmpty())
                results->m_animatedPoints = ((animationMode == FromToAnimation && percentage > 0.5f) || animationMode == ToAnimation || percentage == 1) 
                    ? m_toPoints : m_fromPoints;
        }
        return;
    }
    ASSERT(animationMode == FromToAnimation || animationMode == ToAnimation || animationMode == ValuesAnimation);
    // Replace 'currentColor' / 'inherit' by their computed property values.
    if (m_fromPropertyValueType == InheritValue)
        adjustForInheritance(targetElement, attributeName(), m_fromString);
    if (m_toPropertyValueType == InheritValue)
        adjustForInheritance(targetElement, attributeName(), m_toString);

    if ((animationMode == FromToAnimation && percentage > 0.5f) || animationMode == ToAnimation || percentage == 1)
        results->m_animatedString = m_toString;
    else
        results->m_animatedString = m_fromString;
    // Higher priority replace animation overrides any additive results so far.
    results->m_animatedAttributeType = AnimatedString;
}

static bool inheritsFromProperty(SVGElement* targetElement, const QualifiedName& attributeName, const String& value)
{
    ASSERT(targetElement);
    DEFINE_STATIC_LOCAL(const AtomicString, inherit, ("inherit"));

    if (value.isEmpty() || value != inherit || !targetElement->isStyled())
        return false;
    return SVGStyledElement::isAnimatableCSSProperty(attributeName);
}

static bool attributeValueIsCurrentColor(const String& value)
{
    DEFINE_STATIC_LOCAL(const AtomicString, currentColor, ("currentColor"));
    return value == currentColor;
}

bool SVGAnimateElement::calculateFromAndToValues(const String& fromString, const String& toString)
{
    SVGElement* targetElement = this->targetElement();
    if (!targetElement)
        return false;
    m_fromPropertyValueType = inheritsFromProperty(targetElement, attributeName(), fromString) ? InheritValue : RegularPropertyValue;
    m_toPropertyValueType = inheritsFromProperty(targetElement, attributeName(), toString) ? InheritValue : RegularPropertyValue;

    // FIXME: Needs more solid way determine target attribute type.
    m_animatedAttributeType = determineAnimatedAttributeType(targetElement);
    if (m_animatedAttributeType == AnimatedColor) {
        bool fromIsCurrentColor = attributeValueIsCurrentColor(fromString);
        bool toIsCurrentColor = attributeValueIsCurrentColor(toString);
        if (fromIsCurrentColor)
            m_fromPropertyValueType = CurrentColorValue;
        else
            m_fromColor = SVGColor::colorFromRGBColorString(fromString);
        if (toIsCurrentColor)
            m_toPropertyValueType = CurrentColorValue;
        else
            m_toColor = SVGColor::colorFromRGBColorString(toString);
        bool fromIsValid = m_fromColor.isValid() || fromIsCurrentColor || m_fromPropertyValueType == InheritValue;
        bool toIsValid = m_toColor.isValid() || toIsCurrentColor || m_toPropertyValueType == InheritValue;
        if ((fromIsValid && toIsValid) || (toIsValid && animationMode() == ToAnimation))
            return true;
    } else if (m_animatedAttributeType == AnimatedNumber) {
        m_numberUnit = String();
        if (parseNumberValueAndUnit(toString, m_toNumber, m_numberUnit)) {
            // For to-animations the from number is calculated later
            if (animationMode() == ToAnimation || parseNumberValueAndUnit(fromString, m_fromNumber, m_numberUnit))
                return true;
        }
    } else if (m_animatedAttributeType == AnimatedPath) {
        SVGPathParserFactory* factory = SVGPathParserFactory::self();
        if (factory->buildSVGPathByteStreamFromString(toString, m_toPath, UnalteredParsing)) {
            // For to-animations the from number is calculated later
            if (animationMode() == ToAnimation || factory->buildSVGPathByteStreamFromString(fromString, m_fromPath, UnalteredParsing))
                return true;
        }
        m_fromPath.clear();
        m_toPath.clear();
    } else if (m_animatedAttributeType == AnimatedPoints) {
        m_fromPoints.clear();
        if (pointsListFromSVGData(m_fromPoints, fromString)) {
            m_toPoints.clear();
            if (pointsListFromSVGData(m_toPoints, toString))
                return true;
        }
    }
    m_fromString = fromString;
    m_toString = toString;
    m_animatedAttributeType = AnimatedString;
    return true;
}

bool SVGAnimateElement::calculateFromAndByValues(const String& fromString, const String& byString)
{
    SVGElement* targetElement = this->targetElement();
    if (!targetElement)
        return false;
    m_fromPropertyValueType = inheritsFromProperty(targetElement, attributeName(), fromString) ? InheritValue : RegularPropertyValue;
    m_toPropertyValueType = inheritsFromProperty(targetElement, attributeName(), byString) ? InheritValue : RegularPropertyValue;

    ASSERT(!hasTagName(SVGNames::setTag));
    m_animatedAttributeType = determineAnimatedAttributeType(targetElement);
    if (m_animatedAttributeType == AnimatedColor) {
        bool fromIsCurrentColor = attributeValueIsCurrentColor(fromString);
        bool byIsCurrentColor = attributeValueIsCurrentColor(byString);
        if (fromIsCurrentColor)
            m_fromPropertyValueType = CurrentColorValue;
        else
            m_fromColor = SVGColor::colorFromRGBColorString(fromString);
        if (byIsCurrentColor)
            m_toPropertyValueType = CurrentColorValue;
        else
            m_toColor = SVGColor::colorFromRGBColorString(byString);
        
        if ((!m_fromColor.isValid() && !fromIsCurrentColor)
            || (!m_toColor.isValid() && !byIsCurrentColor))
            return false;
    } else {
        m_numberUnit = String();
        m_fromNumber = 0;
        if (!fromString.isEmpty() && !parseNumberValueAndUnit(fromString, m_fromNumber, m_numberUnit))
            return false;
        if (!parseNumberValueAndUnit(byString, m_toNumber, m_numberUnit))
            return false;
        m_toNumber += m_fromNumber;
    }
    return true;
}

void SVGAnimateElement::resetToBaseValue(const String& baseString)
{
    SVGElement* targetElement = this->targetElement();
    ASSERT(targetElement);
    m_animatedString = baseString;
    AnimatedAttributeType lastType = m_animatedAttributeType;
    m_animatedAttributeType = determineAnimatedAttributeType(targetElement);
    if (m_animatedAttributeType == AnimatedColor) {
        m_animatedColor = baseString.isEmpty() ? Color() : SVGColor::colorFromRGBColorString(baseString);
        if (isContributing(elapsed())) {
            m_animatedAttributeType = lastType;
            return;
        }
    } else if (m_animatedAttributeType == AnimatedNumber) {
        if (baseString.isEmpty()) {
            m_animatedNumber = 0;
            m_numberUnit = String();
            return;
        }
        if (parseNumberValueAndUnit(baseString, m_animatedNumber, m_numberUnit))
            return;
    } else if (m_animatedAttributeType == AnimatedPath) {
        m_animatedPath.clear();
        SVGPathParserFactory* factory = SVGPathParserFactory::self();
        factory->buildSVGPathByteStreamFromString(baseString, m_animatedPath, UnalteredParsing);
        m_animatedPathPointer = m_animatedPath.get();
        return;
    } else if (m_animatedAttributeType == AnimatedPoints) {
        m_animatedPoints.clear();
        return;
    }
    m_animatedAttributeType = AnimatedString;
}
    
void SVGAnimateElement::applyResultsToTarget()
{
    String valueToApply;
    if (m_animatedAttributeType == AnimatedColor)
        valueToApply = m_animatedColor.serialized();
    else if (m_animatedAttributeType == AnimatedNumber)
        valueToApply = String::number(m_animatedNumber) + m_numberUnit;
    else if (m_animatedAttributeType == AnimatedPath) {
        if (!m_animatedPathPointer || m_animatedPathPointer->isEmpty())
            valueToApply = m_animatedString;
        else {
            // We need to keep going to string and back because we are currently only able to paint
            // "processed" paths where complex shapes are replaced with simpler ones. Path 
            // morphing needs to be done with unprocessed paths.
            // FIXME: This could be optimized if paths were not processed at parse time.
            SVGPathParserFactory* factory = SVGPathParserFactory::self();
            factory->buildStringFromByteStream(m_animatedPathPointer, valueToApply, UnalteredParsing);
        }
    } else if (m_animatedAttributeType == AnimatedPoints)
        valueToApply = m_animatedPoints.isEmpty() ? m_animatedString : m_animatedPoints.valueAsString();
    else
        valueToApply = m_animatedString;
    
    setTargetAttributeAnimatedValue(valueToApply);
}
    
float SVGAnimateElement::calculateDistance(const String& fromString, const String& toString)
{
    SVGElement* targetElement = this->targetElement();
    if (!targetElement)
        return -1;
    m_animatedAttributeType = determineAnimatedAttributeType(targetElement);
    if (m_animatedAttributeType == AnimatedNumber) {
        double from;
        double to;
        String unit;
        if (!parseNumberValueAndUnit(fromString, from, unit))
            return -1;
        if (!parseNumberValueAndUnit(toString, to, unit))
            return -1;
        return narrowPrecisionToFloat(fabs(to - from));
    }
    if (m_animatedAttributeType == AnimatedColor) {
        Color from = SVGColor::colorFromRGBColorString(fromString);
        if (!from.isValid())
            return -1;
        Color to = SVGColor::colorFromRGBColorString(toString);
        if (!to.isValid())
            return -1;
        return ColorDistance(from, to).distance();
    }
    return -1;
}
   
}
#endif // ENABLE(SVG)
