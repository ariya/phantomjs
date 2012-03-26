/*
 * Copyright (C) 2004, 2005, 2006, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006, 2007 Rob Buis <buis@kde.org>
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

#ifndef SVGPathElement_h
#define SVGPathElement_h

#if ENABLE(SVG)
#include "SVGAnimatedBoolean.h"
#include "SVGAnimatedNumber.h"
#include "SVGAnimatedPathSegListPropertyTearOff.h"
#include "SVGExternalResourcesRequired.h"
#include "SVGLangSpace.h"
#include "SVGPathByteStream.h"
#include "SVGStyledTransformableElement.h"
#include "SVGTests.h"

namespace WebCore {

class SVGPathSegArcAbs;
class SVGPathSegArcRel;
class SVGPathSegClosePath;
class SVGPathSegLinetoAbs;
class SVGPathSegLinetoRel;
class SVGPathSegMovetoAbs;
class SVGPathSegMovetoRel;
class SVGPathSegCurvetoCubicAbs;
class SVGPathSegCurvetoCubicRel;
class SVGPathSegLinetoVerticalAbs;
class SVGPathSegLinetoVerticalRel;
class SVGPathSegLinetoHorizontalAbs;
class SVGPathSegLinetoHorizontalRel;
class SVGPathSegCurvetoQuadraticAbs;
class SVGPathSegCurvetoQuadraticRel;
class SVGPathSegCurvetoCubicSmoothAbs;
class SVGPathSegCurvetoCubicSmoothRel;
class SVGPathSegCurvetoQuadraticSmoothAbs;
class SVGPathSegCurvetoQuadraticSmoothRel;

class SVGPathElement : public SVGStyledTransformableElement,
                       public SVGTests,
                       public SVGLangSpace,
                       public SVGExternalResourcesRequired {
public:
    static PassRefPtr<SVGPathElement> create(const QualifiedName&, Document*);
    
    float getTotalLength();
    FloatPoint getPointAtLength(float distance);
    unsigned long getPathSegAtLength(float distance);

    PassRefPtr<SVGPathSegClosePath> createSVGPathSegClosePath(SVGPathSegRole role = PathSegUndefinedRole);
    PassRefPtr<SVGPathSegMovetoAbs> createSVGPathSegMovetoAbs(float x, float y, SVGPathSegRole role = PathSegUndefinedRole);
    PassRefPtr<SVGPathSegMovetoRel> createSVGPathSegMovetoRel(float x, float y, SVGPathSegRole role = PathSegUndefinedRole);
    PassRefPtr<SVGPathSegLinetoAbs> createSVGPathSegLinetoAbs(float x, float y, SVGPathSegRole role = PathSegUndefinedRole);
    PassRefPtr<SVGPathSegLinetoRel> createSVGPathSegLinetoRel(float x, float y, SVGPathSegRole role = PathSegUndefinedRole);
    PassRefPtr<SVGPathSegCurvetoCubicAbs> createSVGPathSegCurvetoCubicAbs(float x, float y, float x1, float y1, float x2, float y2, SVGPathSegRole role = PathSegUndefinedRole);
    PassRefPtr<SVGPathSegCurvetoCubicRel> createSVGPathSegCurvetoCubicRel(float x, float y, float x1, float y1, float x2, float y2, SVGPathSegRole role = PathSegUndefinedRole);
    PassRefPtr<SVGPathSegCurvetoQuadraticAbs> createSVGPathSegCurvetoQuadraticAbs(float x, float y, float x1, float y1, SVGPathSegRole role = PathSegUndefinedRole);
    PassRefPtr<SVGPathSegCurvetoQuadraticRel> createSVGPathSegCurvetoQuadraticRel(float x, float y, float x1, float y1, SVGPathSegRole role = PathSegUndefinedRole);
    PassRefPtr<SVGPathSegArcAbs> createSVGPathSegArcAbs(float x, float y, float r1, float r2, float angle, bool largeArcFlag, bool sweepFlag, SVGPathSegRole role = PathSegUndefinedRole);
    PassRefPtr<SVGPathSegArcRel> createSVGPathSegArcRel(float x, float y, float r1, float r2, float angle, bool largeArcFlag, bool sweepFlag, SVGPathSegRole role = PathSegUndefinedRole);
    PassRefPtr<SVGPathSegLinetoHorizontalAbs> createSVGPathSegLinetoHorizontalAbs(float x, SVGPathSegRole role = PathSegUndefinedRole);
    PassRefPtr<SVGPathSegLinetoHorizontalRel> createSVGPathSegLinetoHorizontalRel(float x, SVGPathSegRole role = PathSegUndefinedRole);
    PassRefPtr<SVGPathSegLinetoVerticalAbs> createSVGPathSegLinetoVerticalAbs(float y, SVGPathSegRole role = PathSegUndefinedRole);
    PassRefPtr<SVGPathSegLinetoVerticalRel> createSVGPathSegLinetoVerticalRel(float y, SVGPathSegRole role = PathSegUndefinedRole);
    PassRefPtr<SVGPathSegCurvetoCubicSmoothAbs> createSVGPathSegCurvetoCubicSmoothAbs(float x, float y, float x2, float y2, SVGPathSegRole role = PathSegUndefinedRole);
    PassRefPtr<SVGPathSegCurvetoCubicSmoothRel> createSVGPathSegCurvetoCubicSmoothRel(float x, float y, float x2, float y2, SVGPathSegRole role = PathSegUndefinedRole);
    PassRefPtr<SVGPathSegCurvetoQuadraticSmoothAbs> createSVGPathSegCurvetoQuadraticSmoothAbs(float x, float y, SVGPathSegRole role = PathSegUndefinedRole);
    PassRefPtr<SVGPathSegCurvetoQuadraticSmoothRel> createSVGPathSegCurvetoQuadraticSmoothRel(float x, float y, SVGPathSegRole role = PathSegUndefinedRole);

    // Used in the bindings only.
    SVGPathSegListPropertyTearOff* pathSegList();
    SVGPathSegListPropertyTearOff* animatedPathSegList();
    SVGPathSegListPropertyTearOff* normalizedPathSegList();
    SVGPathSegListPropertyTearOff* animatedNormalizedPathSegList();

    SVGPathByteStream* pathByteStream() const { return m_pathByteStream.get(); }
    SVGAnimatedProperty* animatablePathSegList() const { return m_animatablePathSegList.get(); }

    virtual void toPathData(Path&) const;
    void pathSegListChanged(SVGPathSegRole);

private:
    SVGPathElement(const QualifiedName&, Document*);

    virtual bool isValid() const { return SVGTests::isValid(); }

    virtual void parseMappedAttribute(Attribute*);
    virtual void synchronizeProperty(const QualifiedName&);
    virtual void svgAttributeChanged(const QualifiedName&);
    virtual void fillAttributeToPropertyTypeMap();
    virtual AttributeToPropertyTypeMap& attributeToPropertyTypeMap();
    virtual bool supportsMarkers() const { return true; }

    // Animated property declarations
    DECLARE_ANIMATED_NUMBER(PathLength, pathLength)

    // SVGExternalResourcesRequired
    DECLARE_ANIMATED_BOOLEAN(ExternalResourcesRequired, externalResourcesRequired)

    void synchronizeD();

protected:
    OwnPtr<SVGPathByteStream> m_pathByteStream;
    mutable SVGSynchronizableAnimatedProperty<SVGPathSegList> m_pathSegList;
    RefPtr<SVGAnimatedPathSegListPropertyTearOff> m_animatablePathSegList;
};

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
