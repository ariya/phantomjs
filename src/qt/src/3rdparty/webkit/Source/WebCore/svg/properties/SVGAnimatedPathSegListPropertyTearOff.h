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

#ifndef SVGAnimatedPathSegListPropertyTearOff_h
#define SVGAnimatedPathSegListPropertyTearOff_h

#if ENABLE(SVG)
#include "SVGAnimatedListPropertyTearOff.h"
#include "SVGPathSegList.h"
#include "SVGPathSegListPropertyTearOff.h"

namespace WebCore {

class SVGPathSegListPropertyTearOff;

class SVGAnimatedPathSegListPropertyTearOff : public SVGAnimatedListPropertyTearOff<SVGPathSegList> {
public:
    SVGProperty* baseVal(SVGPathSegRole role)
    {
        if (!m_baseVal)
            m_baseVal = SVGPathSegListPropertyTearOff::create(this, BaseValRole, role);
        return m_baseVal.get();
    }

    SVGProperty* animVal(SVGPathSegRole role)
    {
        if (!m_animVal)
            m_animVal = SVGPathSegListPropertyTearOff::create(this, AnimValRole, role);
        return m_animVal.get();
    }

    int removeItemFromList(const RefPtr<SVGPathSeg>& segment, bool shouldSynchronizeWrappers)
    {
        // This should ever be called for our baseVal, as animVal can't modify the list.
        return static_pointer_cast<SVGPathSegListPropertyTearOff>(m_baseVal)->removeItemFromList(segment, shouldSynchronizeWrappers);
    }

private:
    friend class SVGAnimatedProperty;

    static PassRefPtr<SVGAnimatedPathSegListPropertyTearOff> create(SVGElement* contextElement, const QualifiedName& attributeName, SVGPathSegList& values)
    {
        ASSERT(contextElement);
        return adoptRef(new SVGAnimatedPathSegListPropertyTearOff(contextElement, attributeName, values));
    }

    SVGAnimatedPathSegListPropertyTearOff(SVGElement* contextElement, const QualifiedName& attributeName, SVGPathSegList& values)
        : SVGAnimatedListPropertyTearOff<SVGPathSegList>(contextElement, attributeName, values)
    {
    }
};

}

#endif // ENABLE(SVG)
#endif // SVGAnimatedPathSegListPropertyTearOff_h
