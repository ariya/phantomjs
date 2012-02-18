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

#ifndef SVGAnimateTransformElement_h
#define SVGAnimateTransformElement_h
#if ENABLE(SVG) && ENABLE(SVG_ANIMATION)

#include "SVGAnimationElement.h"
#include "SVGTransform.h"
#include "SVGTransformDistance.h"

namespace WebCore {

class AffineTransform;

class SVGAnimateTransformElement : public SVGAnimationElement {
public:
    static PassRefPtr<SVGAnimateTransformElement> create(const QualifiedName&, Document*);

private:
    SVGAnimateTransformElement(const QualifiedName&, Document*);
    
    virtual bool hasValidAttributeType() const;
    AnimatedAttributeType determineAnimatedAttributeType(SVGElement*) const;

    virtual void parseMappedAttribute(Attribute*);

    virtual void resetToBaseValue(const String&);
    virtual bool calculateFromAndToValues(const String& fromString, const String& toString);
    virtual bool calculateFromAndByValues(const String& fromString, const String& byString);
    virtual void calculateAnimatedValue(float percentage, unsigned repeat, SVGSMILElement* resultElement);
    virtual void applyResultsToTarget();
    virtual float calculateDistance(const String& fromString, const String& toString);

    SVGTransform parseTransformValue(const String&) const;
    
    SVGTransform::SVGTransformType m_type;
    
    unsigned m_baseIndexInTransformList;

    SVGTransform m_toTransform;
    SVGTransform m_fromTransform;
};

} // namespace WebCore

#endif // ENABLE(SVG)
#endif // SVGAnimateTransformElement_h
