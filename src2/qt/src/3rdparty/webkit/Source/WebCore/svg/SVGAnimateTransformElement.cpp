/*
 * Copyright (C) 2004, 2005 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006, 2007 Rob Buis <buis@kde.org>
 * Copyright (C) 2007 Eric Seidel <eric@webkit.org>
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
#include "SVGAnimateTransformElement.h"

#include "AffineTransform.h"
#include "Attribute.h"
#include "RenderObject.h"
#include "RenderSVGResource.h"
#include "SVGAngle.h"
#include "SVGElementInstance.h"
#include "SVGGradientElement.h"
#include "SVGNames.h"
#include "SVGParserUtilities.h"
#include "SVGSVGElement.h"
#include "SVGStyledTransformableElement.h"
#include "SVGTextElement.h"
#include "SVGTransform.h"
#include "SVGTransformList.h"
#include "SVGUseElement.h"
#include <math.h>
#include <wtf/MathExtras.h>

using namespace std;

namespace WebCore {

inline SVGAnimateTransformElement::SVGAnimateTransformElement(const QualifiedName& tagName, Document* document)
    : SVGAnimationElement(tagName, document)
    , m_type(SVGTransform::SVG_TRANSFORM_UNKNOWN)
    , m_baseIndexInTransformList(0)
{
}

PassRefPtr<SVGAnimateTransformElement> SVGAnimateTransformElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new SVGAnimateTransformElement(tagName, document));
}

bool SVGAnimateTransformElement::hasValidAttributeType() const
{
    SVGElement* targetElement = this->targetElement();
    if (!targetElement)
        return false;
    
    return determineAnimatedAttributeType(targetElement) == AnimatedTransformList;
}

AnimatedAttributeType SVGAnimateTransformElement::determineAnimatedAttributeType(SVGElement* targetElement) const
{
    ASSERT(targetElement);
    
    // Just transform lists can be animated with <animateTransform>
    // http://www.w3.org/TR/SVG/animate.html#AnimationAttributesAndProperties
    if (targetElement->animatedPropertyTypeForAttribute(attributeName()) != AnimatedTransformList)
        return AnimatedUnknown;

    return AnimatedTransformList;
}

void SVGAnimateTransformElement::parseMappedAttribute(Attribute* attr)
{
    if (attr->name() == SVGNames::typeAttr) {
        if (attr->value() == "translate")
            m_type = SVGTransform::SVG_TRANSFORM_TRANSLATE;
        else if (attr->value() == "scale")
            m_type = SVGTransform::SVG_TRANSFORM_SCALE;
        else if (attr->value() == "rotate")
            m_type = SVGTransform::SVG_TRANSFORM_ROTATE;
        else if (attr->value() == "skewX")
            m_type = SVGTransform::SVG_TRANSFORM_SKEWX;
        else if (attr->value() == "skewY")
            m_type = SVGTransform::SVG_TRANSFORM_SKEWY;
    } else
        SVGAnimationElement::parseMappedAttribute(attr);
}

    
static SVGTransformList* transformListFor(SVGElement* element)
{
    ASSERT(element);
    if (element->isStyledTransformable())
        return &static_cast<SVGStyledTransformableElement*>(element)->transform();
    if (element->hasTagName(SVGNames::textTag))
        return &static_cast<SVGTextElement*>(element)->transform();
    if (element->hasTagName(SVGNames::linearGradientTag) || element->hasTagName(SVGNames::radialGradientTag))
        return &static_cast<SVGGradientElement*>(element)->gradientTransform();
    // FIXME: Handle patternTransform, which is obviously missing!
    return 0;
}
    
void SVGAnimateTransformElement::resetToBaseValue(const String& baseValue)
{
    SVGElement* targetElement = this->targetElement();
    if (!targetElement || determineAnimatedAttributeType(targetElement) == AnimatedUnknown)
        return;

    if (targetElement->hasTagName(SVGNames::linearGradientTag) || targetElement->hasTagName(SVGNames::radialGradientTag)) {
        targetElement->setAttribute(SVGNames::gradientTransformAttr, baseValue.isEmpty() ? "matrix(1 0 0 1 0 0)" : baseValue);
        return;
    }

    if (baseValue.isEmpty()) {
        if (SVGTransformList* list = transformListFor(targetElement))
            list->clear();
    } else
        targetElement->setAttribute(SVGNames::transformAttr, baseValue);
}

void SVGAnimateTransformElement::calculateAnimatedValue(float percentage, unsigned repeat, SVGSMILElement*)
{
    SVGElement* targetElement = this->targetElement();
    if (!targetElement || determineAnimatedAttributeType(targetElement) == AnimatedUnknown)
        return;
    SVGTransformList* transformList = transformListFor(targetElement);
    ASSERT(transformList);

    if (!isAdditive())
        transformList->clear();
    if (isAccumulated() && repeat) {
        SVGTransform accumulatedTransform = SVGTransformDistance(m_fromTransform, m_toTransform).scaledDistance(repeat).addToSVGTransform(SVGTransform());
        transformList->append(accumulatedTransform);
    }
    SVGTransform transform = SVGTransformDistance(m_fromTransform, m_toTransform).scaledDistance(percentage).addToSVGTransform(m_fromTransform);
    transformList->append(transform);
}
    
bool SVGAnimateTransformElement::calculateFromAndToValues(const String& fromString, const String& toString)
{
    m_fromTransform = parseTransformValue(fromString);
    if (!m_fromTransform.isValid())
        return false;
    m_toTransform = parseTransformValue(toString);
    return m_toTransform.isValid();
}

bool SVGAnimateTransformElement::calculateFromAndByValues(const String& fromString, const String& byString)
{
    m_fromTransform = parseTransformValue(fromString);
    if (!m_fromTransform.isValid())
        return false;
    m_toTransform = SVGTransformDistance::addSVGTransforms(m_fromTransform, parseTransformValue(byString));
    return m_toTransform.isValid();
}

SVGTransform SVGAnimateTransformElement::parseTransformValue(const String& value) const
{
    if (value.isEmpty())
        return SVGTransform(m_type);
    SVGTransform result;
    // FIXME: This is pretty dumb but parseTransformValue() wants those parenthesis.
    String parseString("(" + value + ")");
    const UChar* ptr = parseString.characters();
    SVGTransformable::parseTransformValue(m_type, ptr, ptr + parseString.length(), result); // ignoring return value
    return result;
}
    
void SVGAnimateTransformElement::applyResultsToTarget()
{
    SVGElement* targetElement = this->targetElement();
    if (!targetElement || determineAnimatedAttributeType(targetElement) == AnimatedUnknown)
        return;

    // We accumulate to the target element transform list so there is not much to do here.
    if (RenderObject* renderer = targetElement->renderer()) {
        renderer->setNeedsTransformUpdate();
        RenderSVGResource::markForLayoutAndParentResourceInvalidation(renderer);
    }

    // ...except in case where we have additional instances in <use> trees.
    SVGTransformList* transformList = transformListFor(targetElement);
    if (!transformList)
        return;

    const HashSet<SVGElementInstance*>& instances = targetElement->instancesForElement();
    const HashSet<SVGElementInstance*>::const_iterator end = instances.end();
    for (HashSet<SVGElementInstance*>::const_iterator it = instances.begin(); it != end; ++it) {
        SVGElement* shadowTreeElement = (*it)->shadowTreeElement();
        ASSERT(shadowTreeElement);
        if (shadowTreeElement->isStyledTransformable())
            static_cast<SVGStyledTransformableElement*>(shadowTreeElement)->setTransformBaseValue(*transformList);
        else if (shadowTreeElement->hasTagName(SVGNames::textTag))
            static_cast<SVGTextElement*>(shadowTreeElement)->setTransformBaseValue(*transformList);
        else if (shadowTreeElement->hasTagName(SVGNames::linearGradientTag) || shadowTreeElement->hasTagName(SVGNames::radialGradientTag))
            static_cast<SVGGradientElement*>(shadowTreeElement)->setGradientTransformBaseValue(*transformList);
        // FIXME: Handle patternTransform, obviously missing!
        if (RenderObject* renderer = shadowTreeElement->renderer()) {
            renderer->setNeedsTransformUpdate();
            RenderSVGResource::markForLayoutAndParentResourceInvalidation(renderer);
        }
    }
}
    
float SVGAnimateTransformElement::calculateDistance(const String& fromString, const String& toString)
{
    // FIXME: This is not correct in all cases. The spec demands that each component (translate x and y for example) 
    // is paced separately. To implement this we need to treat each component as individual animation everywhere.
    SVGTransform from = parseTransformValue(fromString);
    if (!from.isValid())
        return -1;
    SVGTransform to = parseTransformValue(toString);
    if (!to.isValid() || from.type() != to.type())
        return -1;
    if (to.type() == SVGTransform::SVG_TRANSFORM_TRANSLATE) {
        FloatSize diff = to.translate() - from.translate();
        return sqrtf(diff.width() * diff.width() + diff.height() * diff.height());
    }
    if (to.type() == SVGTransform::SVG_TRANSFORM_ROTATE)
        return fabsf(to.angle() - from.angle());
    if (to.type() == SVGTransform::SVG_TRANSFORM_SCALE) {
        FloatSize diff = to.scale() - from.scale();
        return sqrtf(diff.width() * diff.width() + diff.height() * diff.height());
    }
    return -1;
}

}
#endif // ENABLE(SVG)
