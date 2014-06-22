/*
 * Copyright (C) 2007 Rob Buis <buis@kde.org>
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

#ifndef SVGViewSpec_h
#define SVGViewSpec_h

#if ENABLE(SVG)
#include "SVGAnimatedPreserveAspectRatio.h"
#include "SVGAnimatedRect.h"
#include "SVGFitToViewBox.h"
#include "SVGTransformList.h"
#include "SVGZoomAndPan.h"

namespace WebCore {

class SVGElement;
class SVGTransformListPropertyTearOff;

class SVGViewSpec : public RefCounted<SVGViewSpec>
                  , public SVGZoomAndPan
                  , public SVGFitToViewBox {
public:
    virtual ~SVGViewSpec() { }

    using RefCounted<SVGViewSpec>::ref;
    using RefCounted<SVGViewSpec>::deref;

    static PassRefPtr<SVGViewSpec> create(SVGElement* contextElement)
    {
        return adoptRef(new SVGViewSpec(contextElement));
    }

    bool parseViewSpec(const String&);
    void reset();

    SVGElement* viewTarget() const;
    String viewBoxString() const;

    String preserveAspectRatioString() const;

    void setTransformString(const String&);
    String transformString() const;

    void setViewTargetString(const String& string) { m_viewTargetString = string; }
    String viewTargetString() const { return m_viewTargetString; }

    SVGZoomAndPanType zoomAndPan() const { return m_zoomAndPan; }
    void setZoomAndPan(unsigned short zoomAndPan) { setZoomAndPanBaseValue(zoomAndPan); }
    void setZoomAndPan(unsigned short, ExceptionCode&);
    void setZoomAndPanBaseValue(unsigned short zoomAndPan) { m_zoomAndPan = SVGZoomAndPan::parseFromNumber(zoomAndPan); }

    SVGElement* contextElement() const { return m_contextElement; }
    void resetContextElement() { m_contextElement = 0; }

    // Custom non-animated 'transform' property.
    SVGTransformListPropertyTearOff* transform();
    SVGTransformList transformBaseValue() const { return m_transform; }

    // Custom animated 'viewBox' property.
    PassRefPtr<SVGAnimatedRect> viewBoxAnimated();
    FloatRect& viewBox() { return m_viewBox; }
    FloatRect viewBoxBaseValue() const { return m_viewBox; }
    void setViewBoxBaseValue(const FloatRect& viewBox) { m_viewBox = viewBox; }

    // Custom animated 'preserveAspectRatio' property.
    PassRefPtr<SVGAnimatedPreserveAspectRatio> preserveAspectRatioAnimated();
    SVGPreserveAspectRatio& preserveAspectRatio() { return m_preserveAspectRatio; }
    SVGPreserveAspectRatio preserveAspectRatioBaseValue() const { return m_preserveAspectRatio; }
    void setPreserveAspectRatioBaseValue(const SVGPreserveAspectRatio& preserveAspectRatio) { m_preserveAspectRatio = preserveAspectRatio; }

private:
    SVGViewSpec(SVGElement*);

    static const SVGPropertyInfo* transformPropertyInfo();
    static const SVGPropertyInfo* viewBoxPropertyInfo();
    static const SVGPropertyInfo* preserveAspectRatioPropertyInfo();

    static const AtomicString& transformIdentifier();
    static const AtomicString& viewBoxIdentifier();
    static const AtomicString& preserveAspectRatioIdentifier();

    static PassRefPtr<SVGAnimatedProperty> lookupOrCreateTransformWrapper(SVGViewSpec* contextElement);
    static PassRefPtr<SVGAnimatedProperty> lookupOrCreateViewBoxWrapper(SVGViewSpec* contextElement);
    static PassRefPtr<SVGAnimatedProperty> lookupOrCreatePreserveAspectRatioWrapper(SVGViewSpec* contextElement);

    SVGElement* m_contextElement;
    SVGZoomAndPanType m_zoomAndPan;

    SVGTransformList m_transform;
    FloatRect m_viewBox;
    SVGPreserveAspectRatio m_preserveAspectRatio;
    String m_viewTargetString;
};

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
