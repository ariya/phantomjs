/*
 * Copyright (C) 2004, 2005, 2006, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006 Rob Buis <buis@kde.org>
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
#include "SVGPolygonElement.h"

#include "Path.h"

namespace WebCore {

inline SVGPolygonElement::SVGPolygonElement(const QualifiedName& tagName, Document* document)
    : SVGPolyElement(tagName, document)
{
}

PassRefPtr<SVGPolygonElement> SVGPolygonElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new SVGPolygonElement(tagName, document));
}

void SVGPolygonElement::toPathData(Path& path) const
{
    ASSERT(path.isEmpty());

    SVGPointList& points = pointList();
    if (points.isEmpty())
        return;

    path.moveTo(points.first());

    unsigned size = points.size();
    for (unsigned i = 1; i < size; ++i)
        path.addLineTo(points.at(i));

    path.closeSubpath();
}

}

#endif // ENABLE(SVG)
