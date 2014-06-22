/*
 * Copyright (C) 2004, 2005, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2007 Rob Buis <buis@kde.org>
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

#ifndef SVGViewElement_h
#define SVGViewElement_h

#if ENABLE(SVG)
#include "SVGAnimatedBoolean.h"
#include "SVGAnimatedPreserveAspectRatio.h"
#include "SVGAnimatedRect.h"
#include "SVGExternalResourcesRequired.h"
#include "SVGFitToViewBox.h"
#include "SVGStringList.h"
#include "SVGStyledElement.h"
#include "SVGZoomAndPan.h"

namespace WebCore {

class SVGViewElement FINAL : public SVGStyledElement,
                             public SVGExternalResourcesRequired,
                             public SVGFitToViewBox,
                             public SVGZoomAndPan {
public:
    static PassRefPtr<SVGViewElement> create(const QualifiedName&, Document*);

    using SVGStyledElement::ref;
    using SVGStyledElement::deref;

    SVGStringList& viewTarget() { return m_viewTarget; }
    SVGZoomAndPanType zoomAndPan() const { return m_zoomAndPan; }
    void setZoomAndPan(unsigned short zoomAndPan) { m_zoomAndPan = SVGZoomAndPan::parseFromNumber(zoomAndPan); }

private:
    SVGViewElement(const QualifiedName&, Document*);

    // FIXME: svgAttributeChanged missing.
    bool isSupportedAttribute(const QualifiedName&);
    virtual void parseAttribute(const QualifiedName&, const AtomicString&) OVERRIDE;

    virtual bool rendererIsNeeded(const NodeRenderingContext&) { return false; }

    BEGIN_DECLARE_ANIMATED_PROPERTIES(SVGViewElement)
        DECLARE_ANIMATED_BOOLEAN(ExternalResourcesRequired, externalResourcesRequired)
        DECLARE_ANIMATED_RECT(ViewBox, viewBox)
        DECLARE_ANIMATED_PRESERVEASPECTRATIO(PreserveAspectRatio, preserveAspectRatio)
    END_DECLARE_ANIMATED_PROPERTIES

    SVGZoomAndPanType m_zoomAndPan;
    SVGStringList m_viewTarget;
};

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
