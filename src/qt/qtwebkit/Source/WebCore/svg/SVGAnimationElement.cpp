/*
 * Copyright (C) 2004, 2005 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006, 2007 Rob Buis <buis@kde.org>
 * Copyright (C) 2007 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2009 Cameron McCormack <cam@mcc.id.au>
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

#include "config.h"

#if ENABLE(SVG)
#include "SVGAnimationElement.h"

#include "Attribute.h"
#include "CSSComputedStyleDeclaration.h"
#include "CSSParser.h"
#include "CSSPropertyNames.h"
#include "Document.h"
#include "FloatConversion.h"
#include "RenderObject.h"
#include "SVGAnimateElement.h"
#include "SVGElementInstance.h"
#include "SVGNames.h"
#include "SVGParserUtilities.h"
#include "SVGStyledElement.h"
#include <wtf/MathExtras.h>

namespace WebCore {

// Animated property definitions
DEFINE_ANIMATED_BOOLEAN(SVGAnimationElement, SVGNames::externalResourcesRequiredAttr, ExternalResourcesRequired, externalResourcesRequired)

BEGIN_REGISTER_ANIMATED_PROPERTIES(SVGAnimationElement)
    REGISTER_LOCAL_ANIMATED_PROPERTY(externalResourcesRequired)
    REGISTER_PARENT_ANIMATED_PROPERTIES(SVGTests)
END_REGISTER_ANIMATED_PROPERTIES

SVGAnimationElement::SVGAnimationElement(const QualifiedName& tagName, Document* document)
    : SVGSMILElement(tagName, document)
    , m_fromPropertyValueType(RegularPropertyValue)
    , m_toPropertyValueType(RegularPropertyValue)
    , m_animationValid(false)
    , m_attributeType(AttributeTypeAuto)
    , m_hasInvalidCSSAttributeType(false)
    , m_calcMode(CalcModeLinear)
    , m_animationMode(NoAnimation)
{
    registerAnimatedPropertiesForSVGAnimationElement();
}

static void parseKeyTimes(const String& parse, Vector<float>& result, bool verifyOrder)
{
    result.clear();
    Vector<String> parseList;
    parse.split(';', parseList);
    for (unsigned n = 0; n < parseList.size(); ++n) {
        String timeString = parseList[n];
        bool ok;
        float time = timeString.toFloat(&ok);
        if (!ok || time < 0 || time > 1)
            goto fail;
        if (verifyOrder) {
            if (!n) {
                if (time)
                    goto fail;
            } else if (time < result.last())
                goto fail;
        }
        result.append(time);
    }
    return;
fail:
    result.clear();
}

static void parseKeySplines(const String& parse, Vector<UnitBezier>& result)
{
    result.clear();
    if (parse.isEmpty())
        return;
    const UChar* cur = parse.characters();
    const UChar* end = cur + parse.length();

    skipOptionalSVGSpaces(cur, end);

    bool delimParsed = false;
    while (cur < end) {
        delimParsed = false;
        float posA = 0;
        if (!parseNumber(cur, end, posA)) {
            result.clear();
            return;
        }

        float posB = 0;
        if (!parseNumber(cur, end, posB)) {
            result.clear();
            return;
        }

        float posC = 0;
        if (!parseNumber(cur, end, posC)) {
            result.clear();
            return;
        }

        float posD = 0;
        if (!parseNumber(cur, end, posD, false)) {
            result.clear();
            return;
        }

        skipOptionalSVGSpaces(cur, end);

        if (cur < end && *cur == ';') {
            delimParsed = true;
            cur++;
        }
        skipOptionalSVGSpaces(cur, end);

        result.append(UnitBezier(posA, posB, posC, posD));
    }
    if (!(cur == end && !delimParsed))
        result.clear();
}

bool SVGAnimationElement::isSupportedAttribute(const QualifiedName& attrName)
{
    DEFINE_STATIC_LOCAL(HashSet<QualifiedName>, supportedAttributes, ());
    if (supportedAttributes.isEmpty()) {
        SVGTests::addSupportedAttributes(supportedAttributes);
        SVGExternalResourcesRequired::addSupportedAttributes(supportedAttributes);
        supportedAttributes.add(SVGNames::valuesAttr);
        supportedAttributes.add(SVGNames::keyTimesAttr);
        supportedAttributes.add(SVGNames::keyPointsAttr);
        supportedAttributes.add(SVGNames::keySplinesAttr);
        supportedAttributes.add(SVGNames::attributeTypeAttr);
        supportedAttributes.add(SVGNames::calcModeAttr);
        supportedAttributes.add(SVGNames::fromAttr);
        supportedAttributes.add(SVGNames::toAttr);
        supportedAttributes.add(SVGNames::byAttr);
    }
    return supportedAttributes.contains<SVGAttributeHashTranslator>(attrName);
}

void SVGAnimationElement::parseAttribute(const QualifiedName& name, const AtomicString& value)
{
    if (!isSupportedAttribute(name)) {
        SVGSMILElement::parseAttribute(name, value);
        return;
    }

    if (name == SVGNames::valuesAttr) {
        // Per the SMIL specification, leading and trailing white space,
        // and white space before and after semicolon separators, is allowed and will be ignored.
        // http://www.w3.org/TR/SVG11/animate.html#ValuesAttribute
        value.string().split(';', m_values);
        for (unsigned i = 0; i < m_values.size(); ++i)
            m_values[i] = m_values[i].stripWhiteSpace();

        updateAnimationMode();
        return;
    }

    if (name == SVGNames::keyTimesAttr) {
        parseKeyTimes(value, m_keyTimes, true);
        return;
    }

    if (name == SVGNames::keyPointsAttr) {
        if (hasTagName(SVGNames::animateMotionTag)) {
            // This is specified to be an animateMotion attribute only but it is simpler to put it here 
            // where the other timing calculatations are.
            parseKeyTimes(value, m_keyPoints, false);
        }
        return;
    }

    if (name == SVGNames::keySplinesAttr) {
        parseKeySplines(value, m_keySplines);
        return;
    }

    if (name == SVGNames::attributeTypeAttr) {
        setAttributeType(value);
        return;
    }

    if (name == SVGNames::calcModeAttr) {
        setCalcMode(value);
        return;
    }

    if (name == SVGNames::fromAttr || name == SVGNames::toAttr || name == SVGNames::byAttr) {
        updateAnimationMode();
        return;
    }

    if (SVGTests::parseAttribute(name, value))
        return;
    if (SVGExternalResourcesRequired::parseAttribute(name, value))
        return;

    ASSERT_NOT_REACHED();
}

void SVGAnimationElement::svgAttributeChanged(const QualifiedName& attrName)
{
    if (!isSupportedAttribute(attrName)) {
        SVGSMILElement::svgAttributeChanged(attrName);
        return;
    }

    animationAttributeChanged();
}

void SVGAnimationElement::animationAttributeChanged()
{
    // Assumptions may not hold after an attribute change.
    m_animationValid = false;
    setInactive();
}

float SVGAnimationElement::getStartTime() const
{
    return narrowPrecisionToFloat(intervalBegin().value());
}

float SVGAnimationElement::getCurrentTime() const
{
    return narrowPrecisionToFloat(elapsed().value());
}

float SVGAnimationElement::getSimpleDuration(ExceptionCode&) const
{
    return narrowPrecisionToFloat(simpleDuration().value());
}    
    
void SVGAnimationElement::beginElement()
{
    beginElementAt(0);
}

void SVGAnimationElement::beginElementAt(float offset)
{
    if (std::isnan(offset))
        return;
    SMILTime elapsed = this->elapsed();
    addBeginTime(elapsed, elapsed + offset, SMILTimeWithOrigin::ScriptOrigin);
}

void SVGAnimationElement::endElement()
{
    endElementAt(0);
}

void SVGAnimationElement::endElementAt(float offset)
{
    if (std::isnan(offset))
        return;
    SMILTime elapsed = this->elapsed();
    addEndTime(elapsed, elapsed + offset, SMILTimeWithOrigin::ScriptOrigin);
}

void SVGAnimationElement::updateAnimationMode()
{
    // http://www.w3.org/TR/2001/REC-smil-animation-20010904/#AnimFuncValues
    if (hasAttribute(SVGNames::valuesAttr))
        setAnimationMode(ValuesAnimation);
    else if (!toValue().isEmpty())
        setAnimationMode(fromValue().isEmpty() ? ToAnimation : FromToAnimation);
    else if (!byValue().isEmpty())
        setAnimationMode(fromValue().isEmpty() ? ByAnimation : FromByAnimation);
    else
        setAnimationMode(NoAnimation);
}

void SVGAnimationElement::setCalcMode(const AtomicString& calcMode)
{
    DEFINE_STATIC_LOCAL(const AtomicString, discrete, ("discrete", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(const AtomicString, linear, ("linear", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(const AtomicString, paced, ("paced", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(const AtomicString, spline, ("spline", AtomicString::ConstructFromLiteral));
    if (calcMode == discrete)
        setCalcMode(CalcModeDiscrete);
    else if (calcMode == linear)
        setCalcMode(CalcModeLinear);
    else if (calcMode == paced)
        setCalcMode(CalcModePaced);
    else if (calcMode == spline)
        setCalcMode(CalcModeSpline);
    else
        setCalcMode(hasTagName(SVGNames::animateMotionTag) ? CalcModePaced : CalcModeLinear);
}

void SVGAnimationElement::setAttributeType(const AtomicString& attributeType)
{
    DEFINE_STATIC_LOCAL(const AtomicString, css, ("CSS", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(const AtomicString, xml, ("XML", AtomicString::ConstructFromLiteral));
    if (attributeType == css)
        m_attributeType = AttributeTypeCSS;
    else if (attributeType == xml)
        m_attributeType = AttributeTypeXML;
    else
        m_attributeType = AttributeTypeAuto;
    checkInvalidCSSAttributeType(targetElement());
}

String SVGAnimationElement::toValue() const
{    
    return fastGetAttribute(SVGNames::toAttr);
}

String SVGAnimationElement::byValue() const
{    
    return fastGetAttribute(SVGNames::byAttr);
}

String SVGAnimationElement::fromValue() const
{    
    return fastGetAttribute(SVGNames::fromAttr);
}

bool SVGAnimationElement::isAdditive() const
{
    DEFINE_STATIC_LOCAL(const AtomicString, sum, ("sum", AtomicString::ConstructFromLiteral));
    const AtomicString& value = fastGetAttribute(SVGNames::additiveAttr);
    return value == sum || animationMode() == ByAnimation;
}

bool SVGAnimationElement::isAccumulated() const
{
    DEFINE_STATIC_LOCAL(const AtomicString, sum, ("sum", AtomicString::ConstructFromLiteral));
    const AtomicString& value = fastGetAttribute(SVGNames::accumulateAttr);
    return value == sum && animationMode() != ToAnimation;
}

bool SVGAnimationElement::isTargetAttributeCSSProperty(SVGElement* targetElement, const QualifiedName& attributeName)
{
    ASSERT(targetElement);
    if (!targetElement->isSVGStyledElement())
        return false;

    return SVGStyledElement::isAnimatableCSSProperty(attributeName);
}

SVGAnimationElement::ShouldApplyAnimation SVGAnimationElement::shouldApplyAnimation(SVGElement* targetElement, const QualifiedName& attributeName)
{
    if (!hasValidAttributeType() || !targetElement || attributeName == anyQName())
        return DontApplyAnimation;

    // Always animate CSS properties, using the ApplyCSSAnimation code path, regardless of the attributeType value.
    if (isTargetAttributeCSSProperty(targetElement, attributeName))
        return ApplyCSSAnimation;

    // If attributeType="CSS" and attributeName doesn't point to a CSS property, ignore the animation.
    if (attributeType() == AttributeTypeCSS)
        return DontApplyAnimation;

    return ApplyXMLAnimation;
}

void SVGAnimationElement::calculateKeyTimesForCalcModePaced()
{
    ASSERT(calcMode() == CalcModePaced);
    ASSERT(animationMode() == ValuesAnimation);

    unsigned valuesCount = m_values.size();
    ASSERT(valuesCount >= 1);
    if (valuesCount == 1)
        return;

    // FIXME, webkit.org/b/109010: m_keyTimes should not be modified in this function.
    m_keyTimes.clear();

    Vector<float> keyTimesForPaced;
    float totalDistance = 0;
    keyTimesForPaced.append(0);
    for (unsigned n = 0; n < valuesCount - 1; ++n) {
        // Distance in any units
        float distance = calculateDistance(m_values[n], m_values[n + 1]);
        if (distance < 0)
            return;
        totalDistance += distance;
        keyTimesForPaced.append(distance);
    }
    if (!totalDistance)
        return;

    // Normalize.
    for (unsigned n = 1; n < keyTimesForPaced.size() - 1; ++n)
        keyTimesForPaced[n] = keyTimesForPaced[n - 1] + keyTimesForPaced[n] / totalDistance;
    keyTimesForPaced[keyTimesForPaced.size() - 1] = 1;

    // Use key times calculated based on pacing instead of the user provided ones.
    m_keyTimes = keyTimesForPaced;
}

static inline double solveEpsilon(double duration) { return 1 / (200 * duration); }

unsigned SVGAnimationElement::calculateKeyTimesIndex(float percent) const
{
    unsigned index;
    unsigned keyTimesCount = m_keyTimes.size();
    // Compare index + 1 to keyTimesCount because the last keyTimes entry is
    // required to be 1, and percent can never exceed 1; i.e., the second last
    // keyTimes entry defines the beginning of the final interval
    for (index = 1; index + 1 < keyTimesCount; ++index) {
        if (m_keyTimes[index] > percent)
            break;
    }
    return --index;
}

float SVGAnimationElement::calculatePercentForSpline(float percent, unsigned splineIndex) const
{
    ASSERT(calcMode() == CalcModeSpline);
    ASSERT_WITH_SECURITY_IMPLICATION(splineIndex < m_keySplines.size());
    UnitBezier bezier = m_keySplines[splineIndex];
    SMILTime duration = simpleDuration();
    if (!duration.isFinite())
        duration = 100.0;
    return narrowPrecisionToFloat(bezier.solve(percent, solveEpsilon(duration.value())));
}

float SVGAnimationElement::calculatePercentFromKeyPoints(float percent) const
{
    ASSERT(!m_keyPoints.isEmpty());
    ASSERT(calcMode() != CalcModePaced);
    ASSERT(m_keyTimes.size() > 1);
    ASSERT(m_keyPoints.size() == m_keyTimes.size());

    if (percent == 1)
        return m_keyPoints[m_keyPoints.size() - 1];

    unsigned index = calculateKeyTimesIndex(percent);
    float fromPercent = m_keyTimes[index];
    float toPercent = m_keyTimes[index + 1];
    float fromKeyPoint = m_keyPoints[index];
    float toKeyPoint = m_keyPoints[index + 1];
    
    if (calcMode() == CalcModeDiscrete)
        return fromKeyPoint;
    
    float keyPointPercent = (percent - fromPercent) / (toPercent - fromPercent);
    
    if (calcMode() == CalcModeSpline) {
        ASSERT(m_keySplines.size() == m_keyPoints.size() - 1);
        keyPointPercent = calculatePercentForSpline(keyPointPercent, index);
    }
    return (toKeyPoint - fromKeyPoint) * keyPointPercent + fromKeyPoint;
}

float SVGAnimationElement::calculatePercentForFromTo(float percent) const
{
    if (calcMode() == CalcModeDiscrete && m_keyTimes.size() == 2)
        return percent > m_keyTimes[1] ? 1 : 0;

    return percent;
}
    
void SVGAnimationElement::currentValuesFromKeyPoints(float percent, float& effectivePercent, String& from, String& to) const
{
    ASSERT(!m_keyPoints.isEmpty());
    ASSERT(m_keyPoints.size() == m_keyTimes.size());
    ASSERT(calcMode() != CalcModePaced);
    effectivePercent = calculatePercentFromKeyPoints(percent);
    unsigned index = effectivePercent == 1 ? m_values.size() - 2 : static_cast<unsigned>(effectivePercent * (m_values.size() - 1));
    from = m_values[index];
    to = m_values[index + 1];
}
    
void SVGAnimationElement::currentValuesForValuesAnimation(float percent, float& effectivePercent, String& from, String& to)
{
    unsigned valuesCount = m_values.size();
    ASSERT(m_animationValid);
    ASSERT(valuesCount >= 1);

    if (percent == 1 || valuesCount == 1) {
        from = m_values[valuesCount - 1];
        to = m_values[valuesCount - 1];
        effectivePercent = 1;
        return;
    }

    CalcMode calcMode = this->calcMode();
    if (hasTagName(SVGNames::animateTag) || hasTagName(SVGNames::animateColorTag)) {
        SVGAnimateElement* animateElement = static_cast<SVGAnimateElement*>(this);
        AnimatedPropertyType attributeType = animateElement->determineAnimatedPropertyType(targetElement());
        // Fall back to discrete animations for Strings.
        if (attributeType == AnimatedBoolean
            || attributeType == AnimatedEnumeration
            || attributeType == AnimatedPreserveAspectRatio
            || attributeType == AnimatedString)
            calcMode = CalcModeDiscrete;
    }
    if (!m_keyPoints.isEmpty() && calcMode != CalcModePaced)
        return currentValuesFromKeyPoints(percent, effectivePercent, from, to);
    
    unsigned keyTimesCount = m_keyTimes.size();
    ASSERT(!keyTimesCount || valuesCount == keyTimesCount);
    ASSERT(!keyTimesCount || (keyTimesCount > 1 && !m_keyTimes[0]));

    unsigned index = calculateKeyTimesIndex(percent);
    if (calcMode == CalcModeDiscrete) {
        if (!keyTimesCount) 
            index = static_cast<unsigned>(percent * valuesCount);
        from = m_values[index];
        to = m_values[index];
        effectivePercent = 0;
        return;
    }
    
    float fromPercent;
    float toPercent;
    if (keyTimesCount) {
        fromPercent = m_keyTimes[index];
        toPercent = m_keyTimes[index + 1];
    } else {        
        index = static_cast<unsigned>(floorf(percent * (valuesCount - 1)));
        fromPercent =  static_cast<float>(index) / (valuesCount - 1);
        toPercent =  static_cast<float>(index + 1) / (valuesCount - 1);
    }
    
    if (index == valuesCount - 1)
        --index;
    from = m_values[index];
    to = m_values[index + 1];
    ASSERT(toPercent > fromPercent);
    effectivePercent = (percent - fromPercent) / (toPercent - fromPercent);

    if (calcMode == CalcModeSpline) {
        ASSERT(m_keySplines.size() == m_values.size() - 1);
        effectivePercent = calculatePercentForSpline(effectivePercent, index);
    }
}
    
void SVGAnimationElement::startedActiveInterval()
{
    m_animationValid = false;

    if (!hasValidAttributeType())
        return;

    // These validations are appropriate for all animation modes.
    if (fastHasAttribute(SVGNames::keyPointsAttr) && m_keyPoints.size() != m_keyTimes.size())
        return;

    AnimationMode animationMode = this->animationMode();
    CalcMode calcMode = this->calcMode();
    if (calcMode == CalcModeSpline) {
        unsigned splinesCount = m_keySplines.size() + 1;
        if ((fastHasAttribute(SVGNames::keyPointsAttr) && m_keyPoints.size() != splinesCount)
            || (animationMode == ValuesAnimation && m_values.size() != splinesCount)
            || (fastHasAttribute(SVGNames::keyTimesAttr) && m_keyTimes.size() != splinesCount))
            return;
    }

    String from = fromValue();
    String to = toValue();
    String by = byValue();
    if (animationMode == NoAnimation)
        return;
    if (animationMode == FromToAnimation)
        m_animationValid = calculateFromAndToValues(from, to);
    else if (animationMode == ToAnimation) {
        // For to-animations the from value is the current accumulated value from lower priority animations.
        // The value is not static and is determined during the animation.
        m_animationValid = calculateFromAndToValues(emptyString(), to);
    } else if (animationMode == FromByAnimation)
        m_animationValid = calculateFromAndByValues(from, by);
    else if (animationMode == ByAnimation)
        m_animationValid = calculateFromAndByValues(emptyString(), by);
    else if (animationMode == ValuesAnimation) {
        m_animationValid = m_values.size() >= 1
            && (calcMode == CalcModePaced || !fastHasAttribute(SVGNames::keyTimesAttr) || fastHasAttribute(SVGNames::keyPointsAttr) || (m_values.size() == m_keyTimes.size()))
            && (calcMode == CalcModeDiscrete || !m_keyTimes.size() || m_keyTimes.last() == 1)
            && (calcMode != CalcModeSpline || ((m_keySplines.size() && (m_keySplines.size() == m_values.size() - 1)) || m_keySplines.size() == m_keyPoints.size() - 1))
            && (!fastHasAttribute(SVGNames::keyPointsAttr) || (m_keyTimes.size() > 1 && m_keyTimes.size() == m_keyPoints.size()));
        if (m_animationValid)
            m_animationValid = calculateToAtEndOfDurationValue(m_values.last());
        if (calcMode == CalcModePaced && m_animationValid)
            calculateKeyTimesForCalcModePaced();
    } else if (animationMode == PathAnimation)
        m_animationValid = calcMode == CalcModePaced || !fastHasAttribute(SVGNames::keyPointsAttr) || (m_keyTimes.size() > 1 && m_keyTimes.size() == m_keyPoints.size());
}

void SVGAnimationElement::updateAnimation(float percent, unsigned repeatCount, SVGSMILElement* resultElement)
{    
    if (!m_animationValid)
        return;

    float effectivePercent;
    CalcMode calcMode = this->calcMode();
    AnimationMode animationMode = this->animationMode();
    if (animationMode == ValuesAnimation) {
        String from;
        String to;
        currentValuesForValuesAnimation(percent, effectivePercent, from, to);
        if (from != m_lastValuesAnimationFrom || to != m_lastValuesAnimationTo) {
            m_animationValid = calculateFromAndToValues(from, to);
            if (!m_animationValid)
                return;
            m_lastValuesAnimationFrom = from;
            m_lastValuesAnimationTo = to;
        }
    } else if (!m_keyPoints.isEmpty() && calcMode != CalcModePaced)
        effectivePercent = calculatePercentFromKeyPoints(percent);
    else if (m_keyPoints.isEmpty() && calcMode == CalcModeSpline && m_keyTimes.size() > 1)
        effectivePercent = calculatePercentForSpline(percent, calculateKeyTimesIndex(percent));
    else if (animationMode == FromToAnimation || animationMode == ToAnimation)
        effectivePercent = calculatePercentForFromTo(percent);
    else
        effectivePercent = percent;

    calculateAnimatedValue(effectivePercent, repeatCount, resultElement);
}

void SVGAnimationElement::computeCSSPropertyValue(SVGElement* element, CSSPropertyID id, String& valueString)
{
    ASSERT(element);
    ASSERT(element->isSVGStyledElement());

    // Don't include any properties resulting from CSS Transitions/Animations or SMIL animations, as we want to retrieve the "base value".
    element->setUseOverrideComputedStyle(true);
    RefPtr<CSSValue> value = ComputedStyleExtractor(element).propertyValue(id);
    valueString = value ? value->cssText() : String();
    element->setUseOverrideComputedStyle(false);
}

void SVGAnimationElement::adjustForInheritance(SVGElement* targetElement, const QualifiedName& attributeName, String& value)
{
    // FIXME: At the moment the computed style gets returned as a String and needs to get parsed again.
    // In the future we might want to work with the value type directly to avoid the String parsing.
    ASSERT(targetElement);

    Element* parent = targetElement->parentElement();
    if (!parent || !parent->isSVGElement())
        return;

    SVGElement* svgParent = toSVGElement(parent);
    if (!svgParent->isSVGStyledElement())
        return;
    computeCSSPropertyValue(svgParent, cssPropertyID(attributeName.localName()), value);
}

static bool inheritsFromProperty(SVGElement* targetElement, const QualifiedName& attributeName, const String& value)
{
    ASSERT(targetElement);
    DEFINE_STATIC_LOCAL(const AtomicString, inherit, ("inherit", AtomicString::ConstructFromLiteral));
    
    if (value.isEmpty() || value != inherit || !targetElement->isSVGStyledElement())
        return false;
    return SVGStyledElement::isAnimatableCSSProperty(attributeName);
}

void SVGAnimationElement::determinePropertyValueTypes(const String& from, const String& to)
{
    SVGElement* targetElement = this->targetElement();
    ASSERT(targetElement);

    const QualifiedName& attributeName = this->attributeName();
    if (inheritsFromProperty(targetElement, attributeName, from))
        m_fromPropertyValueType = InheritValue;
    if (inheritsFromProperty(targetElement, attributeName, to))
        m_toPropertyValueType = InheritValue;
}

void SVGAnimationElement::setTargetElement(SVGElement* target)
{
    SVGSMILElement::setTargetElement(target);
    checkInvalidCSSAttributeType(target);
}

void SVGAnimationElement::setAttributeName(const QualifiedName& attributeName)
{
    SVGSMILElement::setAttributeName(attributeName);
    checkInvalidCSSAttributeType(targetElement());
}

void SVGAnimationElement::checkInvalidCSSAttributeType(SVGElement* target)
{
    m_hasInvalidCSSAttributeType = target && hasValidAttributeName() && attributeType() == AttributeTypeCSS && !isTargetAttributeCSSProperty(target, attributeName());
}

}

#endif // ENABLE(SVG)
