/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "ScrollingConstraints.h"

namespace WebCore {

FloatPoint FixedPositionViewportConstraints::layerPositionForViewportRect(const FloatRect& viewportRect) const
{
    FloatSize offset;

    if (hasAnchorEdge(AnchorEdgeLeft))
        offset.setWidth(viewportRect.x() - m_viewportRectAtLastLayout.x());
    else if (hasAnchorEdge(AnchorEdgeRight))
        offset.setWidth(viewportRect.maxX() - m_viewportRectAtLastLayout.maxX());

    if (hasAnchorEdge(AnchorEdgeTop))
        offset.setHeight(viewportRect.y() - m_viewportRectAtLastLayout.y());
    else if (hasAnchorEdge(AnchorEdgeBottom))
        offset.setHeight(viewportRect.maxY() - m_viewportRectAtLastLayout.maxY());

    return m_layerPositionAtLastLayout + offset;
}

FloatSize StickyPositionViewportConstraints::computeStickyOffset(const FloatRect& constrainingRect) const
{
    FloatRect boxRect = m_stickyBoxRect;
    
    if (hasAnchorEdge(AnchorEdgeRight)) {
        float rightLimit = constrainingRect.maxX() - m_rightOffset;
        float rightDelta = std::min<float>(0, rightLimit - m_stickyBoxRect.maxX());
        float availableSpace = std::min<float>(0, m_containingBlockRect.x() - m_stickyBoxRect.x());
        if (rightDelta < availableSpace)
            rightDelta = availableSpace;

        boxRect.move(rightDelta, 0);
    }

    if (hasAnchorEdge(AnchorEdgeLeft)) {
        float leftLimit = constrainingRect.x() + m_leftOffset;
        float leftDelta = std::max<float>(0, leftLimit - m_stickyBoxRect.x());
        float availableSpace = std::max<float>(0, m_containingBlockRect.maxX() - m_stickyBoxRect.maxX());
        if (leftDelta > availableSpace)
            leftDelta = availableSpace;

        boxRect.move(leftDelta, 0);
    }
    
    if (hasAnchorEdge(AnchorEdgeBottom)) {
        float bottomLimit = constrainingRect.maxY() - m_bottomOffset;
        float bottomDelta = std::min<float>(0, bottomLimit - m_stickyBoxRect.maxY());
        float availableSpace = std::min<float>(0, m_containingBlockRect.y() - m_stickyBoxRect.y());
        if (bottomDelta < availableSpace)
            bottomDelta = availableSpace;

        boxRect.move(0, bottomDelta);
    }

    if (hasAnchorEdge(AnchorEdgeTop)) {
        float topLimit = constrainingRect.y() + m_topOffset;
        float topDelta = std::max<float>(0, topLimit - m_stickyBoxRect.y());
        float availableSpace = std::max<float>(0, m_containingBlockRect.maxY() - m_stickyBoxRect.maxY());
        if (topDelta > availableSpace)
            topDelta = availableSpace;

        boxRect.move(0, topDelta);
    }

    return boxRect.location() - m_stickyBoxRect.location();
}

FloatPoint StickyPositionViewportConstraints::layerPositionForConstrainingRect(const FloatRect& constrainingRect) const
{
    FloatSize offset = computeStickyOffset(constrainingRect);
    return m_layerPositionAtLastLayout + offset - m_stickyOffsetAtLastLayout;
}

} // namespace WebCore
