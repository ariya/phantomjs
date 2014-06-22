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
#include "ScrollingTreeNode.h"

#if ENABLE(THREADED_SCROLLING)

#include "ScrollingStateTree.h"

namespace WebCore {

ScrollingTreeNode::ScrollingTreeNode(ScrollingTree* scrollingTree, ScrollingNodeID nodeID)
    : m_scrollingTree(scrollingTree)
    , m_nodeID(nodeID)
    , m_parent(0)
{
}

ScrollingTreeNode::~ScrollingTreeNode()
{
}

void ScrollingTreeNode::appendChild(PassOwnPtr<ScrollingTreeNode> childNode)
{
    childNode->setParent(this);

    if (!m_children)
        m_children = adoptPtr(new Vector<OwnPtr<ScrollingTreeNode> >);

    m_children->append(childNode);
}

void ScrollingTreeNode::removeChild(ScrollingTreeNode* node)
{
    if (!m_children)
        return;

    size_t index = m_children->find(node);

    // The index will be notFound if the node to remove is a deeper-than-1-level descendant or
    // if node is the root state node.
    if (index != notFound) {
        m_children->remove(index);
        return;
    }

    size_t size = m_children->size();
    for (size_t i = 0; i < size; ++i)
        m_children->at(i)->removeChild(node);
}

} // namespace WebCore

#endif // ENABLE(THREADED_SCROLLING)
