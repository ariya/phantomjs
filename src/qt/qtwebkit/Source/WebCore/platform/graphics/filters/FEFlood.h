/*
 * Copyright (C) 2004, 2005, 2006, 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005 Rob Buis <buis@kde.org>
 * Copyright (C) 2005 Eric Seidel <eric@webkit.org>
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

#ifndef FEFlood_h
#define FEFlood_h

#if ENABLE(FILTERS)
#include "Color.h"
#include "Filter.h"
#include "FilterEffect.h"

namespace WebCore {

class FEFlood : public FilterEffect {
public:
    static PassRefPtr<FEFlood> create(Filter* filter, const Color&, float);

    Color floodColor() const;
    bool setFloodColor(const Color &);

    float floodOpacity() const;
    bool setFloodOpacity(float);

#if !USE(CG)
    // feFlood does not perform color interpolation of any kind, so the result is always in the current
    // color space regardless of the value of color-interpolation-filters.
    void setOperatingColorSpace(ColorSpace) OVERRIDE { FilterEffect::setResultColorSpace(ColorSpaceDeviceRGB); }
    void setResultColorSpace(ColorSpace) OVERRIDE { FilterEffect::setResultColorSpace(ColorSpaceDeviceRGB); }
#endif

    virtual void platformApplySoftware();
#if ENABLE(OPENCL)
    virtual bool platformApplyOpenCL();
#endif
    virtual void dump();

    virtual void determineAbsolutePaintRect() { setAbsolutePaintRect(enclosingIntRect(maxEffectRect())); }

    virtual TextStream& externalRepresentation(TextStream&, int indention) const;

private:
    FEFlood(Filter*, const Color&, float);

    Color m_floodColor;
    float m_floodOpacity;
};

} // namespace WebCore

#endif // ENABLE(FILTERS)

#endif // FEFlood_h
