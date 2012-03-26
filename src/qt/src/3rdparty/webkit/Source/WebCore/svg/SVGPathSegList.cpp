/*
 * Copyright (C) 2004, 2005, 2006, 2007, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005 Rob Buis <buis@kde.org>
 * Copyright (C) 2007 Eric Seidel <eric@webkit.org>
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

#include "config.h"

#if ENABLE(SVG)
#include "SVGPathSegList.h"

#include "SVGNames.h"
#include "SVGPathElement.h"
#include "SVGPathParserFactory.h"

namespace WebCore {

String SVGPathSegList::valueAsString() const
{
    String pathString;
    SVGPathParserFactory::self()->buildStringFromSVGPathSegList(*this, pathString, UnalteredParsing);
    return pathString;
}

void SVGPathSegList::commitChange(SVGElement* contextElement)
{
    ASSERT(contextElement);
    ASSERT(contextElement->hasTagName(SVGNames::pathTag));
    static_cast<SVGPathElement*>(contextElement)->pathSegListChanged(m_role);
}

}

#endif // ENABLE(SVG)
