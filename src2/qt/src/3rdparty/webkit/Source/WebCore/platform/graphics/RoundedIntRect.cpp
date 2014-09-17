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

#include "config.h"
#include "RoundedIntRect.h"

#include "IntRect.h"
#include <algorithm>

using namespace std;

namespace WebCore {

bool RoundedIntRect::Radii::isZero() const
{
    return m_topLeft.isZero() && m_topRight.isZero() && m_bottomLeft.isZero() && m_bottomRight.isZero();
}

void RoundedIntRect::Radii::scale(float factor)
{
    if (factor == 1)
        return;

    // If either radius on a corner becomes zero, reset both radii on that corner.
    m_topLeft.scale(factor);
    if (!m_topLeft.width() || !m_topLeft.height())
        m_topLeft = IntSize();
    m_topRight.scale(factor);
    if (!m_topRight.width() || !m_topRight.height())
        m_topRight = IntSize();
    m_bottomLeft.scale(factor);
    if (!m_bottomLeft.width() || !m_bottomLeft.height())
        m_bottomLeft = IntSize();
    m_bottomRight.scale(factor);
    if (!m_bottomRight.width() || !m_bottomRight.height())
        m_bottomRight = IntSize();

}

void RoundedIntRect::Radii::expand(int topWidth, int bottomWidth, int leftWidth, int rightWidth)
{
    m_topLeft.setWidth(max(0, m_topLeft.width() + leftWidth));
    m_topLeft.setHeight(max(0, m_topLeft.height() + topWidth));

    m_topRight.setWidth(max(0, m_topRight.width() + rightWidth));
    m_topRight.setHeight(max(0, m_topRight.height() + topWidth));

    m_bottomLeft.setWidth(max(0, m_bottomLeft.width() + leftWidth));
    m_bottomLeft.setHeight(max(0, m_bottomLeft.height() + bottomWidth));

    m_bottomRight.setWidth(max(0, m_bottomRight.width() + rightWidth));
    m_bottomRight.setHeight(max(0, m_bottomRight.height() + bottomWidth));
}

void RoundedIntRect::Radii::includeLogicalEdges(const RoundedIntRect::Radii& edges, bool isHorizontal, bool includeLogicalLeftEdge, bool includeLogicalRightEdge)
{
    if (includeLogicalLeftEdge) {
        if (isHorizontal)
            m_bottomLeft = edges.bottomLeft();
        else
            m_topRight = edges.topRight();
        m_topLeft = edges.topLeft();
    }

    if (includeLogicalRightEdge) {
        if (isHorizontal)
            m_topRight = edges.topRight();
        else
            m_bottomLeft = edges.bottomLeft();
        m_bottomRight = edges.bottomRight();
    }
}

void RoundedIntRect::Radii::excludeLogicalEdges(bool isHorizontal, bool excludeLogicalLeftEdge, bool excludeLogicalRightEdge)
{
    if (excludeLogicalLeftEdge) {
        if (isHorizontal)
            m_bottomLeft = IntSize();
        else
            m_topRight = IntSize();
        m_topLeft = IntSize();
    }
        
    if (excludeLogicalRightEdge) {
        if (isHorizontal)
            m_topRight = IntSize();
        else
            m_bottomLeft = IntSize();
        m_bottomRight = IntSize();
    }
}

RoundedIntRect::RoundedIntRect(int x, int y, int width, int height)
    : m_rect(x, y, width, height)
{
}

RoundedIntRect::RoundedIntRect(const IntRect& rect, const Radii& radii)
    : m_rect(rect)
    , m_radii(radii)
{
}

RoundedIntRect::RoundedIntRect(const IntRect& rect, const IntSize& topLeft, const IntSize& topRight, const IntSize& bottomLeft, const IntSize& bottomRight)
    : m_rect(rect)
    , m_radii(topLeft, topRight, bottomLeft, bottomRight)
{
}

void RoundedIntRect::includeLogicalEdges(const Radii& edges, bool isHorizontal, bool includeLogicalLeftEdge, bool includeLogicalRightEdge)
{
    m_radii.includeLogicalEdges(edges, isHorizontal, includeLogicalLeftEdge, includeLogicalRightEdge);
}

void RoundedIntRect::excludeLogicalEdges(bool isHorizontal, bool excludeLogicalLeftEdge, bool excludeLogicalRightEdge)
{
    m_radii.excludeLogicalEdges(isHorizontal, excludeLogicalLeftEdge, excludeLogicalRightEdge);
}

bool RoundedIntRect::isRenderable() const
{
    return m_radii.topLeft().width() + m_radii.topRight().width() <= m_rect.width()
        && m_radii.bottomLeft().width() + m_radii.bottomRight().width() <= m_rect.width()
        && m_radii.topLeft().height() + m_radii.topRight().height() <= m_rect.height()
        && m_radii.bottomLeft().height() + m_radii.bottomRight().height() <= m_rect.height();
}

} // namespace WebCore
