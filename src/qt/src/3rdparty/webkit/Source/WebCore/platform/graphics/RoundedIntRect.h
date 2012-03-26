/*
 * Copyright (C) 2003, 2006, 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef RoundedIntRect_h
#define RoundedIntRect_h

#include "IntRect.h"

namespace WebCore {


class RoundedIntRect {
public:
    class Radii {
    public:
        Radii() {}
        Radii(const IntSize& topLeft, const IntSize& topRight, const IntSize& bottomLeft, const IntSize& bottomRight)
            : m_topLeft(topLeft)
            , m_topRight(topRight)
            , m_bottomLeft(bottomLeft)
            , m_bottomRight(bottomRight)
        {
        }

        void setTopLeft(const IntSize& size) { m_topLeft = size; }
        void setTopRight(const IntSize& size) { m_topRight = size; }
        void setBottomLeft(const IntSize& size) { m_bottomLeft = size; }
        void setBottomRight(const IntSize& size) { m_bottomRight = size; }
        const IntSize& topLeft() const { return m_topLeft; }
        const IntSize& topRight() const { return m_topRight; }
        const IntSize& bottomLeft() const { return m_bottomLeft; }
        const IntSize& bottomRight() const { return m_bottomRight; }

        bool isZero() const;

        void includeLogicalEdges(const Radii& edges, bool isHorizontal, bool includeLogicalLeftEdge, bool includeLogicalRightEdge);
        void excludeLogicalEdges(bool isHorizontal, bool excludeLogicalLeftEdge, bool excludeLogicalRightEdge);

        void scale(float factor);
        void expand(int topWidth, int bottomWidth, int leftWidth, int rightWidth);
        void expand(int size) { expand(size, size, size, size); }
        void shrink(int topWidth, int bottomWidth, int leftWidth, int rightWidth) { expand(-topWidth, -bottomWidth, -leftWidth, -rightWidth); }
        void shrink(int size) { shrink(size, size, size, size); }

    private:
        IntSize m_topLeft;
        IntSize m_topRight;
        IntSize m_bottomLeft;
        IntSize m_bottomRight;
    };

    explicit RoundedIntRect(const IntRect&, const Radii& = Radii());
    RoundedIntRect(int x, int y, int width, int height);
    RoundedIntRect(const IntRect&, const IntSize& topLeft, const IntSize& topRight, const IntSize& bottomLeft, const IntSize& bottomRight);

    const IntRect& rect() const { return m_rect; }
    const Radii& radii() const { return m_radii; }
    bool isRounded() const { return !m_radii.isZero(); }
    bool isEmpty() const { return m_rect.isEmpty(); }

    void setRect(const IntRect& rect) { m_rect = rect; }
    void setRadii(const Radii& radii) { m_radii = radii; }

    void move(const IntSize& size) { m_rect.move(size); }
    void inflate(int size) { m_rect.inflate(size);  }
    void inflateWithRadii(int size) { m_rect.inflate(size); m_radii.expand(size); }
    void expandRadii(int size) { m_radii.expand(size); }
    void shrinkRadii(int size) { m_radii.shrink(size); }

    void includeLogicalEdges(const Radii& edges, bool isHorizontal, bool includeLogicalLeftEdge, bool includeLogicalRightEdge);
    void excludeLogicalEdges(bool isHorizontal, bool excludeLogicalLeftEdge, bool excludeLogicalRightEdge);

    bool isRenderable() const;

private:
    IntRect m_rect;
    Radii m_radii;
};

inline bool operator==(const RoundedIntRect::Radii& a, const RoundedIntRect::Radii& b)
{
    return a.topLeft() == b.topLeft() && a.topRight() == b.topRight() && a.bottomLeft() == b.bottomLeft() && a.bottomRight() == b.bottomRight();
}

inline bool operator==(const RoundedIntRect& a, const RoundedIntRect& b)
{
    return a.rect() == b.rect() && a.radii() == b.radii();
}


} // namespace WebCore

#endif // RoundedIntRect_h
