/*
 * Copyright (C) 2005 Oliver Hunt <ojh16@student.canterbury.ac.nz>
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

#include "config.h"

#if ENABLE(SVG) && ENABLE(FILTERS)
#include "SVGFEDistantLightElement.h"
#include "SVGNames.h"

#include "DistantLightSource.h"

namespace WebCore {

inline SVGFEDistantLightElement::SVGFEDistantLightElement(const QualifiedName& tagName, Document* document)
    : SVGFELightElement(tagName, document)
{
    ASSERT(hasTagName(SVGNames::feDistantLightTag));
}

PassRefPtr<SVGFEDistantLightElement> SVGFEDistantLightElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new SVGFEDistantLightElement(tagName, document));
}

PassRefPtr<LightSource> SVGFEDistantLightElement::lightSource() const
{
    return DistantLightSource::create(azimuth(), elevation());
}

}

#endif // ENABLE(SVG)
