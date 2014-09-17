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

#ifndef SVGPathSegCurvetoQuadraticSmooth_h
#define SVGPathSegCurvetoQuadraticSmooth_h

#if ENABLE(SVG)
#include "SVGPathSegWithContext.h"

namespace WebCore {

class SVGPathSegCurvetoQuadraticSmoothAbs : public SVGPathSegSingleCoordinate {
public:
    static PassRefPtr<SVGPathSegCurvetoQuadraticSmoothAbs> create(SVGPathElement* element, SVGPathSegRole role, float x, float y)
    {
        return adoptRef(new SVGPathSegCurvetoQuadraticSmoothAbs(element, role, x, y));
    }

private:
    SVGPathSegCurvetoQuadraticSmoothAbs(SVGPathElement* element, SVGPathSegRole role, float x, float y)
        : SVGPathSegSingleCoordinate(element, role, x, y)
    {
    }

    virtual unsigned short pathSegType() const { return PATHSEG_CURVETO_QUADRATIC_SMOOTH_ABS; }
    virtual String pathSegTypeAsLetter() const { return "T"; }
};

class SVGPathSegCurvetoQuadraticSmoothRel : public SVGPathSegSingleCoordinate {
public:
    static PassRefPtr<SVGPathSegCurvetoQuadraticSmoothRel> create(SVGPathElement* element, SVGPathSegRole role, float x, float y)
    {
        return adoptRef(new SVGPathSegCurvetoQuadraticSmoothRel(element, role, x, y));
    }

private:
    SVGPathSegCurvetoQuadraticSmoothRel(SVGPathElement* element, SVGPathSegRole role, float x, float y)
        : SVGPathSegSingleCoordinate(element, role, x, y)
    {
    }

    virtual unsigned short pathSegType() const { return PATHSEG_CURVETO_QUADRATIC_SMOOTH_REL; }
    virtual String pathSegTypeAsLetter() const { return "t"; }
};

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
