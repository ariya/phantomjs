/*
 * Copyright (C) 2004, 2005, 2006, 2007, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006 Rob Buis <buis@kde.org>
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

#ifndef SVGMarkerElement_h
#define SVGMarkerElement_h

#if ENABLE(SVG)
#include "SVGAnimatedAngle.h"
#include "SVGAnimatedBoolean.h"
#include "SVGAnimatedEnumeration.h"
#include "SVGAnimatedLength.h"
#include "SVGAnimatedPreserveAspectRatio.h"
#include "SVGAnimatedRect.h"
#include "SVGExternalResourcesRequired.h"
#include "SVGFitToViewBox.h"
#include "SVGLangSpace.h"
#include "SVGStyledElement.h"

namespace WebCore {

class SVGMarkerElement : public SVGStyledElement,
                         public SVGLangSpace,
                         public SVGExternalResourcesRequired,
                         public SVGFitToViewBox {
public:
    enum SVGMarkerUnitsType {
        SVG_MARKERUNITS_UNKNOWN           = 0,
        SVG_MARKERUNITS_USERSPACEONUSE    = 1,
        SVG_MARKERUNITS_STROKEWIDTH       = 2
    };

    enum SVGMarkerOrientType {
        SVG_MARKER_ORIENT_UNKNOWN    = 0,
        SVG_MARKER_ORIENT_AUTO       = 1,
        SVG_MARKER_ORIENT_ANGLE      = 2
    };

    static PassRefPtr<SVGMarkerElement> create(const QualifiedName&, Document*);

    AffineTransform viewBoxToViewTransform(float viewWidth, float viewHeight) const;

    void setOrientToAuto();
    void setOrientToAngle(const SVGAngle&);

private:
    SVGMarkerElement(const QualifiedName&, Document*);

    virtual bool needsPendingResourceHandling() const { return false; }

    virtual void parseMappedAttribute(Attribute*);
    virtual void svgAttributeChanged(const QualifiedName&);
    virtual void synchronizeProperty(const QualifiedName&);
    virtual void fillAttributeToPropertyTypeMap();
    virtual AttributeToPropertyTypeMap& attributeToPropertyTypeMap();
    virtual void childrenChanged(bool changedByParser = false, Node* beforeChange = 0, Node* afterChange = 0, int childCountDelta = 0);

    virtual RenderObject* createRenderer(RenderArena*, RenderStyle*);

    virtual bool selfHasRelativeLengths() const;

    static const AtomicString& orientTypeIdentifier();
    static const AtomicString& orientAngleIdentifier();

    // Animated property declarations
    DECLARE_ANIMATED_LENGTH(RefX, refX)
    DECLARE_ANIMATED_LENGTH(RefY, refY)
    DECLARE_ANIMATED_LENGTH(MarkerWidth, markerWidth)
    DECLARE_ANIMATED_LENGTH(MarkerHeight, markerHeight)
    DECLARE_ANIMATED_ENUMERATION(MarkerUnits, markerUnits)
    DECLARE_ANIMATED_ENUMERATION(OrientType, orientType)
    DECLARE_ANIMATED_ANGLE(OrientAngle, orientAngle)

    // SVGExternalResourcesRequired
    DECLARE_ANIMATED_BOOLEAN(ExternalResourcesRequired, externalResourcesRequired)

    // SVGFitToViewBox
    DECLARE_ANIMATED_RECT(ViewBox, viewBox)
    DECLARE_ANIMATED_PRESERVEASPECTRATIO(PreserveAspectRatio, preserveAspectRatio)
};

}

#endif
#endif
