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
#include "ScrollingTreeFixedNode.h"

#if ENABLE(THREADED_SCROLLING)

#include "ScrollingStateFixedNode.h"
#include "ScrollingTree.h"

namespace WebCore {

PassOwnPtr<ScrollingTreeFixedNode> ScrollingTreeFixedNode::create(ScrollingTree* scrollingTree, ScrollingNodeID nodeID)
{
    return adoptPtr(new ScrollingTreeFixedNode(scrollingTree, nodeID));
}

ScrollingTreeFixedNode::ScrollingTreeFixedNode(ScrollingTree* scrollingTree, ScrollingNodeID nodeID)
    : ScrollingTreeNode(scrollingTree, nodeID)
{
}

ScrollingTreeFixedNode::~ScrollingTreeFixedNode()
{
}

void ScrollingTreeFixedNode::updateBeforeChildren(ScrollingStateNode* stateNode)
{
    ScrollingStateFixedNode* fixedStateNode = toScrollingStateFixedNode(stateNode);

    if (fixedStateNode->hasChangedProperty(ScrollingStateNode::ScrollLayer))
        m_layer = fixedStateNode->platformScrollLayer();

    if (stateNode->hasChangedProperty(ScrollingStateFixedNode::ViewportConstraints))
        m_constraints = fixedStateNode->viewportConstraints();
}

static inline CGPoint operator*(CGPoint& a, const CGSize& b)
{
    return CGPointMake(a.x * b.width, a.y * b.height);
}

void ScrollingTreeFixedNode::parentScrollPositionDidChange(const IntRect& viewportRect, const FloatSize& cumulativeDelta)
{
    FloatPoint layerPosition = m_constraints.layerPositionForViewportRect(viewportRect);
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
