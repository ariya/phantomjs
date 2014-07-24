/*
 * Copyright (C) 2006 Nikolas Zimmermann <zimmermann@kde.org>
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

#ifndef RadialGradientAttributes_h
#define RadialGradientAttributes_h

#if ENABLE(SVG)
#include "GradientAttributes.h"

namespace WebCore {
struct RadialGradientAttributes : GradientAttributes {
    RadialGradientAttributes()
        : m_cx(LengthModeWidth, "50%")
        , m_cy(LengthModeWidth, "50%")
        , m_r(LengthModeWidth, "50%")
        , m_cxSet(false)
        , m_cySet(false)
        , m_rSet(false)
        , m_fxSet(false)
        , m_fySet(false) 
        , m_frSet(false)
    {
    }

    SVGLength cx() const { return m_cx; }
    SVGLength cy() const { return m_cy; }
    SVGLength r() const { return m_r; }
    SVGLength fx() const { return m_fx; }
    SVGLength fy() const { return m_fy; }
    SVGLength fr() const { return m_fr; }

    void setCx(const SVGLength& value) { m_cx = value; m_cxSet = true; }
    void setCy(const SVGLength& value) { m_cy = value; m_cySet = true; }
    void setR(const SVGLength& value) { m_r = value; m_rSet = true; }
    void setFx(const SVGLength& value) { m_fx = value; m_fxSet = true; }
    void setFy(const SVGLength& value) { m_fy = value; m_fySet = true; }
    void setFr(const SVGLength& value) { m_fr = value; m_frSet = true; }

    bool hasCx() const { return m_cxSet; }
    bool hasCy() const { return m_cySet; }
    bool hasR() const { return m_rSet; }
    bool hasFx() const { return m_fxSet; }
    bool hasFy() const { return m_fySet; }
    bool hasFr() const { return m_frSet; }

private:
    // Properties
    SVGLength m_cx;
    SVGLength m_cy;
    SVGLength m_r;
    SVGLength m_fx;
    SVGLength m_fy;
    SVGLength m_fr;

    // Property states
    bool m_cxSet : 1;
    bool m_cySet : 1;
    bool m_rSet : 1;
    bool m_fxSet : 1;
    bool m_fySet : 1;
    bool m_frSet : 1;
};

} // namespace WebCore

#endif // ENABLE(SVG)
#endif

// vim:ts=4:noet
