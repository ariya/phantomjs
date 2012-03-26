/*
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
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

#ifndef SVGPathSegWithContext_h
#define SVGPathSegWithContext_h

#if ENABLE(SVG)
#include "SVGPathElement.h"

namespace WebCore {

class SVGPathSegWithContext : public SVGPathSeg {
public:
    SVGPathSegWithContext(SVGPathElement* element, SVGPathSegRole role)
        : m_role(role)
        , m_element(element)
    {
    }

    SVGAnimatedProperty* animatedProperty() const
    {
        switch (m_role) {
        case PathSegUndefinedRole:
            return 0;
        case PathSegUnalteredRole:
            return m_element->animatablePathSegList();
        case PathSegNormalizedRole:
            // FIXME: https://bugs.webkit.org/show_bug.cgi?id=15412 - Implement normalized path segment lists!
            return 0;
        };

        return 0;
    }

    SVGPathElement* contextElement() const { return m_element.get(); }
    SVGPathSegRole role() const { return m_role; }

    void setContextAndRole(SVGPathElement* element, SVGPathSegRole role)
    {
        m_role = role;
        m_element = element;
    }

protected:
    void commitChange()
    {
        if (!m_element) {
            ASSERT(m_role == PathSegUndefinedRole);
            return;
        }

        ASSERT(m_role != PathSegUndefinedRole);
        m_element->pathSegListChanged(m_role);
    }

private:
    SVGPathSegRole m_role;
    RefPtr<SVGPathElement> m_element;
};

class SVGPathSegSingleCoordinate : public SVGPathSegWithContext { 
public:
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

protected:
    SVGPathSegSingleCoordinate(SVGPathElement* element, SVGPathSegRole role, float x, float y)
        : SVGPathSegWithContext(element, role)
        , m_x(x)
        , m_y(y)
    {
    }

private:
    float m_x;
    float m_y;
};

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
