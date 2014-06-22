/*
 * Copyright (C) 2012 Adobe Systems Incorporated. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "RectangleShape.h"

#include <wtf/MathExtras.h>

namespace WebCore {

static inline float ellipseXIntercept(float y, float rx, float ry)
{
    ASSERT(ry > 0);
    return rx * sqrt(1 - (y * y) / (ry * ry));
}

static inline float ellipseYIntercept(float x, float rx, float ry)
{
    ASSERT(rx > 0);
    return ry * sqrt(1 - (x * x) / (rx * rx));
}

FloatRoundedRect FloatRoundedRect::paddingBounds(float padding) const
{
    ASSERT(padding >= 0);
    if (!padding || isEmpty())
        return *this;

    float boundsX = x() + std::min(width() / 2, padding);
    float boundsY = y() + std::min(height() / 2, padding);
    float boundsWidth = std::max(0.0f, width() - padding * 2);
    float boundsHeight = std::max(0.0f, height() - padding * 2);
    float boundsRadiusX = std::max(0.0f, rx() - padding);
    float boundsRadiusY = std::max(0.0f, ry() - padding);
    return FloatRoundedRect(FloatRect(boundsX, boundsY, boundsWidth, boundsHeight), FloatSize(boundsRadiusX, boundsRadiusY));
}

FloatRoundedRect FloatRoundedRect::marginBounds(float margin) const
{
    ASSERT(margin >= 0);
    if (!margin)
        return *this;

    float boundsX = x() - margin;
    float boundsY = y() - margin;
    float boundsWidth = width() + margin * 2;
    float boundsHeight = height() + margin * 2;
    float boundsRadiusX = rx() + margin;
    float boundsRadiusY = ry() + margin;
    return FloatRoundedRect(FloatRect(boundsX, boundsY, boundsWidth, boundsHeight), FloatSize(boundsRadiusX, boundsRadiusY));
}

FloatPoint FloatRoundedRect::cornerInterceptForWidth(float widthAtIntercept) const
{
    float xi = (width() - widthAtIntercept) / 2;
    float yi = ry() - ellipseYIntercept(rx() - xi, rx(), ry());
    return FloatPoint(xi, yi);
}

FloatRoundedRect RectangleShape::shapePaddingBounds() const
{
    if (!m_haveInitializedPaddingBounds) {
        m_haveInitializedPaddingBounds = true;
        m_paddingBounds = m_bounds.paddingBounds(shapePadding());
    }
    return m_paddingBounds;
}

FloatRoundedRect RectangleShape::shapeMarginBounds() const
{
    if (!m_haveInitializedMarginBounds) {
        m_haveInitializedMarginBounds = true;
        m_marginBounds = m_bounds.marginBounds(shapeMargin());
    }
    return m_marginBounds;
}

void RectangleShape::getExcludedIntervals(LayoutUnit logicalTop, LayoutUnit logicalHeight, SegmentList& result) const
{
    const FloatRoundedRect& bounds = shapeMarginBounds();
    if (bounds.isEmpty())
        return;

    float y1 = logicalTop;
    float y2 = logicalTop + logicalHeight;

    if (y2 < bounds.y() || y1 >= bounds.maxY())
        return;

    float x1 = bounds.x();
    float x2 = bounds.maxX();

    if (bounds.ry() > 0) {
        if (y2 < bounds.y() + bounds.ry()) {
            float yi = y2 - bounds.y() - bounds.ry();
            float xi = ellipseXIntercept(yi, bounds.rx(), bounds.ry());
            x1 = bounds.x() + bounds.rx() - xi;
            x2 = bounds.maxX() - bounds.rx() + xi;
        } else if (y1 > bounds.maxY() - bounds.ry()) {
            float yi =  y1 - (bounds.maxY() - bounds.ry());
            float xi = ellipseXIntercept(yi, bounds.rx(), bounds.ry());
            x1 = bounds.x() + bounds.rx() - xi;
            x2 = bounds.maxX() - bounds.rx() + xi;
        }
    }

    result.append(LineSegment(x1, x2));
}

void RectangleShape::getIncludedIntervals(LayoutUnit logicalTop, LayoutUnit logicalHeight, SegmentList& result) const
{
    const FloatRoundedRect& bounds = shapePaddingBounds();
    if (bounds.isEmpty())
        return;

    float y1 = logicalTop;
    float y2 = logicalTop + logicalHeight;

    if (y1 < bounds.y() || y2 > bounds.maxY())
        return;

    float x1 = bounds.x();
    float x2 = bounds.maxX();

    if (bounds.ry() > 0) {
        bool y1InterceptsCorner = y1 < bounds.y() + bounds.ry();
        bool y2InterceptsCorner = y2 > bounds.maxY() - bounds.ry();
        float xi = 0;

        if (y1InterceptsCorner && y2InterceptsCorner) {
            if  (y1 < bounds.height() + 2 * bounds.y() - y2) {
                float yi = y1 - bounds.y() - bounds.ry();
                xi = ellipseXIntercept(yi, bounds.rx(), bounds.ry());
            } else {
                float yi =  y2 - (bounds.maxY() - bounds.ry());
                xi = ellipseXIntercept(yi, bounds.rx(), bounds.ry());
            }
        } else if (y1InterceptsCorner) {
            float yi = y1 - bounds.y() - bounds.ry();
            xi = ellipseXIntercept(yi, bounds.rx(), bounds.ry());
        } else if (y2InterceptsCorner) {
            float yi =  y2 - (bounds.maxY() - bounds.ry());
            xi = ellipseXIntercept(yi, bounds.rx(), bounds.ry());
        }

        if (y1InterceptsCorner || y2InterceptsCorner) {
            x1 = bounds.x() + bounds.rx() - xi;
            x2 = bounds.maxX() - bounds.rx() + xi;
        }
    }

    result.append(LineSegment(x1, x2));
}

bool RectangleShape::firstIncludedIntervalLogicalTop(LayoutUnit minLogicalIntervalTop, const LayoutSize& minLogicalIntervalSize, LayoutUnit& result) const
{
    float minIntervalTop = minLogicalIntervalTop;
    float minIntervalHeight = minLogicalIntervalSize.height();
    float minIntervalWidth = minLogicalIntervalSize.width();

    const FloatRoundedRect& bounds = shapePaddingBounds();
    if (bounds.isEmpty() || minIntervalWidth > bounds.width())
        return false;

    float minY = std::max(bounds.y(), minIntervalTop);
    float maxY = minY + minIntervalHeight;

    if (maxY > bounds.maxY())
        return false;

    bool intervalOverlapsMinCorner = minY < bounds.y() + bounds.ry();
    bool intervalOverlapsMaxCorner = maxY > bounds.maxY() - bounds.ry();

    if (!intervalOverlapsMinCorner && !intervalOverlapsMaxCorner) {
        result = minY;
        return true;
    }

    float centerY = bounds.y() + bounds.height() / 2;
    bool minCornerDefinesX = fabs(centerY - minY) > fabs(centerY - maxY);
    bool intervalFitsWithinCorners = minIntervalWidth + 2 * bounds.rx() <= bounds.width();
    FloatPoint cornerIntercept = bounds.cornerInterceptForWidth(minIntervalWidth);

    if (intervalOverlapsMinCorner && (!intervalOverlapsMaxCorner || minCornerDefinesX)) {
        if (intervalFitsWithinCorners || bounds.y() + cornerIntercept.y() < minY) {
            result = minY;
            return true;
        }
        if (minIntervalHeight < bounds.height() - (2 * cornerIntercept.y())) {
            result = ceiledLayoutUnit(bounds.y() + cornerIntercept.y());
            return true;
        }
    }

    if (intervalOverlapsMaxCorner && (!intervalOverlapsMinCorner || !minCornerDefinesX)) {
        if (intervalFitsWithinCorners || minY <=  bounds.maxY() - cornerIntercept.y() - minIntervalHeight) {
            result = minY;
            return true;
        }
    }

    return false;
}

} // namespace WebCore
