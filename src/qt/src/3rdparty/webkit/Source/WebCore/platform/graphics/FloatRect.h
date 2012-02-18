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

#ifndef FloatRect_h
#define FloatRect_h

#include "FloatPoint.h"

#if USE(CG) || USE(SKIA_ON_MAC_CHROME)
typedef struct CGRect CGRect;
#endif

#if PLATFORM(MAC) || (PLATFORM(CHROMIUM) && OS(DARWIN))
#ifdef NSGEOMETRY_TYPES_SAME_AS_CGGEOMETRY_TYPES
typedef struct CGRect NSRect;
#else
typedef struct _NSRect NSRect;
#endif
#endif

#if PLATFORM(QT)
QT_BEGIN_NAMESPACE
class QRectF;
QT_END_NAMESPACE
#endif

#if PLATFORM(WX) && USE(WXGC)
class wxRect2DDouble;
#endif

#if PLATFORM(HAIKU)
class BRect;
#endif

#if USE(SKIA)
struct SkRect;
#endif

#if USE(CAIRO)
typedef struct _cairo_rectangle cairo_rectangle_t;
#endif

namespace WebCore {

#if PLATFORM(OPENVG)
class VGRect;
#endif

class IntRect;

class FloatRect {
public:
    FloatRect() { }
    FloatRect(const FloatPoint& location, const FloatSize& size)
        : m_location(location), m_size(size) { }
    FloatRect(float x, float y, float width, float height)
        : m_location(FloatPoint(x, y)), m_size(FloatSize(width, height)) { }
    FloatRect(const IntRect&);

    static FloatRect narrowPrecision(double x, double y, double width, double height);

    FloatPoint location() const { return m_location; }
    FloatSize size() const { return m_size; }

    void setLocation(const FloatPoint& location) { m_location = location; }
    void setSize(const FloatSize& size) { m_size = size; }

    float x() const { return m_location.x(); }
    float y() const { return m_location.y(); }
    float maxX() const { return x() + width(); }
    float maxY() const { return y() + height(); }
    float width() const { return m_size.width(); }
    float height() const { return m_size.height(); }

    void setX(float x) { m_location.setX(x); }
    void setY(float y) { m_location.setY(y); }
    void setWidth(float width) { m_size.setWidth(width); }
    void setHeight(float height) { m_size.setHeight(height); }

    bool isEmpty() const { return m_size.isEmpty(); }

    FloatPoint center() const { return FloatPoint(x() + width() / 2, y() + height() / 2); }

    void move(const FloatSize& delta) { m_location += delta; } 
    void move(float dx, float dy) { m_location.move(dx, dy); } 

    bool intersects(const FloatRect&) const;
    bool contains(const FloatRect&) const;

    void intersect(const FloatRect&);
    void unite(const FloatRect&);
    void uniteIfNonZero(const FloatRect&);

    // Note, this doesn't match what IntRect::contains(IntPoint&) does; the int version
    // is really checking for containment of 1x1 rect, but that doesn't make sense with floats.
    bool contains(float px, float py) const
        { return px >= x() && px <= maxX() && py >= y() && py <= maxY(); }
    bool contains(const FloatPoint& point) const { return contains(point.x(), point.y()); }

    void inflateX(float dx) {
        m_location.setX(m_location.x() - dx);
        m_size.setWidth(m_size.width() + dx + dx);
    }
    void inflateY(float dy) {
        m_location.setY(m_location.y() - dy);
        m_size.setHeight(m_size.height() + dy + dy);
    }
    void inflate(float d) { inflateX(d); inflateY(d); }
    void scale(float s) { scale(s, s); }
    void scale(float sx, float sy);

    // Re-initializes this rectangle to fit the sets of passed points.
    void fitToPoints(const FloatPoint& p0, const FloatPoint& p1);
    void fitToPoints(const FloatPoint& p0, const FloatPoint& p1, const FloatPoint& p2);
    void fitToPoints(const FloatPoint& p0, const FloatPoint& p1, const FloatPoint& p2, const FloatPoint& p3);

#if USE(CG) || USE(SKIA_ON_MAC_CHROME)
    FloatRect(const CGRect&);
    operator CGRect() const;
#endif

#if (PLATFORM(MAC) && !defined(NSGEOMETRY_TYPES_SAME_AS_CGGEOMETRY_TYPES)) \
        || (PLATFORM(CHROMIUM) && OS(DARWIN))
    FloatRect(const NSRect&);
    operator NSRect() const;
#endif

#if PLATFORM(QT)
    FloatRect(const QRectF&);
    operator QRectF() const;
    FloatRect normalized() const;
#endif

#if PLATFORM(WX) && USE(WXGC)
    FloatRect(const wxRect2DDouble&);
    operator wxRect2DDouble() const;
#endif

#if PLATFORM(HAIKU)
    FloatRect(const BRect&);
    operator BRect() const;
#endif

#if USE(SKIA)
    FloatRect(const SkRect&);
    operator SkRect() const;
#endif

#if PLATFORM(OPENVG)
    operator VGRect() const;
#endif

#if USE(CAIRO)
    FloatRect(const cairo_rectangle_t&);
    operator cairo_rectangle_t() const;
#endif

private:
    FloatPoint m_location;
    FloatSize m_size;

    void setLocationAndSizeFromEdges(float left, float top, float right, float bottom)
    {
        m_location.set(left, top);
        m_size.setWidth(right - left);
        m_size.setHeight(bottom - top);
    }
};

inline FloatRect intersection(const FloatRect& a, const FloatRect& b)
{
    FloatRect c = a;
    c.intersect(b);
    return c;
}

inline FloatRect unionRect(const FloatRect& a, const FloatRect& b)
{
    FloatRect c = a;
    c.unite(b);
    return c;
}


inline bool operator==(const FloatRect& a, const FloatRect& b)
{
    return a.location() == b.location() && a.size() == b.size();
}

inline bool operator!=(const FloatRect& a, const FloatRect& b)
{
    return a.location() != b.location() || a.size() != b.size();
}

IntRect enclosingIntRect(const FloatRect&);

// Map rect r from srcRect to an equivalent rect in destRect.
FloatRect mapRect(const FloatRect& r, const FloatRect& srcRect, const FloatRect& destRect);

}

#endif
