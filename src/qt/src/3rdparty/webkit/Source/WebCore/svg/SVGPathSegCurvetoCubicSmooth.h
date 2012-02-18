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

#ifndef SVGPathSegCurvetoCubicSmooth_h
#define SVGPathSegCurvetoCubicSmooth_h

#if ENABLE(SVG)
#include "SVGPathSegWithContext.h"

namespace WebCore {

class SVGPathSegCurvetoCubicSmooth : public SVGPathSegWithContext {
public:
    SVGPathSegCurvetoCubicSmooth(SVGPathElement* element, SVGPathSegRole role, float x, float y, float x2, float y2)
        : SVGPathSegWithContext(element, role)
        , m_x(x)
        , m_y(y)
        , m_x2(x2)
        , m_y2(y2)
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

    float x2() const { return m_x2; }
    void setX2(float x2)
    {
        m_x2 = x2;
        commitChange();
    }

    float y2() const { return m_y2; }
    void setY2(float y2)
    {
        m_y2 = y2;
        commitChange();
    }

private:
    float m_x;
    float m_y;
    float m_x2;
    float m_y2;
};

class SVGPathSegCurvetoCubicSmoothAbs : public SVGPathSegCurvetoCubicSmooth { 
public:
    static PassRefPtr<SVGPathSegCurvetoCubicSmoothAbs> create(SVGPathElement* element, SVGPathSegRole role, float x, float y, float x2, float y2)
    {
        return adoptRef(new SVGPathSegCurvetoCubicSmoothAbs(element, role, x, y, x2, y2));
    }

private:
    SVGPathSegCurvetoCubicSmoothAbs(SVGPathElement* element, SVGPathSegRole role, float x, float y, float x2, float y2)
        : SVGPathSegCurvetoCubicSmooth(element, role, x, y, x2, y2)
    {
    }

    virtual unsigned short pathSegType() const { return PATHSEG_CURVETO_CUBIC_SMOOTH_ABS; }
    virtual String pathSegTypeAsLetter() const { return "S"; }
};

class SVGPathSegCurvetoCubicSmoothRel : public SVGPathSegCurvetoCubicSmooth { 
public:
    static PassRefPtr<SVGPathSegCurvetoCubicSmoothRel> create(SVGPathElement* element, SVGPathSegRole role, float x, float y, float x2, float y2)
    {
        return adoptRef(new SVGPathSegCurvetoCubicSmoothRel(element, role, x, y, x2, y2));
    }

private:
    SVGPathSegCurvetoCubicSmoothRel(SVGPathElement* element, SVGPathSegRole role, float x, float y, float x2, float y2)
        : SVGPathSegCurvetoCubicSmooth(element, role, x, y, x2, y2)
    {
    }

    virtual unsigned short pathSegType() const { return PATHSEG_CURVETO_CUBIC_SMOOTH_REL; }
    virtual String pathSegTypeAsLetter() const { return "s"; }
};

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
