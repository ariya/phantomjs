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

#ifndef SVGPathSegLineto_h
#define SVGPathSegLineto_h

#if ENABLE(SVG)
#include "SVGPathSegWithContext.h"

namespace WebCore {

class SVGPathSegLinetoAbs : public SVGPathSegSingleCoordinate { 
public:
    static PassRefPtr<SVGPathSegLinetoAbs> create(SVGPathElement* element, SVGPathSegRole role, float x, float y)
    {
        return adoptRef(new SVGPathSegLinetoAbs(element, role, x, y));
    }

private:
    SVGPathSegLinetoAbs(SVGPathElement* element, SVGPathSegRole role, float x, float y)
        : SVGPathSegSingleCoordinate(element, role, x, y)
    {
    }

    virtual unsigned short pathSegType() const { return PATHSEG_LINETO_ABS; }
    virtual String pathSegTypeAsLetter() const { return "L"; }
};

class SVGPathSegLinetoRel : public SVGPathSegSingleCoordinate { 
public:
    static PassRefPtr<SVGPathSegLinetoRel> create(SVGPathElement* element, SVGPathSegRole role, float x, float y)
    {
        return adoptRef(new SVGPathSegLinetoRel(element, role, x, y));
    }

private:
    SVGPathSegLinetoRel(SVGPathElement* element, SVGPathSegRole role, float x, float y)
        : SVGPathSegSingleCoordinate(element, role, x, y)
    {
    }

    virtual unsigned short pathSegType() const { return PATHSEG_LINETO_REL; }
    virtual String pathSegTypeAsLetter() const { return "l"; }
};

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
