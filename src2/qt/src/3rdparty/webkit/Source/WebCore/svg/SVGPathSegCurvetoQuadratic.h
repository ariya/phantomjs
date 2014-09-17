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

#ifndef SVGPathSegCurvetoQuadratic_h
#define SVGPathSegCurvetoQuadratic_h

#if ENABLE(SVG)
#include "SVGPathSegWithContext.h"

namespace WebCore {

class SVGPathSegCurvetoQuadratic : public SVGPathSegWithContext { 
public:
    SVGPathSegCurvetoQuadratic(SVGPathElement* element, SVGPathSegRole role, float x, float y, float x1, float y1)
        : SVGPathSegWithContext(element, role)
        , m_x(x)
        , m_y(y)
        , m_x1(x1)
        , m_y1(y1)
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

    float x1() const { return m_x1; }
    void setX1(float x1)
    {
        m_x1 = x1;
        commitChange();
    }

    float y1() const { return m_y1; }
    void setY1(float y1)
    {
        m_y1 = y1;
        commitChange();
    }

private:
    float m_x;
    float m_y;
    float m_x1;
    float m_y1;
};

class SVGPathSegCurvetoQuadraticAbs : public SVGPathSegCurvetoQuadratic {
public:
    static PassRefPtr<SVGPathSegCurvetoQuadraticAbs> create(SVGPathElement* element, SVGPathSegRole role, float x, float y, float x1, float y1)
    {
        return adoptRef(new SVGPathSegCurvetoQuadraticAbs(element, role, x, y, x1, y1));
    }

private:
    SVGPathSegCurvetoQuadraticAbs(SVGPathElement* element, SVGPathSegRole role, float x, float y, float x1, float y1)
        : SVGPathSegCurvetoQuadratic(element, role, x, y, x1, y1)
    {
    }

    virtual unsigned short pathSegType() const { return PATHSEG_CURVETO_QUADRATIC_ABS; }
    virtual String pathSegTypeAsLetter() const { return "Q"; }
};

class SVGPathSegCurvetoQuadraticRel : public SVGPathSegCurvetoQuadratic {
public:
    static PassRefPtr<SVGPathSegCurvetoQuadraticRel> create(SVGPathElement* element, SVGPathSegRole role, float x, float y, float x1, float y1)
    {
        return adoptRef(new SVGPathSegCurvetoQuadraticRel(element, role, x, y, x1, y1));
    }

private:
    SVGPathSegCurvetoQuadraticRel(SVGPathElement* element, SVGPathSegRole role, float x, float y, float x1, float y1)
        : SVGPathSegCurvetoQuadratic(element, role, x, y, x1, y1)
    {
    }

    virtual unsigned short pathSegType() const { return PATHSEG_CURVETO_QUADRATIC_REL; }
    virtual String pathSegTypeAsLetter() const { return "q"; }
};

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
