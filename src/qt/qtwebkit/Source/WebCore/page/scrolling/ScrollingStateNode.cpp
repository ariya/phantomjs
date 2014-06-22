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
#include "ScrollingStateNode.h"

#if ENABLE(THREADED_SCROLLING) || USE(COORDINATED_GRAPHICS)

#include "ScrollingStateFixedNode.h"
#include "ScrollingStateTree.h"
#include "TextStream.h"

#include <wtf/text/WTFString.h>

namespace WebCore {

ScrollingStateNode::ScrollingStateNode(ScrollingStateTree* scrollingStateTree, ScrollingNodeID nodeID)
    : m_scrollingStateTree(scrollingStateTree)
    , m_nodeID(nodeID)
    , m_changedProperties(0)
    , m_parent(0)
{
}

// This copy constructor is used for cloning nodes in the tree, and it doesn't make sense
// to clone the relationship pointers, so don't copy that information from the original
// node.
ScrollingStateNode::ScrollingStateNode(const ScrollingStateNode& stateNode)
    : m_scrollingStateTree(0)
    , m_nodeID(stateNode.scrollingNodeID())
    , m_changedProperties(stateNode.changedProperties())
    , m_parent(0)
{
    // FIXME: why doesn't this set the GraphicsLayer?
    setScrollPlatformLayer(stateNode.platformScrollLayer());
}

ScrollingStateNode::~ScrollingStateNode()
{
}

PassOwnPtr<ScrollingStateNode> ScrollingStateNode::cloneAndReset()
{
    OwnPtr<ScrollingStateNode> clone = this->clone();

    // Now that this node is cloned, reset our change properties.
    resetChangedProperties();

    cloneAndResetChildren(clone.get());
    return clone.release();
}

void ScrollingStateNode::cloneAndResetChildren(ScrollingStateNode* clone)
{
    if (!m_children)
        return;

    size_t size = m_children->size();
    for (size_t i = 0; i < size; ++i)
        clone->appendChild(m_children->at(i)->cloneAndReset());
}

void ScrollingStateNode::appendChild(PassOwnPtr<ScrollingStateNode> childNode)
{
    childNode->setParent(this);

    if (!m_children)
        m_children = adoptPtr(new Vector<OwnPtr<ScrollingStateNode> >);

    m_children->append(childNode);
}

void ScrollingStateNode::removeChild(ScrollingStateNode* node)
{
    if (!m_children)
        return;

    size_t index = m_children->find(node);

    // The index will be notFound if the node to remove is a deeper-than-1-level descendant or
    // if node is the root state node.
    if (index != notFound) {
        m_scrollingStateTree->didRemoveNode(node->scrollingNodeID());
        m_children->remove(index);
        return;
    }

    size_t size = m_children->size();
    for (size_t i = 0; i < size; ++i)
        m_children->at(i)->removeChild(node);
}

void ScrollingStateNode::writeIndent(TextStream& ts, int indent)
{
    for (int i = 0; i != indent; ++i)
        ts << "  ";
}

void ScrollingStateNode::dump(TextStream& ts, int indent) const
{
    writeIndent(ts, indent);
    dumpProperties(ts, indent);

    if (m_children) {
        writeIndent(ts, indent + 1);
        size_t size = children()->size();
        ts << "(children " << size << "\n";
        
        for (size_t i = 0; i < size; i++)
            m_children->at(i)->dump(ts, indent + 2);
        writeIndent(ts, indent + 1);
        ts << ")\n";
    }

    writeIndent(ts, indent);
    ts << ")\n";
}

String ScrollingStateNode::scrollingStateTreeAsText() const
{
    TextStream ts;

    dump(ts, 0);
    return ts.release();
}

} // namespace WebCore

#endif // ENABLE(THREADED_SCROLLING) || USE(COORDINATED_GRAPHICS)
