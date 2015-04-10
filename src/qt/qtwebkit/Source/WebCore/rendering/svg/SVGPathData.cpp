/*
 * Copyright (C) Research In Motion Limited 2011. All rights reserved.
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
#include "SVGPathData.h"

#if ENABLE(SVG)
#include "Path.h"
#include "SVGCircleElement.h"
#include "SVGEllipseElement.h"
#include "SVGLineElement.h"
#include "SVGNames.h"
#include "SVGPathElement.h"
#include "SVGPathUtilities.h"
#include "SVGPolygonElement.h"
#include "SVGPolylineElement.h"
#include "SVGRectElement.h"
#include <wtf/HashMap.h>

namespace WebCore {

static void updatePathFromCircleElement(SVGElement* element, Path& path)
{
    ASSERT(element->hasTagName(SVGNames::circleTag));
    SVGCircleElement* circle = static_cast<SVGCircleElement*>(element);

    SVGLengthContext lengthContext(element);
    float r = circle->r().value(lengthContext);
    if (r > 0)
        path.addEllipse(FloatRect(circle->cx().value(lengthContext) - r, circle->cy().value(lengthContext) - r, r * 2, r * 2));
}

static void updatePathFromEllipseElement(SVGElement* element, Path& path)
{
    ASSERT(element->hasTagName(SVGNames::ellipseTag));
    SVGEllipseElement* ellipse = static_cast<SVGEllipseElement*>(element);

    SVGLengthContext lengthContext(element);
    float rx = ellipse->rx().value(lengthContext);
    if (rx <= 0)
        return;
    float ry = ellipse->ry().value(lengthContext);
    if (ry <= 0)
        return;
    path.addEllipse(FloatRect(ellipse->cx().value(lengthContext) - rx, ellipse->cy().value(lengthContext) - ry, rx * 2, ry * 2));
}

static void updatePathFromLineElement(SVGElement* element, Path& path)
{
    ASSERT(element->hasTagName(SVGNames::lineTag));
    SVGLineElement* line = static_cast<SVGLineElement*>(element);

    SVGLengthContext lengthContext(element);
    path.moveTo(FloatPoint(line->x1().value(lengthContext), line->y1().value(lengthContext)));
    path.addLineTo(FloatPoint(line->x2().value(lengthContext), line->y2().value(lengthContext)));
}

static void updatePathFromPathElement(SVGElement* element, Path& path)
{
    buildPathFromByteStream(toSVGPathElement(element)->pathByteStream(), path);
}

static void updatePathFromPolygonElement(SVGElement* element, Path& path)
{
    ASSERT(element->hasTagName(SVGNames::polygonTag));
    SVGPolygonElement* polygon = static_cast<SVGPolygonElement*>(element);

    SVGPointList& points = polygon->pointList();
    if (points.isEmpty())
        return;

    path.moveTo(points.first());

    unsigned size = points.size();
    for (unsigned i = 1; i < size; ++i)
        path.addLineTo(points.at(i));

    path.closeSubpath();
}

static void updatePathFromPolylineElement(SVGElement* element, Path& path)
{
    ASSERT(element->hasTagName(SVGNames::polylineTag));
    SVGPolylineElement* polyline = static_cast<SVGPolylineElement*>(element);

    SVGPointList& points = polyline->pointList();
    if (points.isEmpty())
        return;

    path.moveTo(points.first());

    unsigned size = points.size();
    for (unsigned i = 1; i < size; ++i)
        path.addLineTo(points.at(i));
}

static void updatePathFromRectElement(SVGElement* element, Path& path)
{
    ASSERT(element->hasTagName(SVGNames::rectTag));
    SVGRectElement* rect = static_cast<SVGRectElement*>(element);

    SVGLengthContext lengthContext(element);
    float width = rect->width().value(lengthContext);
    if (width <= 0)
        return;
    float height = rect->height().value(lengthContext);
    if (height <= 0)
        return;
    float x = rect->x().value(lengthContext);
    float y = rect->y().value(lengthContext);
    bool hasRx = rect->hasAttribute(SVGNames::rxAttr);
    bool hasRy = rect->hasAttribute(SVGNames::ryAttr);
    if (hasRx || hasRy) {
        float rx = rect->rx().value(lengthContext);
        float ry = rect->ry().value(lengthContext);
        if (!hasRx)
            rx = ry;
        else if (!hasRy)
            ry = rx;
        // FIXME: We currently enforce using beziers here, as at least on CoreGraphics/Lion, as
        // the native method uses a different line dash origin, causing svg/custom/dashOrigin.svg to fail.
        // See bug https://bugs.webkit.org/show_bug.cgi?id=79932 which tracks this issue.
        path.addRoundedRect(FloatRect(x, y, width, height), FloatSize(rx, ry), Path::PreferBezierRoundedRect);
        return;
    }

    path.addRect(FloatRect(x, y, width, height));
}

void updatePathFromGraphicsElement(SVGElement* element, Path& path)
{
    ASSERT(element);
    ASSERT(path.isEmpty());

    typedef void (*PathUpdateFunction)(SVGElement*, Path&);
    static HashMap<AtomicStringImpl*, PathUpdateFunction>* map = 0;
    if (!map) {
        map = new HashMap<AtomicStringImpl*, PathUpdateFunction>;
        map->set(SVGNames::circleTag.localName().impl(), updatePathFromCircleElement);
        map->set(SVGNames::ellipseTag.localName().impl(), updatePathFromEllipseElement);
        map->set(SVGNames::lineTag.localName().impl(), updatePathFromLineElement);
        map->set(SVGNames::pathTag.localName().impl(), updatePathFromPathElement);
        map->set(SVGNames::polygonTag.localName().impl(), updatePathFromPolygonElement);
        map->set(SVGNames::polylineTag.localName().impl(), updatePathFromPolylineElement);
        map->set(SVGNames::rectTag.localName().impl(), updatePathFromRectElement);
    }

    if (PathUpdateFunction pathUpdateFunction = map->get(element->localName().impl()))
        (*pathUpdateFunction)(element, path);
}

} // namespace WebCore

#endif // ENABLE(SVG)
