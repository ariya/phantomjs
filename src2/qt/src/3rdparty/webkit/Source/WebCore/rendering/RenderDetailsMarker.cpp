/*
 * Copyright (C) 2010, 2011 Nokia Corporation and/or its subsidiary(-ies)
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
 *
 */

#include "config.h"
#include "RenderDetailsMarker.h"

#if ENABLE(DETAILS)

#include "GraphicsContext.h"
#include "HTMLNames.h"
#include "PaintInfo.h"
#include "RenderDetails.h"

namespace WebCore {

using namespace HTMLNames;

RenderDetailsMarker::RenderDetailsMarker(Node* node)
    : RenderBlock(node)
{
}

bool RenderDetailsMarker::isOpen() const
{
    if (RenderDetails* owner = details())
        return owner->isOpen();
    return false;
}

static Path createPath(const FloatPoint* path)
{
    Path result;
    result.moveTo(FloatPoint(path[0].x(), path[0].y()));
    for (int i = 1; i < 4; ++i)
        result.addLineTo(FloatPoint(path[i].x(), path[i].y()));
    return result;
}

static Path createDownArrowPath()
{
    FloatPoint points[4] = { FloatPoint(0.0f, 0.07f), FloatPoint(0.5f, 0.93f), FloatPoint(1.0f, 0.07f), FloatPoint(0.0f, 0.07f) };
    return createPath(points);
}

static Path createUpArrowPath()
{
    FloatPoint points[4] = { FloatPoint(0.0f, 0.93f), FloatPoint(0.5f, 0.07f), FloatPoint(1.0f, 0.93f), FloatPoint(0.0f, 0.93f) };
    return createPath(points);
}

static Path createLeftArrowPath()
{
    FloatPoint points[4] = { FloatPoint(1.0f, 0.0f), FloatPoint(0.14f, 0.5f), FloatPoint(1.0f, 1.0f), FloatPoint(1.0f, 0.0f) };
    return createPath(points);
}

static Path createRightArrowPath()
{
    FloatPoint points[4] = { FloatPoint(0.0f, 0.0f), FloatPoint(0.86f, 0.5f), FloatPoint(0.0f, 1.0f), FloatPoint(0.0f, 0.0f) };
    return createPath(points);
}

RenderDetailsMarker::Orientation RenderDetailsMarker::orientation() const
{
    switch (style()->writingMode()) {
    case TopToBottomWritingMode:
        if (style()->isLeftToRightDirection())
            return isOpen() ? Down : Right;
        return isOpen() ? Down : Left;
    case RightToLeftWritingMode:
        if (style()->isLeftToRightDirection())
            return isOpen() ? Left : Down;
        return isOpen() ? Left : Up;
    case LeftToRightWritingMode:
        if (style()->isLeftToRightDirection())
            return isOpen() ? Right : Down;
        return isOpen() ? Right : Up;
    case BottomToTopWritingMode:
        if (style()->isLeftToRightDirection())
            return isOpen() ? Up : Right;
        return isOpen() ? Up : Left;
    }
    return Right;
}

Path RenderDetailsMarker::getCanonicalPath() const
{
    switch (orientation()) {
    case Left: return createLeftArrowPath();
    case Right: return createRightArrowPath();
    case Up: return createUpArrowPath();
    case Down: return createDownArrowPath();
    }

    return Path();
}

Path RenderDetailsMarker::getPath(const IntPoint& origin) const
{
    Path result = getCanonicalPath();
    result.transform(AffineTransform().scale(logicalHeight()));
    result.translate(FloatSize(origin.x(), origin.y()));
    return result;
}

void RenderDetailsMarker::paint(PaintInfo& paintInfo, int tx, int ty)
{
    if (paintInfo.phase != PaintPhaseForeground || style()->visibility() != VISIBLE) {
        RenderBlock::paint(paintInfo, tx, ty);
        return;
    }

    IntPoint boxOrigin(tx + x(), ty + y());
    IntRect overflowRect(visualOverflowRect());
    overflowRect.move(boxOrigin.x(), boxOrigin.y());
    overflowRect.inflate(maximalOutlineSize(paintInfo.phase));

    if (!paintInfo.rect.intersects(overflowRect))
        return;

    const Color color(style()->visitedDependentColor(CSSPropertyColor));
    paintInfo.context->setStrokeColor(color, style()->colorSpace());
    paintInfo.context->setStrokeStyle(SolidStroke);
    paintInfo.context->setStrokeThickness(1.0f);
    paintInfo.context->setFillColor(color, style()->colorSpace());

    paintInfo.context->fillPath(getPath(boxOrigin));
}

RenderDetails* RenderDetailsMarker::details() const
{
    for (RenderObject* renderer = parent(); renderer; renderer = renderer->parent()) {
        if (renderer->isDetails())
            return toRenderDetails(renderer);
    }

    return 0;
}

}

#endif
