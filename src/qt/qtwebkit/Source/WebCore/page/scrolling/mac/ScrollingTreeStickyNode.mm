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
#include "ScrollingTreeStickyNode.h"

#if ENABLE(THREADED_SCROLLING)

#include "ScrollingStateStickyNode.h"
#include "ScrollingTree.h"

namespace WebCore {

PassOwnPtr<ScrollingTreeStickyNode> ScrollingTreeStickyNode::create(ScrollingTree* scrollingTree, ScrollingNodeID nodeID)
{
    return adoptPtr(new ScrollingTreeStickyNode(scrollingTree, nodeID));
}

ScrollingTreeStickyNode::ScrollingTreeStickyNode(ScrollingTree* scrollingTree, ScrollingNodeID nodeID)
    : ScrollingTreeNode(scrollingTree, nodeID)
{
}

ScrollingTreeStickyNode::~ScrollingTreeStickyNode()
{
}

void ScrollingTreeStickyNode::updateBeforeChildren(ScrollingStateNode* stateNode)
{
    ScrollingStateStickyNode* stickyStateNode = toScrollingStateStickyNode(stateNode);

    if (stickyStateNode->hasChangedProperty(ScrollingStateNode::ScrollLayer))
        m_layer = stickyStateNode->platformScrollLayer();

    if (stateNode->hasChangedProperty(ScrollingStateStickyNode::ViewportConstraints))
        m_constraints = stickyStateNode->viewportConstraints();
}

static inline CGPoint operator*(CGPoint& a, const CGSize& b)
{
    return CGPointMake(a.x * b.width, a.y * b.height);
}

void ScrollingTreeStickyNode::parentScrollPositionDidChange(const IntRect& viewportRect, const FloatSize& cumulativeDelta)
{
    FloatPoint layerPosition = m_constraints.layerPositionForConstrainingRect(viewportRect);

    // FIXME: Subtracting the cumulativeDelta is not totally sufficient to get the new position right for nested
    // sticky objects. We probably need a way to modify the containingBlockRect in the ViewportContraints
    // to get this right in all cases.
    layerPosition -= cumulativeDelta;

    CGRect layerBounds = [m_layer.get() bounds];
    CGPoint anchorPoint = [m_layer.get() anchorPoint];
    CGPoint newPosition = layerPosition - m_constraints.alignmentOffset() + anchorPoint * layerBounds.size;
    [m_layer.get() setPosition:newPosition];

    if (!m_children)
        return;

    FloatSize newDelta = layerPosition - m_constraints.layerPositionAtLastLayout() + cumulativeDelta;

    size_t size = m_children->size();
    for (size_t i = 0; i < size; ++i)
        m_children->at(i)->parentScrollPositionDidChange(viewportRect, newDelta);
}

} // namespace WebCore

#endif // ENABLE(THREADED_SCROLLING)
