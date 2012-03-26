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

class SVGViewSpec : public SVGFitToViewBox,
                    public SVGZoomAndPan {
    WTF_MAKE_NONCOPYABLE(SVGViewSpec);
public:
    SVGViewSpec(SVGElement*);

    bool parseViewSpec(const String&);

    void setTransform(const String&);
    SVGTransformList transform() const { return m_transform; }

    void setViewBoxString(const String&);

    void setPreserveAspectRatioString(const String&);

    void setViewTargetString(const String&);
    String viewTargetString() const { return m_viewTargetString; }
    SVGElement* viewTarget() const;

    SVGElement* contextElement() const { return const_cast<SVGElement*>(m_contextElement); }

private:
    SVGElement* m_contextElement;

    // Animated property declarations

    // SVGFitToViewBox
    DECLARE_ANIMATED_RECT(ViewBox, viewBox)
    DECLARE_ANIMATED_PRESERVEASPECTRATIO(PreserveAspectRatio, preserveAspectRatio)

    SVGTransformList m_transform;
    String m_viewTargetString;
};

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
