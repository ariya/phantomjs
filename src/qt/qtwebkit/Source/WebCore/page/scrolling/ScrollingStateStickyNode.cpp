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
#include "ScrollingStateStickyNode.h"

#if ENABLE(THREADED_SCROLLING) || USE(COORDINATED_GRAPHICS)

#include "GraphicsLayer.h"
#include "ScrollingStateTree.h"
#include "TextStream.h"
#include <wtf/OwnPtr.h>

namespace WebCore {

PassOwnPtr<ScrollingStateStickyNode> ScrollingStateStickyNode::create(ScrollingStateTree* stateTree, ScrollingNodeID nodeID)
{
    return adoptPtr(new ScrollingStateStickyNode(stateTree, nodeID));
}

ScrollingStateStickyNode::ScrollingStateStickyNode(ScrollingStateTree* tree, ScrollingNodeID nodeID)
    : ScrollingStateNode(tree, nodeID)
{
}

ScrollingStateStickyNode::ScrollingStateStickyNode(const ScrollingStateStickyNode& node)
    : ScrollingStateNode(node)
    , m_constraints(StickyPositionViewportConstraints(node.viewportConstraints()))
{
}

ScrollingStateStickyNode::~ScrollingStateStickyNode()
{
}

PassOwnPtr<ScrollingStateNode> ScrollingStateStickyNode::clone()
{
    return adoptPtr(new ScrollingStateStickyNode(*this));
}

void ScrollingStateStickyNode::updateConstraints(const StickyPositionViewportConstraints& constraints)
{
    if (m_constraints == constraints)
        return;

    m_constraints = constraints;
    setPropertyChanged(ViewportConstraints);
    m_scrollingStateTree->setHasChangedProperties(true);
}

void ScrollingStateStickyNode::syncLayerPositionForViewportRect(const LayoutRect& viewportRect)
{
    FloatPoint position = m_constraints.layerPositionForConstrainingRect(viewportRect);
    graphicsLayer()->syncPosition(position);
}

void ScrollingStateStickyNode::dumpProperties(TextStream& ts, int indent) const
{
    ts << "(" << "Sticky node" << "\n";

    if (m_constraints.anchorEdges()) {
        writeIndent(ts, indent + 1);
        ts << "(anchor edges: ";
        if (m_constraints.hasAnchorEdge(ViewportConstraints::AnchorEdgeLeft))
            ts << "AnchorEdgeLeft ";
        if (m_constraints.hasAnchorEdge(ViewportConstraints::AnchorEdgeRight))
            ts << "AnchorEdgeRight ";
        if (m_constraints.hasAnchorEdge(ViewportConstraints::AnchorEdgeTop))
            ts << "AnchorEdgeTop ";
        if (m_constraints.hasAnchorEdge(ViewportConstraints::AnchorEdgeBottom))
            ts << "AnchorEdgeBottom";
        ts << ")\n";
    }

    if (m_constraints.hasAnchorEdge(ViewportConstraints::AnchorEdgeLeft)) {
        writeIndent(ts, indent + 1);
        ts << "(left offset " << m_constraints.leftOffset() << ")\n";
    }
    if (m_constraints.hasAnchorEdge(ViewportConstraints::AnchorEdgeRight)) {
        writeIndent(ts, indent + 1);
        ts << "(right offset " << m_constraints.rightOffset() << ")\n";
    }
    if (m_constraints.hasAnchorEdge(ViewportConstraints::AnchorEdgeTop)) {
        writeIndent(ts, indent + 1);
        ts << "(top offset " << m_constraints.topOffset() << ")\n";
    }
    if (m_constraints.hasAnchorEdge(ViewportConstraints::AnchorEdgeBottom)) {
        writeIndent(ts, indent + 1);
        ts << "(bottom offset " << m_constraints.bottomOffset() << ")\n";
    }

    writeIndent(ts, indent + 1);
    FloatRect r = m_constraints.containingBlockRect();
    ts << "(containing block rect " << r.x() << ", " << r.y() << " " << r.width() << " x " << r.height() << ")\n";

    writeIndent(ts, indent + 1);
    r = m_constraints.stickyBoxRect();
    ts << "(sticky box rect " << r.x() << " " << r.y() << " " << r.width() << " " << r.height() << ")\n";

    writeIndent(ts, indent + 1);
    r = m_constraints.stickyBoxRect();
    ts << "(sticky box rect " << r.x() << " " << r.y() << " " << r.width() << " " << r.height() << ")\n";

    writeIndent(ts, indent + 1);
    ts << "(sticky offset at last layout " << m_constraints.stickyOffsetAtLastLayout().width() << " " << m_constraints.stickyOffsetAtLastLayout().height() << ")\n";

    writeIndent(ts, indent + 1);
    ts << "(layer position at last layout " << m_constraints.layerPositionAtLastLayout().x() << " " << m_constraints.layerPositionAtLastLayout().y() << ")\n";
}

} // namespace WebCore

#endif // ENABLE(THREADED_SCROLLING) || USE(COORDINATED_GRAPHICS)
