/*
 * Copyright (C) 2003, 2006, 2007 Apple Inc.  All rights reserved.
 * Copyright (C) 2005 Nokia.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "FloatRect.h"

#include "FloatConversion.h"
#include "IntRect.h"
#include <algorithm>
#include <math.h>
#include <wtf/MathExtras.h>

using std::max;
using std::min;

namespace WebCore {

FloatRect::FloatRect(const IntRect& r) : m_location(r.location()), m_size(r.size())
{
}

FloatRect FloatRect::narrowPrecision(double x, double y, double width, double height)
{
    return FloatRect(narrowPrecisionToFloat(x), narrowPrecisionToFloat(y), narrowPrecisionToFloat(width), narrowPrecisionToFloat(height));
}

bool FloatRect::intersects(const FloatRect& other) const
{
    // Checking emptiness handles negative widths as well as zero.
    return !isEmpty() && !other.isEmpty()
        && x() < other.maxX() && other.x() < maxX()
        && y() < other.maxY() && other.y() < maxY();
}

bool FloatRect::contains(const FloatRect& other) const
{
    return x() <= other.x() && maxX() >= other.maxX()
        && y() <= other.y() && maxY() >= other.maxY();
}

void FloatRect::intersect(const FloatRect& other)
{
    float l = max(x(), other.x());
    float t = max(y(), other.y());
    float r = min(maxX(), other.maxX());
    float b = min(maxY(), other.maxY());

    // Return a clean empty rectangle for non-intersecting cases.
    if (l >= r || t >= b) {
        l = 0;
        t = 0;
        r = 0;
        b = 0;
    }

    setLocationAndSizeFromEdges(l, t, r, b);
}

void FloatRect::unite(const FloatRect& other)
{
    // Handle empty special cases first.
    if (other.isEmpty())
        return;
    if (isEmpty()) {
        *this = other;
        return;
    }

    float l = min(x(), other.x());
    float t = min(y(), other.y());
    float r = max(maxX(), other.maxX());
    float b = max(maxY(), other.maxY());

    setLocationAndSizeFromEdges(l, t, r, b);
}

void FloatRect::uniteIfNonZero(const FloatRect& other)
{
    // Handle empty special cases first.
    if (!other.width() && !other.height())
        return;
    if (!width() && !height()) {
        *this = other;
        return;
    }

    float left = min(x(), other.x());
    float top = min(y(), other.y());
    float right = max(maxX(), other.maxX());
    float bottom = max(maxY(), other.maxY());

    setLocationAndSizeFromEdges(left, top, right, bottom);
}

void FloatRect::scale(float sx, float sy)
{
    m_location.setX(x() * sx);
    m_location.setY(y() * sy);
    m_size.setWidth(width() * sx);
    m_size.setHeight(height() * sy);
}

void FloatRect::fitToPoints(const FloatPoint& p0, const FloatPoint& p1)
{
    float left = min(p0.x(), p1.x());
    float top = min(p0.y(), p1.y());
    float right = max(p0.x(), p1.x());
    float bottom = max(p0.y(), p1.y());

    setLocationAndSizeFromEdges(left, top, right, bottom);
}

namespace {
// Helpers for 3- and 4-way max and min.

template <typename T>
T min3(const T& v1, const T& v2, const T& v3)
{
    return min(min(v1, v2), v3);
}

template <typename T>
T max3(const T& v1, const T& v2, const T& v3)
{
    return max(max(v1, v2), v3);
}

template <typename T>
T min4(const T& v1, const T& v2, const T& v3, const T& v4)
{
    return min(min(v1, v2), min(v3, v4));
}

template <typename T>
T max4(const T& v1, const T& v2, const T& v3, const T& v4)
{
    return max(max(v1, v2), max(v3, v4));
}

} // anonymous namespace

void FloatRect::fitToPoints(const FloatPoint& p0, const FloatPoint& p1, const FloatPoint& p2)
{
    float left = min3(p0.x(), p1.x(), p2.x());
    float top = min3(p0.y(), p1.y(), p2.y());
    float right = max3(p0.x(), p1.x(), p2.x());
    float bottom = max3(p0.y(), p1.y(), p2.y());

    setLocationAndSizeFromEdges(left, top, right, bottom);
}

void FloatRect::fitToPoints(const FloatPoint& p0, const FloatPoint& p1, const FloatPoint& p2, const FloatPoint& p3)
{
    float left = min4(p0.x(), p1.x(), p2.x(), p3.x());
    float top = min4(p0.y(), p1.y(), p2.y(), p3.y());
    float right = max4(p0.x(), p1.x(), p2.x(), p3.x());
    float bottom = max4(p0.y(), p1.y(), p2.y(), p3.y());

    setLocationAndSizeFromEdges(left, top, right, bottom);
}

IntRect enclosingIntRect(const FloatRect& rect)
{
    float left = floorf(rect.x());
    float top = floorf(rect.y());
    float width = ceilf(rect.maxX()) - left;
    float height = ceilf(rect.maxY()) - top;
    
    return IntRect(clampToInteger(left), clampToInteger(top), 
                   clampToInteger(width), clampToInteger(height));
}

FloatRect mapRect(const FloatRect& r, const FloatRect& srcRect, const FloatRect& destRect)
{
    if (srcRect.width() == 0 || srcRect.height() == 0)
        return FloatRect();

    float widthScale = destRect.width() / srcRect.width();
    float heightScale = destRect.height() / srcRect.height();
    return FloatRect(destRect.x() + (r.x() - srcRect.x()) * widthScale,
                     destRect.y() + (r.y() - srcRect.y()) * heightScale,
                     r.width() * widthScale, r.height() * heightScale);
}

}
