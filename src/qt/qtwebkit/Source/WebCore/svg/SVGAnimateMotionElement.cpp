/*
 * Copyright (C) 2007 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2007 Rob Buis <buis@kde.org>
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

#if ENABLE(SVG)
#include "SVGAnimateMotionElement.h"

#include "AffineTransform.h"
#include "Attribute.h"
#include "RenderObject.h"
#include "RenderSVGResource.h"
#include "SVGElementInstance.h"
#include "SVGImageElement.h"
#include "SVGMPathElement.h"
#include "SVGNames.h"
#include "SVGParserUtilities.h"
#include "SVGPathData.h"
#include "SVGPathElement.h"
#include "SVGPathUtilities.h"
#include "SVGTransformList.h"
#include <wtf/MathExtras.h>
#include <wtf/StdLibExtras.h>

namespace WebCore {
    
using namespace SVGNames;

inline SVGAnimateMotionElement::SVGAnimateMotionElement(const QualifiedName& tagName, Document* document)
    : SVGAnimationElement(tagName, document)
    , m_hasToPointAtEndOfDuration(false)
{
    setCalcMode(CalcModePaced);
    ASSERT(hasTagName(animateMotionTag));
}

PassRefPtr<SVGAnimateMotionElement> SVGAnimateMotionElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new SVGAnimateMotionElement(tagName, document));
}

bool SVGAnimateMotionElement::hasValidAttributeType()
{
    SVGElement* targetElement = this->targetElement();
    if (!targetElement)
        return false;

    // We don't have a special attribute name to verify the animation type. Check the element name instead.
    if (!targetElement->isSVGGraphicsElement())
        return false;
    // Spec: SVG 1.1 section 19.2.15
    // FIXME: svgTag is missing. Needs to be checked, if transforming <svg> could cause problems.
    if (targetElement->hasTagName(gTag)
        || targetElement->hasTagName(defsTag)
        || targetElement->hasTagName(useTag)
        || isSVGImageElement(targetElement)
        || targetElement->hasTagName(switchTag)
        || targetElement->hasTagName(pathTag)
        || targetElement->hasTagName(rectTag)
        || targetElement->hasTagName(circleTag)
        || targetElement->hasTagName(ellipseTag)
        || targetElement->hasTagName(lineTag)
        || targetElement->hasTagName(polylineTag)
        || targetElement->hasTagName(polygonTag)
        || targetElement->hasTagName(textTag)
        || targetElement->hasTagName(clipPathTag)
        || targetElement->hasTagName(maskTag)
        || targetElement->hasTagName(SVGNames::aTag)
        || targetElement->hasTagName(foreignObjectTag)
        )
        return true;
    return false;
}

bool SVGAnimateMotionElement::hasValidAttributeName()
{
    // AnimateMotion does not use attributeName so it is always valid.
    return true;
}

bool SVGAnimateMotionElement::isSupportedAttribute(const QualifiedName& attrName)
{
    DEFINE_STATIC_LOCAL(HashSet<QualifiedName>, supportedAttributes, ());
    if (supportedAttributes.isEmpty())
        supportedAttributes.add(SVGNames::pathAttr);
    return supportedAttributes.contains<SVGAttributeHashTranslator>(attrName);
}

void SVGAnimateMotionElement::parseAttribute(const QualifiedName& name, const AtomicString& value)
{
    if (!isSupportedAttribute(name)) {
        SVGAnimationElement::parseAttribute(name, value);
        return;
    }

    if (name == SVGNames::pathAttr) {
        m_path = Path();
        buildPathFromString(value, m_path);
        updateAnimationPath();
        return;
    }

    ASSERT_NOT_REACHED();
}
    
SVGAnimateMotionElement::RotateMode SVGAnimateMotionElement::rotateMode() const
{
    DEFINE_STATIC_LOCAL(const AtomicString, autoVal, ("auto", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(const AtomicString, autoReverse, ("auto-reverse", AtomicString::ConstructFromLiteral));
    const AtomicString& rotate = getAttribute(SVGNames::rotateAttr);
    if (rotate == autoVal)
        return RotateAuto;
    if (rotate == autoReverse)
        return RotateAutoReverse;
    return RotateAngle;
}

void SVGAnimateMotionElement::updateAnimationPath()
{
    m_animationPath = Path();
    bool foundMPath = false;

    for (Node* child = firstChild(); child; child = child->nextSibling()) {
        if (child->hasTagName(SVGNames::mpathTag)) {
            SVGMPathElement* mPath = static_cast<SVGMPathElement*>(child);
            SVGPathElement* pathElement = mPath->pathElement();
            if (pathElement) {
                updatePathFromGraphicsElement(pathElement, m_animationPath);
                foundMPath = true;
                break;
            }
        }
    }

    if (!foundMPath && fastHasAttribute(SVGNames::pathAttr))
        m_animationPath = m_path;

    updateAnimationMode();
}

static bool parsePoint(const String& s, FloatPoint& point)
{
    if (s.isEmpty())
        return false;
    const UChar* cur = s.characters();
    const UChar* end = cur + s.length();
    
    if (!skipOptionalSVGSpaces(cur, end))
        return false;
    
    float x = 0;
    if (!parseNumber(cur, end, x))
        return false;
    
    float y = 0;
    if (!parseNumber(cur, end, y))
        return false;
    
    point = FloatPoint(x, y);
    
    // disallow anything except spaces at the end
    return !skipOptionalSVGSpaces(cur, end);
}
    
void SVGAnimateMotionElement::resetAnimatedType()
{
    if (!hasValidAttributeType())
        return;
    SVGElement* targetElement = this->targetElement();
    if (!targetElement)
        return;
    if (AffineTransform* transform = targetElement->supplementalTransform())
        transform->makeIdentity();
}

void SVGAnimateMotionElement::clearAnimatedType(SVGElement* targetElement)
{
    if (!targetElement)
        return;
    if (AffineTransform* transform = targetElement->supplementalTransform())
        transform->makeIdentity();
}

bool SVGAnimateMotionElement::calculateToAtEndOfDurationValue(const String& toAtEndOfDurationString)
{
    parsePoint(toAtEndOfDurationString, m_toPointAtEndOfDuration);
    m_hasToPointAtEndOfDuration = true;
    return true;
}

bool SVGAnimateMotionElement::calculateFromAndToValues(const String& fromString, const String& toString)
{
    m_hasToPointAtEndOfDuration = false;
    parsePoint(fromString, m_fromPoint);
    parsePoint(toString, m_toPoint);
    return true;
}
    
bool SVGAnimateMotionElement::calculateFromAndByValues(const String& fromString, const String& byString)
{
    m_hasToPointAtEndOfDuration = false;
    if (animationMode() == ByAnimation && !isAdditive())
        return false;
    parsePoint(fromString, m_fromPoint);
    FloatPoint byPoint;
    parsePoint(byString, byPoint);
    m_toPoint = FloatPoint(m_fromPoint.x() + byPoint.x(), m_fromPoint.y() + byPoint.y());
    return true;
}

void SVGAnimateMotionElement::buildTransformForProgress(AffineTransform* transform, float percentage)
{
    ASSERT(!m_animationPath.isEmpty());

    bool ok = false;
    float positionOnPath = m_animationPath.length() * percentage;
    FloatPoint position = m_animationPath.pointAtLength(positionOnPath, ok);
    if (!ok)
        return;
    transform->translate(position.x(), position.y());
    RotateMode rotateMode = this->rotateMode();
    if (rotateMode != RotateAuto && rotateMode != RotateAutoReverse)
        return;
    float angle = m_animationPath.normalAngleAtLength(positionOnPath, ok);
    if (rotateMode == RotateAutoReverse)
        angle += 180;
    transform->rotate(angle);
}

void SVGAnimateMotionElement::calculateAnimatedValue(float percentage, unsigned repeatCount, SVGSMILElement*)
{
    SVGElement* targetElement = this->targetElement();
    if (!targetElement)
        return;
    AffineTransform* transform = targetElement->supplementalTransform();
    if (!transform)
        return;

    if (RenderObject* targetRenderer = targetElement->renderer())
        targetRenderer->setNeedsTransformUpdate();

    if (!isAdditive())
        transform->makeIdentity();

    if (animationMode() != PathAnimation) {
        FloatPoint toPointAtEndOfDuration = m_toPoint;
        if (isAccumulated() && repeatCount && m_hasToPointAtEndOfDuration)
            toPointAtEndOfDuration = m_toPointAtEndOfDuration;

        float animatedX = 0;
        animateAdditiveNumber(percentage, repeatCount, m_fromPoint.x(), m_toPoint.x(), toPointAtEndOfDuration.x(), animatedX);

        float animatedY = 0;
        animateAdditiveNumber(percentage, repeatCount, m_fromPoint.y(), m_toPoint.y(), toPointAtEndOfDuration.y(), animatedY);

        transform->translate(animatedX, animatedY);
        return;
    }

    buildTransformForProgress(transform, percentage);

    // Handle accumulate="sum".
    if (isAccumulated() && repeatCount) {
        for (unsigned i = 0; i < repeatCount; ++i)
            buildTransformForProgress(transform, 1);
    }
}

void SVGAnimateMotionElement::applyResultsToTarget()
{
    // We accumulate to the target element transform list so there is not much to do here.
    SVGElement* targetElement = this->targetElement();
    if (!targetElement)
        return;

    if (RenderObject* renderer = targetElement->renderer())
        RenderSVGResource::markForLayoutAndParentResourceInvalidation(renderer);

    AffineTransform* t = targetElement->supplementalTransform();
    if (!t)
        return;

    // ...except in case where we have additional instances in <use> trees.
    const HashSet<SVGElementInstance*>& instances = targetElement->instancesForElement();
    const HashSet<SVGElementInstance*>::const_iterator end = instances.end();
    for (HashSet<SVGElementInstance*>::const_iterator it = instances.begin(); it != end; ++it) {
        SVGElement* shadowTreeElement = (*it)->shadowTreeElement();
        ASSERT(shadowTreeElement);
        AffineTransform* transform = shadowTreeElement->supplementalTransform();
        if (!transform)
            continue;
        transform->setMatrix(t->a(), t->b(), t->c(), t->d(), t->e(), t->f());
        if (RenderObject* renderer = shadowTreeElement->renderer()) {
            renderer->setNeedsTransformUpdate();
            RenderSVGResource::markForLayoutAndParentResourceInvalidation(renderer);
        }
    }
}

float SVGAnimateMotionElement::calculateDistance(const String& fromString, const String& toString)
{
    FloatPoint from;
    FloatPoint to;
    if (!parsePoint(fromString, from))
        return -1;
    if (!parsePoint(toString, to))
        return -1;
    FloatSize diff = to - from;
    return sqrtf(diff.width() * diff.width() + diff.height() * diff.height());
}

void SVGAnimateMotionElement::updateAnimationMode()
{
    if (!m_animationPath.isEmpty())
        setAnimationMode(PathAnimation);
    else
        SVGAnimationElement::updateAnimationMode();
}

}
#endif // ENABLE(SVG)
