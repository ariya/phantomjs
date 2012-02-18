/*
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

#ifndef SVGAnimateMotionElement_h
#define SVGAnimateMotionElement_h
#if ENABLE(SVG_ANIMATION)
#include "AffineTransform.h"
#include "Path.h"
#include "SVGAnimationElement.h"

namespace WebCore {
            
class SVGAnimateMotionElement : public SVGAnimationElement {
public:
    static PassRefPtr<SVGAnimateMotionElement> create(const QualifiedName&, Document*);

private:
    SVGAnimateMotionElement(const QualifiedName&, Document*);

    virtual bool hasValidAttributeType() const;

    virtual void parseMappedAttribute(Attribute*);

    virtual void resetToBaseValue(const String&);
    virtual bool calculateFromAndToValues(const String& fromString, const String& toString);
    virtual bool calculateFromAndByValues(const String& fromString, const String& byString);
    virtual void calculateAnimatedValue(float percentage, unsigned repeat, SVGSMILElement* resultElement);
    virtual void applyResultsToTarget();
    virtual float calculateDistance(const String& fromString, const String& toString);
    virtual Path animationPath() const;

    enum RotateMode {
        RotateAngle,
        RotateAuto,
        RotateAutoReverse
    };
    RotateMode rotateMode() const;

    FloatSize m_animatedTranslation;
    float m_animatedAngle;

    // Note: we do not support percentage values for to/from coords as the spec implies we should (opera doesn't either)
    FloatPoint m_fromPoint;
    float m_fromAngle;
    FloatPoint m_toPoint;
    float m_toAngle;

    unsigned m_baseIndexInTransformList;

    Path m_path;
    Vector<float> m_keyPoints;
    float m_angle;
};
    
} // namespace WebCore

#endif // ENABLE(SVG_ANIMATION)
#endif // SVGAnimateMotionElement_h
