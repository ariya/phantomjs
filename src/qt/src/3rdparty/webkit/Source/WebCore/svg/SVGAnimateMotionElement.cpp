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

#if ENABLE(SVG) && ENABLE(SVG_ANIMATION)
#include "SVGAnimateMotionElement.h"

#include "Attribute.h"
#include "RenderObject.h"
#include "RenderSVGResource.h"
#include "SVGElementInstance.h"
#include "SVGMPathElement.h"
#include "SVGNames.h"
#include "SVGParserUtilities.h"
#include "SVGPathElement.h"
#include "SVGPathParserFactory.h"
#include "SVGTransformList.h"
#include <wtf/MathExtras.h>
#include <wtf/StdLibExtras.h>

namespace WebCore {
    
using namespace SVGNames;

inline SVGAnimateMotionElement::SVGAnimateMotionElement(const QualifiedName& tagName, Document* document)
    : SVGAnimationElement(tagName, document)
    , m_baseIndexInTransformList(0)
    , m_angle(0)
{
}

PassRefPtr<SVGAnimateMotionElement> SVGAnimateMotionElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new SVGAnimateMotionElement(tagName, document));
}

bool SVGAnimateMotionElement::hasValidAttributeType() const
{
    SVGElement* targetElement = this->targetElement();
    if (!targetElement)
        return false;

    // We don't have a special attribute name to verify the animation type. Check the element name instead.
    if (!targetElement->isStyledTransformable() && !targetElement->hasTagName(SVGNames::textTag))
        return false;
    // Spec: SVG 1.1 section 19.2.15
    // FIXME: svgTag is missing. Needs to be checked, if transforming <svg> could cause problems.
    if (targetElement->hasTagName(gTag)
        || targetElement->hasTagName(defsTag)
        || targetElement->hasTagName(useTag)
        || targetElement->hasTagName(imageTag)
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
        || targetElement->hasTagName(aTag)
#if ENABLE(SVG_FOREIGN_OBJECT)
        || targetElement->hasTagName(foreignObjectTag)
#endif
        )
        return true;
    return false;
}

void SVGAnimateMotionElement::parseMappedAttribute(Attribute* attr)
{
    if (attr->name() == SVGNames::pathAttr) {
        m_path = Path();
        SVGPathParserFactory* factory = SVGPathParserFactory::self();
        factory->buildPathFromString(attr->value(), m_path);
    } else
        SVGAnimationElement::parseMappedAttribute(attr);
}
    
SVGAnimateMotionElement::RotateMode SVGAnimateMotionElement::rotateMode() const
{
    DEFINE_STATIC_LOCAL(const AtomicString, autoVal, ("auto"));
    DEFINE_STATIC_LOCAL(const AtomicString, autoReverse, ("auto-reverse"));
    String rotate = getAttribute(SVGNames::rotateAttr);
    if (rotate == autoVal)
        return RotateAuto;
    if (rotate == autoReverse)
        return RotateAutoReverse;
    return RotateAngle;
}

Path SVGAnimateMotionElement::animationPath() const
{
    for (Node* child = firstChild(); child; child = child->nextSibling()) {
        if (child->hasTagName(SVGNames::mpathTag)) {
            SVGMPathElement* mPath = static_cast<SVGMPathElement*>(child);
            SVGPathElement* pathElement = mPath->pathElement();
            Path path;
            if (pathElement)
                pathElement->toPathData(path);
            return path;
        }
    }
    if (hasAttribute(SVGNames::pathAttr))
        return m_path;
    return Path();
}

static bool parsePoint(const String& s, FloatPoint& point)
{
    if (s.isEmpty())
        return false;
    const UChar* cur = s.characters();
    const UChar* end = cur + s.length();
    
    if (!skipOptionalSpaces(cur, end))
        return false;
    
    float x = 0;
    if (!parseNumber(cur, end, x))
        return false;
    
    float y = 0;
    if (!parseNumber(cur, end, y))
        return false;
    
    point = FloatPoint(x, y);
    
    // disallow anything except spaces at the end
    return !skipOptionalSpaces(cur, end);
}
    
void SVGAnimateMotionElement::resetToBaseValue(const String&)
{
    if (!hasValidAttributeType())
        return;
    AffineTransform* transform = targetElement()->supplementalTransform();
    if (!transform)
        return;
    transform->makeIdentity();
}

bool SVGAnimateMotionElement::calculateFromAndToValues(const String& fromString, const String& toString)
{
    parsePoint(fromString, m_fromPoint);
    parsePoint(toString, m_toPoint);
    return true;
}
    
bool SVGAnimateMotionElement::calculateFromAndByValues(const String& fromString, const String& byString)
{
    parsePoint(fromString, m_fromPoint);
    FloatPoint byPoint;
    parsePoint(byString, byPoint);
    m_toPoint = FloatPoint(m_fromPoint.x() + byPoint.x(), m_fromPoint.y() + byPoint.y());
    return true;
}

void SVGAnimateMotionElement::calculateAnimatedValue(float percentage, unsigned, SVGSMILElement*)
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
    
    // FIXME: Implement accumulate.
    
    if (animationMode() == PathAnimation) {
        ASSERT(!animationPath().isEmpty());
        Path path = animationPath();
        float positionOnPath = path.length() * percentage;
        bool ok;
        FloatPoint position = path.pointAtLength(positionOnPath, ok);
        if (ok) {
            transform->translate(position.x(), position.y());
            RotateMode rotateMode = this->rotateMode();
            if (rotateMode == RotateAuto || rotateMode == RotateAutoReverse) {
                float angle = path.normalAngleAtLength(positionOnPath, ok);
                if (rotateMode == RotateAutoReverse)
                    angle += 180;
                transform->rotate(angle);
            }
        }
        return;
    }
    FloatSize diff = m_toPoint - m_fromPoint;
    transform->translate(diff.width() * percentage + m_fromPoint.x(), diff.height() * percentage + m_fromPoint.y());
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

}
#endif // ENABLE(SVG)
