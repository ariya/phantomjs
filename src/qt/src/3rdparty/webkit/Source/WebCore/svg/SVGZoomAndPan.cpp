/*
 * Copyright (C) 2004, 2005, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006, 2007 Rob Buis <buis@kde.org>
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
#include "SVGZoomAndPan.h"

#include "Attribute.h"
#include "SVGNames.h"
#include "SVGParserUtilities.h"

namespace WebCore {

void SVGZoomAndPan::setZoomAndPan(unsigned short zoomAndPan)
{
    m_zoomAndPan = zoomAndPan;
}

bool SVGZoomAndPan::parseMappedAttribute(Attribute* attr)
{
    if (attr->name() == SVGNames::zoomAndPanAttr) {
        const UChar* start = attr->value().characters();
        const UChar* end = start + attr->value().length();
        parseZoomAndPan(start, end);
        return true;
    }

    return false;
}

bool SVGZoomAndPan::isKnownAttribute(const QualifiedName& attrName)
{
    return attrName == SVGNames::zoomAndPanAttr;
}

static const UChar disable[] =  {'d', 'i', 's', 'a', 'b', 'l', 'e'};
static const UChar magnify[] =  {'m', 'a', 'g', 'n', 'i', 'f', 'y'};

bool SVGZoomAndPan::parseZoomAndPan(const UChar*& start, const UChar* end)
{
    if (skipString(start, end, disable, WTF_ARRAY_LENGTH(disable)))
        setZoomAndPan(SVG_ZOOMANDPAN_DISABLE);
    else if (skipString(start, end, magnify, WTF_ARRAY_LENGTH(magnify)))
        setZoomAndPan(SVG_ZOOMANDPAN_MAGNIFY);
    else
        return false;

    return true;
}

}

#endif // ENABLE(SVG)
