/*
 * Copyright (c) 2012, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "LayoutRect.h"

#include <algorithm>

using std::max;
using std::min;

namespace WebCore {

LayoutRect::LayoutRect(const FloatRect& r)
    : m_location(LayoutPoint(r.location()))
    , m_size(LayoutSize(r.size()))
{
}

bool LayoutRect::intersects(const LayoutRect& other) const
{
    // Checking emptiness handles negative widths as well as zero.
    return !isEmpty() && !other.isEmpty()
        && x() < other.maxX() && other.x() < maxX()
        && y() < other.maxY() && other.y() < maxY();
}

bool LayoutRect::contains(const LayoutRect& other) const
{
    return x() <= other.x() && maxX() >= other.maxX()
        && y() <= other.y() && maxY() >= other.maxY();
}

void LayoutRect::intersect(const LayoutRect& other)
{
    LayoutPoint newLocation(max(x(), other.x()), max(y(), other.y()));
    LayoutPoint newMaxPoint(min(maxX(), other.maxX()), min(maxY(), other.maxY()));

    // Return a clean empty rectangle for non-intersecting cases.
    if (newLocation.x() >= newMaxPoint.x() || newLocation.y() >= newMaxPoint.y()) {
        newLocation = LayoutPoint(0, 0);
        newMaxPoint = LayoutPoint(0, 0);
    }

    m_location = newLocation;
    m_size = newMaxPoint - newLocation;
}

void LayoutRect::unite(const LayoutRect& other)
{
    // Handle empty special cases first.
    if (other.isEmpty())
        return;
    if (isEmpty()) {
        *this = other;
        return;
    }

    LayoutPoint newLocation(min(x(), other.x()), min(y(), other.y()));
    LayoutPoint newMaxPoint(max(maxX(), other.maxX()), max(maxY(), other.maxY()));

    m_location = newLocation;
    m_size = newMaxPoint - newLocation;
}

void LayoutRect::uniteIfNonZero(const LayoutRect& other)
{
    // Handle empty special cases first.
    if (!other.width() && !other.height())
        return;
    if (!width() && !height()) {
        *this = other;
        return;
    }

    LayoutPoint newLocation(min(x(), other.x()), min(y(), other.y()));
    LayoutPoint newMaxPoint(max(maxX(), other.maxX()), max(maxY(), other.maxY()));

    m_location = newLocation;
    m_size = newMaxPoint - newLocation;
}

void LayoutRect::scale(float s)
{
    m_location.scale(s, s);
    m_size.scale(s);
}

LayoutRect unionRect(const Vector<LayoutRect>& rects)
{
    LayoutRect result;

    size_t count = rects.size();
    for (size_t i = 0; i < count; ++i)
        result.unite(rects[i]);

    return result;
}

IntRect enclosingIntRect(const LayoutRect& rect)
{
    IntPoint location = flooredIntPoint(rect.minXMinYCorner());
    IntPoint maxPoint = ceiledIntPoint(rect.maxXMaxYCorner());

    return IntRect(location, maxPoint - location);
}

LayoutRect enclosingLayoutRect(const FloatRect& rect)
{
#if ENABLE(SUBPIXEL_LAYOUT)
    LayoutPoint location = flooredLayoutPoint(rect.minXMinYCorner());
    LayoutPoint maxPoint = ceiledLayoutPoint(rect.maxXMaxYCorner());

    return LayoutRect(location, maxPoint - location);
#else
    return enclosingIntRect(rect);
#endif
}

} // namespace WebCore
