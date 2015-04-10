/*
 * Copyright (C) 2004, 2005, 2006 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006, 2008 Rob Buis <buis@kde.org>
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

#ifndef SVGPathSegArc_h
#define SVGPathSegArc_h

#if ENABLE(SVG)
#include "SVGPathSegWithContext.h"

namespace WebCore {

class SVGPathSegArc : public SVGPathSegWithContext {
public:
    SVGPathSegArc(SVGPathElement* element, SVGPathSegRole role, float x, float y, float r1, float r2, float angle, bool largeArcFlag, bool sweepFlag)
        : SVGPathSegWithContext(element, role)
        , m_x(x)
        , m_y(y)
        , m_r1(r1)
        , m_r2(r2)
        , m_angle(angle)
        , m_largeArcFlag(largeArcFlag)
        , m_sweepFlag(sweepFlag)
    {
    }

    float x() const { return m_x; }
    void setX(float x)
    {
        m_x = x;
        commitChange();
    }

    float y() const { return m_y; }
    void setY(float y)
    {
        m_y = y;
        commitChange();
    }

    float r1() const { return m_r1; }
    void setR1(float r1)
    {
        m_r1 = r1;
        commitChange();
    }

    float r2() const { return m_r2; }
    void setR2(float r2)
    {
        m_r2 = r2;
        commitChange();
    }

    float angle() const { return m_angle; }
    void setAngle(float angle)
    {
        m_angle = angle;
        commitChange();
    }

    bool largeArcFlag() const { return m_largeArcFlag; }
    void setLargeArcFlag(bool largeArcFlag)
    {
        m_largeArcFlag = largeArcFlag;
        commitChange();
    }

    bool sweepFlag() const { return m_sweepFlag; }
    void setSweepFlag(bool sweepFlag)
    {
        m_sweepFlag = sweepFlag;
        commitChange();
    }

private:
    float m_x;
    float m_y;
    float m_r1;
    float m_r2;
    float m_angle;

    bool m_largeArcFlag : 1;
    bool m_sweepFlag : 1;
};

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
