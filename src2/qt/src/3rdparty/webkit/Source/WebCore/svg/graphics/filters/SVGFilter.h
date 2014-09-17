/*
 * Copyright (C) 2009 Dirk Schulze <krit@webkit.org>
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

#ifndef SVGFilter_h
#define SVGFilter_h

#if ENABLE(SVG) && ENABLE(FILTERS)
#include "AffineTransform.h"
#include "Filter.h"
#include "FilterEffect.h"
#include "FloatRect.h"
#include "FloatSize.h"

#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>

namespace WebCore {

class SVGFilter : public Filter {
public:
    static PassRefPtr<SVGFilter> create(const AffineTransform&, const FloatRect&, const FloatRect&, const FloatRect&, bool);

    virtual bool effectBoundingBoxMode() const { return m_effectBBoxMode; }

    virtual FloatRect filterRegionInUserSpace() const { return m_filterRegion; }
    virtual FloatRect filterRegion() const { return m_absoluteFilterRegion; }

    virtual FloatPoint mapAbsolutePointToLocalPoint(const FloatPoint& point) const { return m_absoluteTransform.inverse().mapPoint(point); }
    FloatRect mapLocalRectToAbsoluteRect(const FloatRect& rect) const { return m_absoluteTransform.mapRect(rect); }

    virtual float applyHorizontalScale(float value) const;
    virtual float applyVerticalScale(float value) const;

    virtual FloatRect sourceImageRect() const { return m_absoluteSourceDrawingRegion; }
    FloatRect targetBoundingBox() const { return m_targetBoundingBox; }

private:
    SVGFilter(const AffineTransform& absoluteTransform, const FloatRect& absoluteSourceDrawingRegion, const FloatRect& targetBoundingBox, const FloatRect& filterRegion, bool effectBBoxMode);

    AffineTransform m_absoluteTransform;
    FloatRect m_absoluteSourceDrawingRegion;
    FloatRect m_targetBoundingBox;
    FloatRect m_absoluteFilterRegion;
    FloatRect m_filterRegion;
    bool m_effectBBoxMode;
};

} // namespace WebCore

#endif // ENABLE(SVG) && ENABLE(FILTERS)

#endif // SVGFilter_h
