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
#include "ScrollingStateFixedNode.h"

#include "GraphicsLayer.h"
#include "ScrollingStateTree.h"
#include "TextStream.h"
#include <wtf/OwnPtr.h>

#if ENABLE(THREADED_SCROLLING) || USE(COORDINATED_GRAPHICS)

namespace WebCore {

PassOwnPtr<ScrollingStateFixedNode> ScrollingStateFixedNode::create(ScrollingStateTree* stateTree, ScrollingNodeID nodeID)
{
    return adoptPtr(new ScrollingStateFixedNode(stateTree, nodeID));
}

ScrollingStateFixedNode::ScrollingStateFixedNode(ScrollingStateTree* tree, ScrollingNodeID nodeID)
    : ScrollingStateNode(tree, nodeID)
{
}

ScrollingStateFixedNode::ScrollingStateFixedNode(const ScrollingStateFixedNode& node)
    : ScrollingStateNode(node)
    , m_constraints(FixedPositionViewportConstraints(node.viewportConstraints()))
{
}

ScrollingStateFixedNode::~ScrollingStateFixedNode()
{
}

PassOwnPtr<ScrollingStateNode> ScrollingStateFixedNode::clone()
{
    return adoptPtr(new ScrollingStateFixedNode(*this));
}

void ScrollingStateFixedNode::updateConstraints(const FixedPositionViewportConstraints& constraints)
{
    if (m_constraints == constraints)
        return;

    m_constraints = constraints;
    setPropertyChanged(ViewportConstraints);
    m_scrollingStateTree->setHasChangedProperties(true);
}

void ScrollingStateFixedNode::syncLayerPositionForViewportRect(const LayoutRect& viewportRect)
{
    FloatPoint position = m_constraints.layerPositionForViewportRect(viewportRect);
    graphicsLayer()->syncPosition(position);
}

void ScrollingStateFixedNode::dumpProperties(TextStream& ts, int indent) const
{
    ts << "(" << "Fixed node" << "\n";

    if (m_constraints.anchorEdges()) {
        writeIndent(ts, indent + 1);
        ts << "(anchor edges: ";
        if (m_constraints.hasAnchorEdge(ViewportConstraints::AnchorEdgeLeft))
            ts << "AnchorEdgeLeft ";
        if (m_constraints.hasAnchorEdge(ViewportConstraints::AnchorEdgeRight))
            ts << "AnchorEdgeRight ";
        if (m_constraints.hasAnchorEdge(ViewportConstraints::AnchorEdgeTop))
            ts << "AnchorEdgeTop";
        if (m_constraints.hasAnchorEdge(ViewportConstraints::AnchorEdgeBottom))
            ts << "AnchorEdgeBottom";
        ts << ")\n";
    }

    if (!m_constraints.alignmentOffset().isEmpty()) {
        writeIndent(ts, indent + 1);
        ts << "(alignment offset " << m_constraints.alignmentOffset().width() << " " << m_constraints.alignmentOffset().height() << ")\n";
    }

    if (!m_constraints.viewportRectAtLastLayout().isEmpty()) {
        writeIndent(ts, indent + 1);
        FloatRect viewportRect = m_constraints.viewportRectAtLastLayout();
        ts << "(viewport rect at last layout: " << viewportRect.x() << " " << viewportRect.y() << " " << viewportRect.width() << " " << viewportRect.height() << ")\n";
    }

    if (m_constraints.layerPositionAtLastLayout() != FloatPoint()) {
        writeIndent(ts, indent + 1);
        ts << "(layer position at last layout " << m_constraints.layerPositionAtLastLayout().x() << " " << m_constraints.layerPositionAtLastLayout().y() << ")\n";
    }
}

} // namespace WebCore

#endif // ENABLE(THREADED_SCROLLING) || USE(COORDINATED_GRAPHICS)
